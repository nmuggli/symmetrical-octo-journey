/*
 * "cupsfile.c 2021-05-17 15:55:05
 *  
 *  cups file for TSC Printer Driver
 *  
 *  Copyright (c) 2005,by TSC Printronix Auto ID .
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at

 *      http://www.apache.org/licenses/LICENSE-2.0

 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *	
 *
 */
#include "config.h"
#include "common.h"
#include "debug.h"
#include <errno.h>

#include "cupsinc/cups.h"
#include "cupsinc/file.h"

#include "mycups.h"

/*
 * Some operating systems support large files via open flag O_LARGEFILE...
 */

#ifndef O_LARGEFILE
#  define O_LARGEFILE 0
#endif /* !O_LARGEFILE */


/*
 * Some operating systems don't define O_BINARY, which is used by Microsoft
 * and IBM to flag binary files...
 */

#ifndef O_BINARY
#  define O_BINARY 0
#endif /* !O_BINARY */

static cups_file_t *stdio_files[1] = {NULL};

/*
 * Types and structures...
 */

struct _cups_file_s			/**** CUPS file structure... ****/
{
  int		fd;			/* File descriptor */
  char		mode,			/* Mode ('r' or 'w') */
		compressed,		/* Compression used? */
		is_stdio,		/* stdin/out/err? */
		eof,			/* End of file? */
		buf[4096],		/* Buffer */
		*ptr,			/* Pointer into buffer */
		*end;			/* End of buffer data */
  off_t		pos;			/* File position for start of buffer */
};


/*
 * Local functions...
 */

static ssize_t	cups_fill(cups_file_t *fp);
static ssize_t	cups_read(cups_file_t *fp, char *buf, size_t bytes);
static ssize_t	cups_write(cups_file_t *fp, const char *buf, size_t bytes);
static int my_cupsTempFd(char *filename, int len);


cups_file_t *				/* O - CUPS file or NULL on error */
my_cupsTempFile2(char *filename,		/* I - Pointer to buffer */
              int  len)			/* I - Size of buffer */
{
  cups_file_t	*file;			/* CUPS file */
  int		fd;			/* File descriptor */

  if ((fd = my_cupsTempFd(filename, len)) < 0)
    return (NULL);
  else if ((file = my_cupsFileOpenFd(fd, "w")) == NULL)
  {
    close(fd);
    unlink(filename);
    return (NULL);
  }
  else
    return (file);
}

int					/* O - New file descriptor or -1 on error */
my_cupsTempFd(char *filename,		/* I - Pointer to buffer */
           int  len)			/* I - Size of buffer */
{
  int		fd;			/* File descriptor for temp file */
  int		tries;			/* Number of tries */
  const char	*tmpdir;		/* TMPDIR environment var */
  struct timeval curtime;		/* Current time */

 /*
  * See if TMPDIR is defined...
  */

 /*
  * Previously we put root temporary files in the default CUPS temporary
  * directory under /var/spool/cups.  However, since the scheduler cleans
  * out temporary files there and runs independently of the user apps, we
  * don't want to use it unless specifically told to by cupsd.
  */

  if ((tmpdir = getenv("TMPDIR")) == NULL)
    tmpdir = "/tmp";

 /*
  * Make the temporary name using the specified directory...
  */

  tries = 0;

  do
  {
   /*
    * Get the current time of day...
    */

    gettimeofday(&curtime, NULL);

   /*
    * Format a string using the hex time values...
    */

    snprintf(filename, len - 1, "%s/%08lx%05lx", tmpdir,
             (unsigned long)curtime.tv_sec, (unsigned long)curtime.tv_usec);

   /*
    * Open the file in "exclusive" mode, making sure that we don't
    * stomp on an existing file or someone's symlink crack...
    */
    fd = open(filename, O_RDWR | O_CREAT | O_EXCL, 0600);

    if (fd < 0 && errno != EEXIST)
      break;

    tries ++;
  }
  while (fd < 0 && tries < 1000);

 /*
  * Return the file descriptor...
  */

  return (fd);
}

/*
 * 'cupsFileClose()' - Close a CUPS file.
 */

