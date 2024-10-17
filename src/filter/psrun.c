/*
 * "psrun.c 2021-05-17 15:55:05
 *  
 *  ps run routines for TSC Printer Driver
 *  
 *  Copyright (c) 2005, by TSC Printronix Auto ID .
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at

 *      http://www.apache.org/licenses/LICENSE-2.0

 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 
 */

#include "config.h"
#include "common.h"
#include "debug.h"
#include "device.h"
#include "libloader.h"
#include "gsrun.h"

#ifndef FILTER_NOT_PSTOPS

typedef struct				/**** Page information ****/
{
	char			*label;				/* Page label */
	int				bounding_box[4];	/* PageBoundingBox */
	float			page_width;
	float			page_height;
	off_t			offset;				/* Offset to start of page */
	ssize_t			length;				/* Number of bytes for page */
	int				num_options;		/* Number of options for this page */
	cups_option_t	*options;			/* Options for this page */
} pstops_page_t;

typedef struct
{
	int				page;					/* Current page */
	int				Orientation;			/* 0 = portrait, 1 = landscape, etc. */
	ppd_size_t		*pPageSize;
	ppd_size_t		*pPageSizeOutput;

	float			PageLeft;
	float			PageRight;
	float			PageBottom;
	float			PageTop;
	float			PageWidth;
	float			PageLength;

	cups_array_t	*pages;					/* Pages in document */
	cups_file_t		*temp;					/* Temporary file, if any */
	char			tempfile[1024];			/* Temporary filename */
	FILE			*fp_temp;				/* Temporary file for read, if any */

	int				have_ESPshowpage;
	int				use_PANshowpage;
	int				use_PANrc;
	int				changed_pageorder;
	int				total_page;
	int				collate;

}	pstops_doc_t;


static void copy_dsc(DEVDATA *pdev, pstops_doc_t *doc, char *line, ssize_t linelen, size_t linesize);
static ssize_t copy_comments(DEVDATA *pdev, pstops_doc_t *doc, char *line, ssize_t linelen, size_t linesize);
static ssize_t copy_prolog(DEVDATA *pdev, pstops_doc_t *doc, char *line, ssize_t linelen, size_t linesize);
static ssize_t copy_setup(DEVDATA *pdev, pstops_doc_t *doc, char *line, ssize_t linelen, size_t linesize);
static ssize_t copy_page(DEVDATA *pdev, pstops_doc_t *doc, char *line, ssize_t linelen, size_t linesize);
static ssize_t copy_trailer(DEVDATA *pdev, pstops_doc_t *doc, char *line, ssize_t linelen, size_t linesize);

static char * parse_text(const char	*start, char **end, char *buffer, size_t bufsize);
static pstops_page_t *add_page(DEVDATA *pdev, pstops_doc_t *doc, const char *label);

#define doc_puts(pdev, doc, s)		doc_write(pdev, doc, s, strlen(s))
static void copy_bytes(DEVDATA *pdev, FILE *fp, off_t offset, size_t length);
static void doc_printf(DEVDATA *pdev, pstops_doc_t *doc, const char *format, ...);
static void doc_write(DEVDATA *pdev, pstops_doc_t *doc, const char *s, size_t len);


int psrun(
	DEVDATA		*pdev,
	char		*line,
	size_t		linelen,
	size_t		linesize
)
{
	int				nRtn = 0;
	pstops_doc_t	doc;				/* Document information */
	ppd_choice_t	*choice;			/* PPD choice */

	DebugPrintf("\n#ENTER: pstops()\n");

	memset(&doc, 0, sizeof(doc));

	doc.collate = 0;
    if ((choice = pdev->lib_cups.ppdFindMarkedChoice(pdev->ppd, "Collate")) != NULL &&
    	!strcasecmp(choice->choice, "True"))
    {
		doc.collate = 1;
	}

	// Start with a DSC header...
	gs_puts(pdev, "%!PS-Adobe-3.0\n");

    if ((doc.temp = pdev->lib_cups.cupsTempFile2(doc.tempfile, sizeof(doc.tempfile))) == NULL)
    {
		Error_Log(LEVEL_ERROR, "Unable to create temporary file: %s\n", strerror(errno));
		exit(1);
    }

	// filter the document...
    copy_dsc(pdev, &doc, line, linelen, linesize);

	// Send %%EOF as needed...
	gs_puts(pdev, "%%EOF\n");

	if ( doc.fp_temp )
	{
		fclose(doc.fp_temp);
		unlink(doc.tempfile);
	}
	if (doc.temp)
	{
		pdev->lib_cups.cupsFileClose(doc.temp);
		unlink(doc.tempfile);
	}

	DebugPrintf("\n#LEAVE: pstops()\n");

	return nRtn;
}

void copy_dsc(
	DEVDATA			*pdev,
	pstops_doc_t	*doc,
	char			*line,
	ssize_t			linelen,
	size_t			linesize
)
{
	int					number;			/* Page number */
	pstops_page_t		*pageinfo;		/* Page information */

	// Start sending the document with any commands needed...
	linelen = copy_comments(pdev, doc, line, linelen, linesize);

	// Now find the prolog section, if any...
	linelen = copy_prolog(pdev, doc, line, linelen, linesize);

	// Then the document setup section...
	linelen = copy_setup(pdev, doc, line, linelen, linesize);

	// Copy until we see %%Page:...
	while (strncmp(line, "%%Page:", 7) && strncmp(line, "%%Trailer", 9))
	{
		gs_write(pdev, line, linelen);

		if ((linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize)) == 0)
			break;
	}

	// Then process pages until we have no more...
	doc->total_page = 0;

	while (!strncmp(line, "%%Page:", 7))
	{
		doc->total_page ++;

		linelen = copy_page(pdev, doc, line, linelen, linesize);
	}

	// Make additional copies as necessary...
	number = pdev->lib_cups.cupsArrayCount(doc->pages);
	if (doc->temp && number > 0)
	{
		// Reopen the temporary file for reading...
		pdev->lib_cups.cupsFileClose(doc->temp);
		doc->temp = NULL;
		doc->fp_temp = fopen(doc->tempfile, "r");

		doc->total_page = 0;
		{
			int			i;
			int			nCopies;
			int			nDriverCoyies = 1;		// Collection Copies

			pdev->dm.dmDocPages = number;
			pdev->dm.dmCollate = 0;
			if ( number > 1 && doc->collate )
			{
				pdev->dm.dmCollate = 1;
				nDriverCoyies = pdev->dm.dmCopies;
			}
			OutputDevmode(pdev);

			DebugPrintf("  nDriverCoyies = %d, dmDocPages = %d\n", nDriverCoyies, number);

			for ( nCopies=0; nCopies<nDriverCoyies; nCopies++ )
			{
				for(i=0; i<number; i++)
				{
					pageinfo = (pstops_page_t *)pdev->lib_cups.cupsArrayIndex(doc->pages, i);

					if ( pageinfo )
					{
						doc->total_page ++;
						gs_printf(pdev, "%%%%Page: %s %d\n", pageinfo->label, doc->total_page);

						copy_bytes(pdev, doc->fp_temp, pageinfo->offset, pageinfo->length);
					}
					else
					{
						Error_Log(LEVEL_ERROR, "Cannot get page %d data\n", i+1);
					}
				}
			}
		}
	}

	// Write/copy the trailer...
	linelen = copy_trailer(pdev, doc, line, linelen, linesize);
}

ssize_t				/* O - Length of next line */
copy_comments(
	DEVDATA			*pdev,
	pstops_doc_t	*doc,
	char			*line,
	ssize_t			linelen,
	size_t			linesize
)
{
	// Loop until we see %%EndComments or a non-comment line...
	while (line[0] == '%')
	{
		// Strip trailing whitespace...
		while (linelen > 0)
		{
			linelen --;

			if (!isspace(line[linelen] & 255))
				break;
			else
				line[linelen] = '\0';
		}

		// Pull the headers out...
		if (!strncmp(line, "%%Pages:", 8))
		{
		}
		else if (!strncmp(line, "%%BoundingBox:", 14))
		{
		}
//		else if (!strncmp(line, "%cupsRotation:", 14))
//		{
//			// Reset orientation of document?
//		}
		else if (!strcmp(line, "%%EndComments"))
		{
			linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize);
			break;
		}
		else if (strncmp(line, "%!", 2) && strncmp(line, "%cups", 5))
			gs_printf(pdev, "%s\n", line);

		if ((linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize)) == 0)
			break;
	}
	gs_puts(pdev, "%%Pages: (atend)\n");
	gs_puts(pdev, "%%BoundingBox: (atend)\n");
	gs_puts(pdev, "%%EndComments\n");

	return (linelen);
}