int					/* O - 0 on success, -1 on error */
my_cupsFileClose(cups_file_t *fp)		/* I - CUPS file */
{
  int	fd;				/* File descriptor */
  char	mode;				/* Open mode */
  int	status;				/* Return status */
  int	is_stdio;			/* Is a stdio file? */

 /*
  * Range check...
  */

  if (!fp)
    return (-1);

 /*
  * Flush pending write data...
  */

  if (fp->mode == 'w')
    status = my_cupsFileFlush(fp);
  else
    status = 0;

 /*
  * Save the file descriptor we used and free memory...
  */

  fd       = fp->fd;
  mode     = fp->mode;
  is_stdio = fp->is_stdio;

  free(fp);

 /*
  * Close the file, returning the close status...
  */
  if (!is_stdio)
  {
    if (close(fd) < 0)
      status = -1;
  }

  return (status);
}

/*
 * 'cupsFileFlush()' - Flush pending output.
 */

int					/* O - 0 on success, -1 on error */
my_cupsFileFlush(cups_file_t *fp)		/* I - CUPS file */
{
  ssize_t	bytes;			/* Bytes to write */


//  DEBUG_printf(("cupsFileFlush(fp=%p)\n", fp));

 /*
  * Range check input...
  */

  if (!fp || fp->mode != 'w')
  {
//    DEBUG_puts("    Attempt to flush a read-only file...");
    return (-1);
  }

  bytes = (ssize_t)(fp->ptr - fp->buf);

//  DEBUG_printf(("    Flushing %ld bytes...\n", (long)bytes));

  if (bytes > 0)
  {
      bytes = cups_write(fp, fp->buf, bytes);

    if (bytes < 0)
      return (-1);

    fp->ptr = fp->buf;
  }
   
  return (0);
}


/*
 * 'cupsFileGetLine()' - Get a CR and/or LF-terminated line that may
 *                       contain binary data.
 *
 * This function differs from cupsFileGets() in that the trailing CR and LF
 * are preserved, as is any binary data on the line. The buffer is nul-
 * terminated, however you should use the returned length to determine
 * the number of bytes on the line.
 */

size_t					/* O - Number of bytes on line or 0 on EOF */
my_cupsFileGetLine(cups_file_t *fp,	/* I - File to read from */
                char        *buf,	/* I - Buffer */
                size_t      buflen)	/* I - Size of buffer */
{
  int		ch;			/* Character from file */
  char		*ptr,			/* Current position in line buffer */
		*end;			/* End of line buffer */


 /*
  * Range check input...
  */

  if (!fp || (fp->mode != 'r' && fp->mode != 's') || !buf || buflen < 3)
    return (0);

 /*
  * Now loop until we have a valid line...
  */

  for (ptr = buf, end = buf + buflen - 2; ptr < end ;)
  {
    if (fp->ptr >= fp->end)
      if (cups_fill(fp) <= 0)
        break;

    *ptr++ = ch = *(fp->ptr)++;

    if (ch == '\r')
    {
     /*
      * Check for CR LF...
      */

      if (fp->ptr >= fp->end)
	if (cups_fill(fp) <= 0)
          break;

      if (*(fp->ptr) == '\n')
        *ptr++ = *(fp->ptr)++;

      break;
    }
    else if (ch == '\n')
    {
     /*
      * Line feed ends a line...
      */

      break;
    }
  }

  *ptr = '\0';

  return (ptr - buf);
}

/*
 * 'cupsFileOpen()' - Open a CUPS file.
 */