ssize_t	copy_prolog(
	DEVDATA			*pdev,
	pstops_doc_t	*doc,
	char			*line,
	ssize_t			linelen,
	size_t			linesize
)
{
	while (strncmp(line, "%%BeginProlog", 13))
	{
		if (!strncmp(line, "%%BeginSetup", 12) || !strncmp(line, "%%Page:", 7))
			break;

		gs_write(pdev, line, linelen);

		if ((linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize)) == 0)
			break;
	}
	gs_puts(pdev, "%%BeginProlog\n");

	if (!strncmp(line, "%%BeginProlog", 13))
	{
		while ((linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize)) > 0)
		{
			if (!strncmp(line, "%%EndProlog", 11) ||
				!strncmp(line, "%%BeginSetup", 12) ||
				!strncmp(line, "%%Page:", 7))
				break;

			gs_write(pdev, line, linelen);
		}

		if (!strncmp(line, "%%EndProlog", 11))
			linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize);
	}

	gs_puts(pdev, "%%EndProlog\n");

	return (linelen);
}

ssize_t	copy_setup(
	DEVDATA			*pdev,
	pstops_doc_t	*doc,
	char			*line,
	ssize_t			linelen,
	size_t			linesize
)
{
	BOOL	bNotUseCupsCopies = TRUE;

	while (strncmp(line, "%%BeginSetup", 12))
	{
		if (!strncmp(line, "%%Page:", 7))
			break;

		gs_write(pdev, line, linelen);

		if ((linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize)) == 0)
			break;
	}

	gs_puts(pdev, "%%BeginSetup\n");

	if (!strncmp(line, "%%BeginSetup", 12))
	{
		BOOL	bOut = TRUE;
		while (strncmp(line, "%%EndSetup", 10))
		{
			if (!strncmp(line, "%%Page:", 7))
				break;
//			else if (!strncmp(line, "%%IncludeFeature:", 17))
//			{
//
//				// %%IncludeFeature: *MainKeyword OptionKeyword
//
//				if (doc->number_up == 1 && !doc->fitplot)
//					doc->num_options = include_feature(ppd, line, doc->num_options,
//				                                     &(doc->options));
//			}
			// Do not use CUPS copies
			else if (bNotUseCupsCopies && !strncmp(line, "%RBIBeginNonPPDFeature:", 23))
			{
				bOut = FALSE;
			}
			else if (bNotUseCupsCopies && !bOut && !strncmp(line, "%RBIEndNonPPDFeature", 20))
			{
				bOut = TRUE;
			}
			else if (strncmp(line, "%%BeginSetup", 12))
			{
				if ( bOut )
				{
					gs_write(pdev, line, linelen);
				}
			}

			if ((linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize)) == 0)
				break;
		}

		if (!strncmp(line, "%%EndSetup", 10))
			linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize);
	}

	gs_puts(pdev, "%%EndSetup\n");

	return (linelen);
}