cups_file_t *				/* O - CUPS file or NULL */
my_cupsFileOpen(const char *filename,	/* I - Name of file */
             const char *mode)		/* I - Open mode */
{
  cups_file_t	*fp;			/* New CUPS file */
  int		fd;			/* File descriptor */
  char		hostname[1024],		/* Hostname */
		*portname;		/* Port "name" (number or service) */
  http_addrlist_t *addrlist;		/* Host address list */


//  DEBUG_printf(("cupsFileOpen(filename=\"%s\", mode=\"%s\")\n", filename,
//                mode));

 /*
  * Range check input...
  */

  if (!filename || !mode ||
      (*mode != 'r' && *mode != 'w' && *mode != 'a'))
    return (NULL);

 /*
  * Open the file...
  */

  switch (*mode)
  {
    case 'a' : /* Append file */
        fd = open(filename, O_RDWR | O_CREAT | O_APPEND | O_LARGEFILE | O_BINARY, 0666);
        break;

    case 'r' : /* Read file */
		fd = open(filename, O_RDONLY | O_LARGEFILE | O_BINARY, 0);
		break;

    case 'w' : /* Write file */
        fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT | O_LARGEFILE | O_BINARY, 0666);
        break;

    default : /* Remove bogus compiler warning... */
        return (NULL);
  }

  if (fd < 0)
    return (NULL);

 /*
  * Create the CUPS file structure...
  */

  if ((fp = my_cupsFileOpenFd(fd, mode)) == NULL)
  {
    close(fd);
  }

 /*
  * Return it...
  */

  return (fp);
}

/*
 * 'cupsFileOpenFd()' - Open a CUPS file using a file descriptor.
 */

cups_file_t *				/* O - CUPS file or NULL */
my_cupsFileOpenFd(int        fd,		/* I - File descriptor */
	       const char *mode)	/* I - Open mode */
{
  cups_file_t	*fp;			/* New CUPS file */


//  DEBUG_printf(("cupsFileOpenFd(fd=%d, mode=\"%s\")\n", fd, mode));

 /*
  * Range check input...
  */

  if (fd < 0 || !mode ||
      (*mode != 'r' && *mode != 'w' && *mode != 'a'))
    return (NULL);

 /*
  * Allocate memory...
  */

  if ((fp = calloc(1, sizeof(cups_file_t))) == NULL)
    return (NULL);

 /*
  * Open the file...
  */

  fp->fd = fd;

  switch (*mode)
  {
    case 'w' :
    case 'a' :
		fp->mode = 'w';
		fp->ptr  = fp->buf;
		fp->end  = fp->buf + sizeof(fp->buf);
        break;

    case 'r' :
		fp->mode = 'r';
		break;

    default : /* Remove bogus compiler warning... */
        return (NULL);
  }

 /*
  * Don't pass this file to child processes...
  */

#ifndef WIN32
  fcntl(fp->fd, F_SETFD, fcntl(fp->fd, F_GETFD) | FD_CLOEXEC);
#endif /* !WIN32 */

  return (fp);
}


/*
 * 'cupsFileRead()' - Read from a file.
 */

ssize_t					/* O - Number of bytes read or -1 */
my_cupsFileRead(cups_file_t *fp,		/* I - CUPS file */
             char        *buf,		/* O - Buffer */
	     size_t      bytes)		/* I - Number of bytes to read */
{
  size_t	total;			/* Total bytes read */
  ssize_t	count;			/* Bytes read */


//  DEBUG_printf(("cupsFileRead(fp=%p, buf=%p, bytes=%ld)\n", fp, buf,
//                (long)bytes));

 /*
  * Range check input...
  */

  if (!fp || !buf || bytes < 0 || fp->mode != 'r')
    return (-1);

  if (bytes == 0)
    return (0);

 /*
  * Loop until all bytes are read...
  */

  total = 0;
  while (bytes > 0)
  {
    if (fp->ptr >= fp->end)
      if (cups_fill(fp) <= 0)
      {
//        DEBUG_printf(("    cups_fill() returned -1, total=%d\n", (int)total));

        if (total > 0)
          return ((ssize_t)total);
	else
	  return (-1);
      }

    count = (ssize_t)(fp->end - fp->ptr);
    if (count > (ssize_t)bytes)
      count = (ssize_t)bytes;

    memcpy(buf, fp->ptr, count);
    fp->ptr += count;

   /*
    * Update the counts for the last read...
    */

    bytes -= count;
    total += count;
    buf   += count;
  }

 /*
  * Return the total number of bytes read...
  */

//  DEBUG_printf(("    total=%d\n", (int)total));

  return ((ssize_t)total);
}

/*
 * 'cupsFileRewind()' - Rewind a file.
 */

off_t					/* O - New file position or -1 */
my_cupsFileRewind(cups_file_t *fp)		/* I - CUPS file */
{
 /*
  * Range check input...
  */

  if (!fp || fp->mode != 'r')
    return (-1);

 /*
  * Handle special cases...
  */

  if (fp->pos == 0)
  {
   /*
    * No seeking necessary...
    */

    if (fp->ptr)
    {
      fp->ptr = fp->buf;
      fp->eof = 0;
    }

    return (0);
  }

  lseek(fp->fd, 0, SEEK_SET);

  fp->pos = 0;
  fp->ptr = NULL;
  fp->end = NULL;
  fp->eof = 0;

  return (0);
}


/*
 * 'cupsFileSeek()' - Seek in a file.
 */

off_t					/* O - New file position or -1 */
my_cupsFileSeek(cups_file_t *fp,		/* I - CUPS file */
             off_t       pos)		/* I - Position in file */
{
  ssize_t	bytes;			/* Number bytes in buffer */


//  DEBUG_printf(("cupsFileSeek(fp=%p, pos=" CUPS_LLFMT ")\n", fp, pos));
//  DEBUG_printf(("    fp->pos=" CUPS_LLFMT "\n", fp->pos));
//  DEBUG_printf(("    fp->ptr=%p, fp->end=%p\n", fp->ptr, fp->end));

 /*
  * Range check input...
  */

  if (!fp || pos < 0 || fp->mode != 'r')
    return (-1);

 /*
  * Handle special cases...
  */

  if (pos == 0)
    return (my_cupsFileRewind(fp));

  if (fp->pos == pos)
  {
   /*
    * No seeking necessary...
    */

    if (fp->ptr)
    {
      fp->ptr = fp->buf;
      fp->eof = 0;
    }

    return (pos);
  }


 /*
  * Figure out the number of bytes in the current buffer, and then
  * see if we are outside of it...
  */

  if (fp->ptr)
    bytes = (ssize_t)(fp->end - fp->buf);
  else
    bytes = 0;

  fp->eof = 0;

//  DEBUG_printf(("    bytes=" CUPS_LLFMT "\n", CUPS_LLCAST bytes));

  if (pos < fp->pos)
  {
   /*
    * Need to seek backwards...
    */

//    DEBUG_puts("    SEEK BACKWARDS");

    {
      fp->pos = lseek(fp->fd, pos, SEEK_SET);
      fp->ptr = NULL;
      fp->end = NULL;

//      DEBUG_printf(("    lseek() returned %ld...\n", (long)fp->pos));
    }
  }
  else if (pos >= (fp->pos + bytes))
  {
   /*
    * Need to seek forwards...
    */

//    DEBUG_puts("    SEEK FORWARDS");

    {
      fp->pos = lseek(fp->fd, pos, SEEK_SET);
      fp->ptr = NULL;
      fp->end = NULL;

//      DEBUG_printf(("    lseek() returned " CUPS_LLFMT "...\n", fp->pos));
    }
  }
  else
  {
   /*
    * Just reposition the current pointer, since we have the right
    * range...
    */

//    DEBUG_puts("    SEEK INSIDE BUFFER");

    fp->ptr = fp->buf + pos - fp->pos;
  }

  return (fp->pos);
}

/*
 * 'cupsFileStdin()' - Return a CUPS file associated with stdin.
 */

cups_file_t *
my_cupsFileStdin(void)
{
 /*
  * Open file descriptor 0 as needed...
  */
DebugPrintf( "Enter cupsFileStdin");

  if (!stdio_files[0])
  {
   /*
    * Open file descriptor 0...
    */

    if ((stdio_files[0] = my_cupsFileOpenFd(0, "r")) != NULL)
      stdio_files[0]->is_stdio = 1;
  }

  return (stdio_files[0]);
}

/*
 * 'cupsFileTell()' - Return the current file position.
 */

off_t					/* O - File position */
my_cupsFileTell(cups_file_t *fp)		/* I - CUPS file */
{
  return (fp ? fp->pos : 0);
}

/*
 * 'cupsFileWrite()' - Write to a file.
 */