ssize_t	copy_page(
	DEVDATA			*pdev,
	pstops_doc_t	*doc,
	char			*line,
	ssize_t			linelen,
	size_t			linesize
)
{
	char	label[256],		/* Page label string */
			*ptr;			/* Pointer into line */
	int		level;				/* Embedded document level */
	pstops_page_t	*pageinfo;		/* Page information */
	int		first_page;			/* First page on N-up output? */
	int		bounding_box[4];	/* PageBoundingBox */
	int		number;

	DebugPrintf("#Enter copy_page(), Page = %d\n", doc->total_page);

	if (!parse_text(line + 7, &ptr, label, sizeof(label)))
	{
		Error_Log(LEVEL_ERROR, "Bad %%Page: comment in file!\n");
		label[0] = '\0';
		number   = doc->total_page;
	}
	else if (strtol(ptr, &ptr, 10) == LONG_MAX || !isspace(*ptr & 255))
	{
		Error_Log(LEVEL_ERROR, "Bad %%Page: comment in file!\n");
		number = doc->total_page;
	}

	pageinfo = add_page(pdev, doc, label);

//	memcpy(bounding_box, doc->bounding_box, sizeof(bounding_box));
	while ((linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize)) > 0)
	{
		if (!strncmp(line, "%%PageBoundingBox:", 18))
		{
			// %%PageBoundingBox: llx lly urx ury
//			if (sscanf(line + 18, "%d%d%d%d", bounding_box + 0,
//				bounding_box + 1, bounding_box + 2,
//				bounding_box + 3) != 4)
//			{
//				memcpy(bounding_box, doc->bounding_box, sizeof(bounding_box));
//				DebugPrintf("Get PageBoundingBox ERROR!\n");
//			}
//			else
//			{
//				DebugPrintf("Get PageBoundingBox OK!\n");
//			}
//			DebugPrintf("%%%%PageBoundingBox: %d %d %d %d\n",
//				bounding_box[0], bounding_box[1], bounding_box[2], bounding_box[3]);
		}
		else if (!strncmp(line, "%%PageCustomColors:", 19))
		{
		}
		else if (!strncmp(line, "%%PageMedia:", 12))
		{
		}
		else if (!strncmp(line, "%%PageOrientation:", 18))
		{
		}
		else if (!strncmp(line, "%%PageProcessColors:", 20))
		{
		}
		else if (!strncmp(line, "%%PageRequirements:", 18))
		{
		}
		else if (!strncmp(line, "%%PageResources:", 16))
		{
		}
		else if (!strncmp(line, "%%IncludeFeature:", 17))
		{
		}
		else if (strncmp(line, "%%Include", 9))
			break;
	}

	// Copy any page setup commands...
	doc_puts(pdev, doc, "%%BeginPageSetup\n");

	// Copy page setup commands as needed...
	if (!strncmp(line, "%%BeginPageSetup", 16))
	{
	    int	feature = 0;			/* In a Begin/EndFeature block? */

		while ((linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize)) > 0)
		{
//			DebugPrintf("%s\n", line);
			if (!strncmp(line, "%%EndPageSetup", 14))
				break;
			else if (!strncmp(line, "%%BeginFeature:", 15))
			{
				feature = 1;
			}
			else if (!strncmp(line, "%%EndFeature", 12))
			{
				feature = 0;
			}
			else if (!strncmp(line, "%%PageBoundingBox:", 18))
			{
				continue;
			}
			else if (!strncmp(line, "%%Include", 9))
				continue;

			if (!feature)
				doc_write(pdev, doc, line, linelen);
		}

		// Skip %%EndPageSetup...
		if (linelen > 0)
			linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize);
	}

	// Finish the PageSetup section as needed...
	doc_puts(pdev, doc, "%%EndPageSetup\n");

	//  Read the rest of the page description...
	level = 0;

	do
	{
		if (level == 0 &&
			(!strncmp(line, "%%Page:", 7) ||
			!strncmp(line, "%%Trailer", 9) ||
			!strncmp(line, "%%EOF", 5)))
		{
			break;
		}
		else if (!strncmp(line, "%%BeginDocument", 15) ||
			!strncmp(line, "%ADO_BeginApplication", 21))
		{
			doc_write(pdev, doc, line, linelen);

			level ++;
		}
		else if ((!strncmp(line, "%%EndDocument", 13) ||
			!strncmp(line, "%ADO_EndApplication", 19)) && level > 0)
		{
			doc_write(pdev, doc, line, linelen);

			level --;
		}
		else if (!strncmp(line, "%%BeginBinary:", 14) ||
			(!strncmp(line, "%%BeginData:", 12) &&
			!strstr(line, "ASCII") && !strstr(line, "Hex")))
		{
			/*
			* Copy binary data...
			*/

			int	bytes;			/* Bytes of data */

			doc_write(pdev, doc, line, linelen);

			bytes = atoi(strchr(line, ':') + 1);

			while (bytes > 0)
			{
				if (bytes > linesize)
					linelen = pdev->lib_cups.cupsFileRead(pdev->fpPS, line, linesize);
				else
					linelen = pdev->lib_cups.cupsFileRead(pdev->fpPS, line, bytes);

				if (linelen < 1)
				{
					line[0] = '\0';
					Error_Log(LEVEL_ERROR, "Early end-of-file while reading binary data\n");
					exit(1);
				}
				doc_write(pdev, doc, line, linelen);

				bytes -= linelen;
			}
		}
		else
			doc_write(pdev, doc, line, linelen);
	}
	while ((linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize)) > 0);

	pageinfo->length = pdev->lib_cups.cupsFileTell(doc->temp) - pageinfo->offset;

	return (linelen);
}

ssize_t
copy_trailer(
	DEVDATA			*pdev,
	pstops_doc_t	*doc,
	char			*line,
	ssize_t			linelen,
	size_t			linesize
)
{
	gs_puts(pdev, "%%Trailer\n");
	while (linelen > 0)
	{
		if (!strncmp(line, "%%EOF", 5))
			break;
		else if (!strncmp(line, "%%Trailer", 9))
		{
		}
		else
			gs_write(pdev, line, linelen);

		linelen = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, linesize);
	}
	return (linelen);
}