ssize_t					/* O - Number of bytes written */
my_cupsFileWrite(cups_file_t *fp,		/* I - CUPS file */
              const char  *buf,		/* I - Buffer */
	      size_t      bytes)	/* I - Number of bytes to write */
{
 /*
  * Range check input...
  */

  if (!fp || !buf || bytes < 0 || fp->mode != 'w' )
    return (-1);

  if (bytes == 0)
    return (0);

 /*
  * Write the buffer...
  */

  if ((fp->ptr + bytes) > fp->end)
    if (my_cupsFileFlush(fp))
      return (-1);

  fp->pos += (off_t)bytes;

  if (bytes > sizeof(fp->buf))
  {
      return (cups_write(fp, buf, bytes));
  }
  else
  {
    memcpy(fp->ptr, buf, bytes);
    fp->ptr += bytes;
    return ((ssize_t)bytes);
  }
}


/*
 * 'cups_fill()' - Fill the input buffer...
 */

static ssize_t				/* O - Number of bytes or -1 */
cups_fill(cups_file_t *fp)		/* I - CUPS file */
{
  ssize_t		bytes;		/* Number of bytes read */


//  DEBUG_printf(("cups_fill(fp=%p)\n", fp));
//  DEBUG_printf(("    fp->ptr=%p, fp->end=%p, fp->buf=%p, "
//                "fp->pos=" CUPS_LLFMT ", fp->eof=%d\n",
//                fp->ptr, fp->end, fp->buf, fp->pos, fp->eof));

 /*
  * Update the "pos" element as needed...
  */

  if (fp->ptr && fp->end)
    fp->pos += (off_t)(fp->end - fp->buf);

 /*
  * Read a buffer's full of data...
  */

  if ((bytes = cups_read(fp, fp->buf, sizeof(fp->buf))) <= 0)
  {
   /*
    * Can't read from file!
    */

    fp->eof = 1;
    fp->ptr = fp->buf;
    fp->end = fp->buf;

    return (-1);
  }

 /*
  * Return the bytes we read...
  */

  fp->eof = 0;
  fp->ptr = fp->buf;
  fp->end = fp->buf + bytes;

  return (bytes);
}


/*
 * 'cups_read()' - Read from a file descriptor.
 */

static ssize_t				/* O - Number of bytes read or -1 */
cups_read(cups_file_t *fp,		/* I - CUPS file */
          char        *buf,		/* I - Buffer */
	  size_t      bytes)		/* I - Number bytes */
{
  ssize_t	total;			/* Total bytes read */


 /*
  * Loop until we read at least 0 bytes...
  */

  for (;;)
  {
#ifdef WIN32
      total = (ssize_t)read(fp->fd, buf, (unsigned)bytes);
#else
      total = read(fp->fd, buf, bytes);
#endif /* WIN32 */

    if (total >= 0)
      break;

   /*
    * Reads can be interrupted by signals and unavailable resources...
    */

    if (errno == EAGAIN || errno == EINTR)
      continue;
    else
      return (-1);
  }

 /*
  * Return the total number of bytes read...
  */

  return (total);
}


/*
 * 'cups_write()' - Write to a file descriptor.
 */

static ssize_t				/* O - Number of bytes written or -1 */
cups_write(cups_file_t *fp,		/* I - CUPS file */
           const char  *buf,		/* I - Buffer */
	   size_t      bytes)		/* I - Number bytes */
{
  size_t	total;			/* Total bytes written */
  ssize_t	count;			/* Count this time */


//  DEBUG_printf(("cups_write(fp=%p, buf=%p, bytes=%ld)\n", fp, buf,
//                (long)bytes));

 /*
  * Loop until all bytes are written...
  */

  total = 0;
  while (bytes > 0)
  {
#ifdef WIN32
      count = (ssize_t)write(fp->fd, buf, (unsigned)bytes);
#else
      count = write(fp->fd, buf, bytes);
#endif /* WIN32 */

    if (count < 0)
    {
     /*
      * Writes can be interrupted by signals and unavailable resources...
      */

      if (errno == EAGAIN || errno == EINTR)
        continue;
      else
        return (-1);
    }

//    DEBUG_printf(("    count=%ld\n", (long)count));

   /*
    * Update the counts for the last write call...
    */

    bytes -= count;
    total += count;
    buf   += count;
  }

 /*
  * Return the total number of bytes written...
  */

  return ((ssize_t)total);
}