void copy_bytes(
	DEVDATA			*pdev,
	FILE			*fp,
	off_t			offset,
	size_t			length
)
{
	char		buffer[8192];	/* Data buffer */
	ssize_t		nbytes;			/* Number of bytes read */
	size_t		nleft;			/* Number of bytes left/remaining */

	nleft = length;

//	DebugPrintf(_("seek to offset %ld in file\n"), (long)offset);
	if ( fseek(fp, offset, SEEK_SET) < 0)
	{
		Error_Log(LEVEL_ERROR, "Unable to seek to offset %ld in file - %s\n",
	        (long)offset, strerror(errno));
		return;
	}
//	DebugPrintf(_("current offset:%ld\n"), (long)pdev->lib_cups.cupsFileTell(fp));

	while (nleft > 0 || length == 0)
	{
		if (nleft > sizeof(buffer) || length == 0)
			nbytes = sizeof(buffer);
		else
			nbytes = nleft;

		if ((nbytes = fread(buffer, 1, nbytes, fp)) < 1)
			return;

		nleft -= nbytes;

		gs_write(pdev, buffer, nbytes);
	}
}

char * parse_text(
	const char	*start,
	char		**end,
	char		*buffer,
	size_t		bufsize
)
{
	char	*bufptr,			/* Pointer in buffer */
			*bufend;			/* End of buffer */
	int		level;				/* Parenthesis level */

	/*
	* Skip leading whitespace...
	*/

	while (isspace(*start & 255))
		start ++;

	/*
	* Then copy the value...
	*/

	level  = 0;
	bufptr = buffer;
	bufend = buffer + bufsize - 1;

	while (bufptr < bufend)
	{
		if (isspace(*start & 255) && !level)
			break;

		*bufptr++ = *start;

		if (*start == '(')
			level ++;
		else if (*start == ')')
		{
			if (!level)
			{
				start ++;
				break;
			}
			else
				level --;
		}
		else if (*start == '\\')
		{
			/*
			* Copy escaped character...
			*/

			int	i;			/* Looping var */


			for (i = 1;
			i <= 3 && isdigit(start[i] & 255) && bufptr < bufend;
			*bufptr++ = start[i], i ++);
		}

		start ++;
	}

	*bufptr = '\0';

	/*
	* Return the value and new pointer into the line...
	*/

	if (end)
		*end = (char *)start;

	if (bufptr == bufend)
		return (NULL);
	else
		return (buffer);
}

pstops_page_t *	add_page(
	DEVDATA			*pdev,
	pstops_doc_t	*doc,
	const char		*label
)
{
	pstops_page_t	*pageinfo;		/* New page info object */

	if (!doc->pages)
		doc->pages = pdev->lib_cups.cupsArrayNew(NULL, NULL);

	if (!doc->pages)
	{
		Error_Log(LEVEL_EMERG, "Unable to allocate memory for pages array: %s\n", strerror(errno));
		exit(1);
	}

	if ((pageinfo = MEMALLOC(sizeof(pstops_page_t))) == NULL)
	{
		Error_Log(LEVEL_EMERG, "Unable to allocate memory for pages info: %s\n", strerror(errno));
		exit(1);
	}

	pageinfo->label  = strdup(label);
	pageinfo->offset = pdev->lib_cups.cupsFileTell(doc->temp);

	pdev->lib_cups.cupsArrayAdd(doc->pages, pageinfo);

	doc->page ++;

	return (pageinfo);
}

void doc_printf(
	DEVDATA			*pdev,
	pstops_doc_t	*doc,
	const char		*format,
	...
)
{
	va_list		ap;				/* Pointer to arguments */
	char		buffer[1024];	/* Output buffer */
	size_t		bytes;			/* Number of bytes to write */

	va_start(ap, format);
	bytes = vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);

	if (bytes > sizeof(buffer))
	{
		Error_Log(LEVEL_ERROR, "doc_printf overflow (%d bytes) detected, aborting!", (int)bytes);
		exit(1);
	}

	doc_write(pdev, doc, buffer, bytes);
}

void doc_write(
	DEVDATA			*pdev,
	pstops_doc_t	*doc,
	const char		*s,
	size_t			len
)
{
	if (doc->temp)
		pdev->lib_cups.cupsFileWrite(doc->temp, s, len);
	else
		gs_write(pdev, s, len);
}
#endif	// #ifndef FILTER_NOT_PSTOPS
