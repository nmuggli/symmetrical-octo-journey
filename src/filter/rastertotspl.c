/*
 * "rastertotspl.c 2021-05-17 15:55:05
 *  
 *  raster filter for TSC Printer Driver
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

#include "cupsinc/cups.h"
#include "cupsinc/ppd.h"
//#include "cupsinc/string.h"
#include "raster.h"
//#include <stdlib.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <signal.h>

#define	DRAWMODE_COPY			0
#define	DRAWMODE_OR				1
#define	DRAWMODE_XOR			2

typedef struct _pageinfo_t
{
	unsigned		width;				/* Width of page image in pixels */
	unsigned		height;				/* Height of page image in pixels */
	off_t			offset;				/* Offset to start of page */
	ssize_t			length;				/* Number of bytes for page */
}	pageinfo_t;

typedef struct _doc_t
{
	char			tempfile[1024];			/* Temporary filename */
	FILE			*fp_temp;				/* Temporary file for read, if any */

	cups_array_t	*pages;					/* Pages in document */

}	doc_t;


static DEVDATA* DrvEnable(int argc, char *argv[]);
static void DrvDisable(DEVDATA *pdev);
static BOOL bInitCupsOptions(DEVDATA *pdev, char *argv[]);
static int ParseDocData(DEVDATA *pdev, int fd, doc_t *doc);
static void FreeDocData(DEVDATA *pdev, doc_t *doc);
size_t printer_write(const void* pbuf, size_t cbbuf);
int printer_printf(const char* strfmt, ...);

int TSPL_SendJobStart(DEVMODE *pdm);
int TSPL_SendJobEnd(DEVMODE *pdm);
int TSPL_SendPageStart(DEVMODE *pdm);
int TSPL_SendPageEnd(DEVMODE *pdm);

int
main(int  argc, char *argv[])
{
	int					fd;	/* File descriptor */
//	cups_raster_t		*ras;	/* Raster stream for printing */
//	cups_page_header_t	header;	/* Page header from file */
	doc_t				doc;
//	ppd_file_t			*ppd;	/* PPD file */
	int					page;	/* Current page */
	int					copies;	/* Current copies */
	DEVDATA				*pdev = NULL;

//	DebugPrintf("#ENTER:rastertobarcodetspl\n");
	Error_Log(LEVEL_DEBUG, "### Start rastertobarcodetspl ###\n");

	/*
	* Make sure status messages are not buffered...
	*/

	setbuf(stderr, NULL);

	/*
	* Check command-line...
	*/

	if (argc < 6 || argc > 7)
	{
		/*
		* We don't have the correct number of arguments; write an error message
		* and return.
		*/

		fputs("ERROR: rastertoepson job-id user title copies options [file]\n", stderr);
		return (1);
	}

	/*
	* Open the page stream...
	*/

	if (argc == 7)
	{
		if ((fd = open(argv[6], O_RDONLY)) == -1)
		{
			perror("ERROR: Unable to open raster file - ");
			sleep(1);
			return (1);
		}
	}
	else
		fd = 0;

	pdev = DrvEnable(argc, argv);
	if ( pdev == NULL )
	{
		sleep(1);
		return (1);
	}

	memset(&doc, 0, sizeof(doc));
	// Process pages as needed...
	if ( ParseDocData(pdev, fd, &doc) )
	{
		Error_Log(LEVEL_ERROR, "Raster Data Error.\n");
		DrvDisable(pdev);
		return (1);
	}


	if ( CheckTrialTime() )
	{
		Error_Log(LEVEL_ERROR, "Trial Version, Time end.\n");
		printer_printf("\r\n");
		DrvDisable(pdev);
		return (1);
	}	

	TSPL_SendJobStart(&pdev->dm);

	for ( copies=0; copies<(pdev->dm.dmCollate ? pdev->dm.dmCopies : 1); copies++ )
	{
		DebugPrintf("--copies %d --\n", copies);
		for(page=0; page<pdev->dm.dmDocPages; page++)
		{
			pageinfo_t	*pageinfo = (pageinfo_t*)pdev->lib_cups.cupsArrayIndex(doc.pages, page);

			if ( pageinfo )
			{
				unsigned char	*PlaneData;

				DebugPrintf("PAGE: %d\n", page + 1);
				DebugPrintf("pageinfo->offset=%d, pageinfo->length=%d\n", pageinfo->offset, pageinfo->length);

				PlaneData = MEMALLOC(pageinfo->length);
				if ( fseek(doc.fp_temp, pageinfo->offset, SEEK_SET) >= 0)
				{
					if ( fread(PlaneData, 1, pageinfo->length, doc.fp_temp) == pageinfo->length )
					{
						DebugPrintf("PAGE START\n");
						TSPL_SendPageStart(&pdev->dm);

						printer_printf("BITMAP %d,%d,%d,%d,%d,", 0, 0, WIDTHBYTES_8(pageinfo->width), pageinfo->height, DRAWMODE_OR);
						printer_write(PlaneData, pageinfo->length);
						printer_printf("\r\n");

						DebugPrintf("PAGE END\n");
						TSPL_SendPageEnd(&pdev->dm);
					}
				}
				MEMFREE(PlaneData);
			}
		}
	}

	TSPL_SendJobEnd(&pdev->dm);

	FreeDocData(pdev, &doc);

	DrvDisable(pdev);

	if (fd != 0)
		close(fd);

//	DebugPrintf("#LEAVE:rastertobarcodetspl\n");
	Error_Log(LEVEL_DEBUG, "### End rastertobarcodetspl ###\n");
	return (page == 0);
}

int ParseDocData(DEVDATA *pdev, int fd, doc_t *doc)
{
	int					ret = 0;
	cups_file_t			*temp;			/* Temporary file, if any */
	cups_raster_t		*ras;			/* Raster stream for printing */
	cups_page_header_t	header;			/* Page header from file */
	unsigned			NumCopies = 0;	/* Number of copies to produce */
	cups_bool_t			Collate = 0;

    if ((temp = pdev->lib_cups.cupsTempFile2(doc->tempfile, sizeof(doc->tempfile))) == NULL)
    {
		Error_Log(LEVEL_ERROR, "Unable to create temporary file: %s\n", strerror(errno));
		return (1);
	}

	doc->pages = pdev->lib_cups.cupsArrayNew(NULL, NULL);

	ras = cupsRasterOpen(fd, CUPS_RASTER_READ);
	DebugPrintf("ras->sync: %x\n", *(unsigned*)ras);
	while (ret ==0 && cupsRasterReadHeader(ras, &header))
	{
		int					y;	/* Current line */
		unsigned char		*RowData;
		unsigned char		*PlaneData;
		unsigned			WidthBytes;
		int					nOutWidth;
		int					nOutHeight;
		pageinfo_t			*pageinfo;

		DebugPrintf("PAGE: %d\n", pdev->lib_cups.cupsArrayCount(doc->pages) + 1);
		DebugPrintf("NumCopies=%d\n", header.NumCopies);
		DebugPrintf("PageSize(%dx%d) HWResolution(%dx%d)\n", header.PageSize[0], header.PageSize[1], header.HWResolution[0], header.HWResolution[1]);
		DebugPrintf("Margins(%dx%d)\n", header.Margins[0], header.Margins[1]);
		DebugPrintf("ImagingBoundingBox(%d, %d, %d, %d)\n", header.ImagingBoundingBox[0], header.ImagingBoundingBox[1], header.ImagingBoundingBox[2], header.ImagingBoundingBox[3]);
		DebugPrintf("cupsWidth=%d, cupsHeight=%d\n", header.cupsWidth, header.cupsHeight);
		DebugPrintf("cupsBitsPerPixel=%d, cupsBytesPerLine=%d\n", header.cupsBitsPerPixel, header.cupsBytesPerLine);
		DebugPrintf("cupsRowCount=%d, cupsRowFeed=%d, cupsRowStep=%d\n", header.cupsRowCount, header.cupsRowFeed, header.cupsRowStep);

		nOutWidth  = header.cupsWidth;
		nOutHeight = header.cupsHeight;

		// Check Page Size
		if ( header.PageSize[0] > pdev->ppd->custom_max[0] || header.PageSize[1] > pdev->ppd->custom_max[1] )
		{
			// Page Size is too big
			ppd_size_t		*pagesize = NULL;
			ppd_option_t	*option;

			DebugPrintf("Custom Max: %.3fx%.3f\n", pdev->ppd->custom_max[0], pdev->ppd->custom_max[1]);
			DebugPrintf("Page size is too big\n");
			if ( (option = pdev->lib_cups.ppdFindOption(pdev->ppd, "PageSize")) != NULL )
			{
				DebugPrintf("DefaultPageSize: '%s'\n", option->defchoice);
				if ( option->num_choices > 0 )
				{
					int		i;
					
					for (i=0; i<option->num_choices; i++)
					{
						if ( strcasecmp(option->choices[i].choice, "Custom") )
						{
							DebugPrintf("Get PageSize: '%s'\n", option->choices[i].choice);
							if ( (pagesize = pdev->lib_cups.ppdPageSize(pdev->ppd, option->choices[i].choice)) != NULL )
							{
								DebugPrintf("PageSize: '%s' -> %.3fx%.3f (point)\n", pagesize->name, pagesize->width, pagesize->length);
								
								pdev->dm.dmPaperWidth  = pagesize->width;
								pdev->dm.dmPaperLength = pagesize->length;
								pdev->dm.dmFields |= DM_PAPERLENGTH | DM_PAPERWIDTH;

								nOutWidth  = (int)(pagesize->width  * header.HWResolution[0] / 72 + 0.5);
								nOutHeight = (int)(pagesize->length * header.HWResolution[1] / 72 + 0.5);

								DebugPrintf("Change Out PageSize to %dx%d (pixel)\n", nOutWidth, nOutHeight);
								break;
							}
						}
					}
				}
			}
		}

		if ( ! (pdev->dm.dmFields & (DM_PAPERLENGTH | DM_PAPERWIDTH)) )
		{
			pdev->dm.dmPaperWidth  = header.PageSize[0];
			pdev->dm.dmPaperLength = header.PageSize[1];
			pdev->dm.dmFields |= DM_PAPERLENGTH | DM_PAPERWIDTH;
		}
		if ( !NumCopies )
		{
			NumCopies = header.NumCopies;
			Collate = header.Collate;
		}

		WidthBytes = min(WIDTHBYTES_8(nOutWidth), header.cupsBytesPerLine);
		DebugPrintf("WidthBytes=%d\n", WidthBytes);
		pageinfo = MEMALLOC(sizeof(pageinfo_t));
		if ( pageinfo )
		{
			pageinfo->width  = nOutWidth;
			pageinfo->height = nOutHeight;
			pageinfo->offset = pdev->lib_cups.cupsFileTell(temp);

			RowData = MEMALLOC(header.cupsBytesPerLine);
			PlaneData = MEMALLOC(WidthBytes * nOutHeight);

			if ( RowData && PlaneData )
			{
				for (y = 0; y < header.cupsHeight; y ++)
				{
//					DebugPrintf("cupsRasterReadPixels Line %d\n", y);
					if (cupsRasterReadPixels(ras, RowData, header.cupsBytesPerLine) < 1)
					{
						DebugPrintf("ERROR: cupsRasterReadPixels\n");
						ret = 1;
						break;
					}
//					memmove(PlaneData + WidthBytes * (header.cupsHeight-y-1), RowData, WidthBytes);
					if (y < nOutHeight )
						memmove(PlaneData + WidthBytes * y, RowData, WidthBytes);
				}
				
				//

				for(y=0; y<WidthBytes * nOutHeight; y++)
					PlaneData[y] = ~PlaneData[y];

				pdev->lib_cups.cupsFileWrite(temp, PlaneData, WidthBytes * nOutHeight);
				pageinfo->length = pdev->lib_cups.cupsFileTell(temp) - pageinfo->offset;
				if ( pageinfo->length != WidthBytes * nOutHeight )
				{
					Error_Log(LEVEL_ERROR, "IO error: %s\n", strerror(errno));
					ret = 1;
				}
				MEMFREE(RowData);
				MEMFREE(PlaneData);
			}
			else
			{
				DebugPrintf("No memory: %s\n", strerror(errno));
				ret = 1;
			}
			pdev->lib_cups.cupsArrayAdd(doc->pages, pageinfo);
		}
		else
		{
			DebugPrintf("No memory: %s\n", strerror(errno));
			Error_Log(LEVEL_ERROR, "No memory: %s\n", strerror(errno));
			ret = 1;
		}
	}

	// Close the raster stream...
	cupsRasterClose(ras);

	pdev->lib_cups.cupsFileClose(temp);
	doc->fp_temp = fopen(doc->tempfile, "r");

	if ( NumCopies )
		pdev->dm.dmCopies = NumCopies;
	pdev->dm.dmDocPages = pdev->lib_cups.cupsArrayCount(doc->pages);
	pdev->dm.dmCollate = pdev->dm.dmDocPages > 1 ? Collate : 0;

	DebugPrintf("pdev->dm.dmDocPages=%d\n", pdev->dm.dmDocPages);
	DebugPrintf("pdev->dm.dmCopies=%d\n", pdev->dm.dmCopies);
	DebugPrintf("pdev->dm.dmCollate=%d\n", pdev->dm.dmCollate);
	DebugPrintf("LEAVE ParseDocData %d\n", ret);
	return ret;
}

void FreeDocData(DEVDATA *pdev, doc_t *doc)
{
	if ( doc->fp_temp )
		fclose(doc->fp_temp);
	if ( doc->tempfile[0] )
		unlink(doc->tempfile);
}

DEVDATA* DrvEnable(int argc, char *argv[])
{
	DEVDATA*	pdev = NULL;

	DebugPrintf("\n#ENTER:DrvEnable\n");

	pdev = MEMALLOC(sizeof(DEVDATA));
	if ( pdev )
	{
		memset(pdev, 0, sizeof(DEVDATA));

		// Load CUPS lib
		if ( LoadCupsLibrary(&pdev->lib_cups) )
		{
			Error_Log(LEVEL_ERROR, "Cannot load libcups or libcups version too older then 1.1.19\n");
			DrvDisable(pdev);
			pdev = NULL;
		}
		// Get Printer Name
		if ( pdev )
		{
			pdev->szPrinterName = strdup(getenv("PRINTER"));
			if ( pdev->szPrinterName == NULL )
			{
				Error_Log(LEVEL_ERROR, "no printer name\n", argv[6], strerror(errno));
				DrvDisable(pdev);
				pdev = NULL;
			}
		}

		if ( pdev && (!bInitCupsOptions(pdev, argv)) )
		{
			Error_Log(LEVEL_ERROR, "ERROR in InitCupsOptions\n");
			DrvDisable(pdev);
			pdev = NULL;
		}
	}

#ifdef _DEBUG
	if ( pdev )
	{
		DebugPrintf("\nDrvEnable Success, pdev=%p\n", pdev);
		DebugPrintf("\tPrinterName : '%s'\n", pdev->szPrinterName);
		DumpDevmode(&pdev->dm);
	}
	else
	{
		DebugPrintf("\nDrvEnable Error!\n", pdev);
	}
#endif	// #ifdef _DEBUG

	return pdev;
}

void DrvDisable(DEVDATA *pdev)
{
	DebugPrintf("\n#ENTER:DrvDisable(pdev=%p)\n", pdev);
	if ( pdev )
	{
		if ( pdev->ppd && pdev->lib_cups.ppdClose )
		{
			pdev->lib_cups.ppdClose(pdev->ppd);
		}
		if ( pdev->options && pdev->lib_cups.cupsFreeOptions )
		{
			pdev->lib_cups.cupsFreeOptions(pdev->num_options, pdev->options);
		}

		FreeCupsLibrary(&pdev->lib_cups);

		MEMFREE(pdev->szPrinterName);
	}
	MEMFREE(pdev);
}

static BOOL bInitCupsOptions(DEVDATA *pdev, char *argv[])
{
	BOOL				bRtn = FALSE;
	int					i;
//	struct passwd		*pwd;
//	uid_t				euid;

	if ( pdev == NULL || pdev->szPrinterName == NULL )
		return bRtn;

//	DumpRESUID();
//
//	euid = geteuid();
//	pwd = getpwnam(argv[2]);
//	if ( pwd )
//	{
//		if ( seteuid(pwd->pw_uid) )
//			DebugPrintf("set effect user '%s' error: %s\n", argv[2], strerror(errno));
//		else
//			DebugPrintf("set effect user '%s'\n", argv[2]);
//	}
//	DumpRESUID();

	pdev->num_options = 0;
	pdev->options = NULL;

	// Get options from arg
	pdev->num_options = pdev->lib_cups.cupsParseOptions(argv[5], pdev->num_options, &pdev->options);
	// replace copies setting
	pdev->num_options = pdev->lib_cups.cupsAddOption("copies" ,argv[4], pdev->num_options, &pdev->options);

	// If not have Collate , Collate = false
	// Is need ? When app set collate off, collate option is not exist
	if (pdev->lib_cups.cupsGetOption("Collate", pdev->num_options, pdev->options) == NULL)
	{
		pdev->num_options = pdev->lib_cups.cupsAddOption("Collate", "false", pdev->num_options, &pdev->options);
	}

	DebugPrintf("=== ARG OPTIONS ===\n");
	for(i=0; i<pdev->num_options; i++)
	{
		DebugPrintf("\t%s=%s\n", pdev->options[i].name, pdev->options[i].value);
	}

	{
		//////////////////////////////////
		// Get options from CUPS setting file
		int					num_options;		// CUPS option count
		cups_option_t		*options;			// CUPS options

		num_options = GetPrinterOptions(pdev->szPrinterName, &options, &pdev->lib_cups);
		for(i=0; i<num_options; i++)
		{
			if ( strcmp(options[i].name, "orientation-requested")
				&& strcmp(options[i].name, "landscape")
				&& strcmp(options[i].name, "sides")
			)
			{
				if (pdev->lib_cups.cupsGetOption(options[i].name, pdev->num_options, pdev->options) == NULL)
				{
					pdev->num_options = pdev->lib_cups.cupsAddOption(options[i].name, options[i].value, pdev->num_options, &pdev->options);
				}
			}
		}
		pdev->lib_cups.cupsFreeOptions(num_options, options);
	}

//	DebugPrintf("Resort effect user\n");
//	seteuid(euid);
//	DumpRESUID();

	DebugPrintf("=== OPTIONS ===\n");
	for(i=0; i<pdev->num_options; i++)
	{
		DebugPrintf("\t%s=%s\n", pdev->options[i].name, pdev->options[i].value);
	}

	{
		const char		*filename;
//		ppd_file_t		*ppd;

		// Create temp PPD file by printer name
		if ( (filename = pdev->lib_cups.cupsGetPPD(pdev->szPrinterName)) != NULL )
		{
	//		DebugPrintf("the ppd of printers is %s.\n", filename);

			// Open the PPD file
			pdev->ppd = pdev->lib_cups.ppdOpenFile(filename);

			// Delete temp PPD file
			unlink(filename);

			if ( pdev->ppd )
			{
				ppd_size_t		*pagesize = NULL;

				pdev->lib_cups.ppdMarkDefaults(pdev->ppd);
				// Mark PPD options by printer options
				pdev->lib_cups.cupsMarkOptions(pdev->ppd, pdev->num_options, pdev->options);

				bRtn = SetDevmodeFromOptions(&pdev->lib_cups, pdev->ppd, &pdev->dm, pdev->num_options, pdev->options);

//				pagesize = pdev->lib_cups.ppdPageSize(pdev->ppd, NULL);
//				if ( !pagesize )
//				{
//					for (i = 0; i < pdev->ppd->num_sizes; i ++)
//						if (pdev->ppd->sizes[i].marked)
//						{
//							pagesize = (pdev->ppd->sizes + i);
//							break;
//						}
//				}
//				DebugPrintf("pagesize = %p.\n", pagesize);

				pdev->dm.dmFields |= DM_COPIES;
				pdev->dm.dmCopies = atoi(argv[4]);
				if ( pdev->dm.dmCopies == 0 )
					pdev->dm.dmCopies = 1;

//				if ( pagesize )
//				{
//					pdev->dm.dmPaperWidth  = pagesize->width;
//					pdev->dm.dmPaperLength = pagesize->length;
//					pdev->dm.dmFields |= DM_PAPERLENGTH | DM_PAPERWIDTH;
//				}
//				else
//				{
//					Error_Log(LEVEL_ERROR, "Paper size is NOT set.\n");
//					bRtn = FALSE;
//				}
			}
		}
	}

	if ( pdev->dm.dmCopies == 0 )
		pdev->dm.dmCopies = 1;

	if ( pdev->dm.dmPrintQuality == 0 )
		pdev->dm.dmPrintQuality = DPI_300;
	if ( pdev->dm.dmFields & DM_YRESOLUTION )
	{
		if ( pdev->dm.dmYResolution == 0 )
			pdev->dm.dmYResolution = DPI_300;
	}
	else
	{
		pdev->dm.dmYResolution = pdev->dm.dmPrintQuality ;
	}
	pdev->dm.dmOutPages = 0;

	return bRtn;
}

size_t printer_write(const void* pbuf, size_t cbbuf)
{
//	DebugPrintf("printer_write %d bytes\n", cbbuf);
	return write(fileno(stdout), pbuf, cbbuf);
}

int printer_printf(const char* strfmt, ...)
{
	int		iRtn = -1;
	int		size = 128;
	va_list	args;
	char	*p;

	// Guess we need no more than 128 bytes.
	while ((p = MEMALLOC(size)) != NULL)
	{
		// Try to print in the allocated space.
		va_start(args, strfmt);
		iRtn = vsnprintf(p, size, strfmt, args);
		va_end(args);

		// If that worked, break;
		if (iRtn > -1 && iRtn < size)
		{
			break;
		}
		
		// Else try again with more space. */
		if (iRtn > -1)		// glibc 2.1
			size = iRtn+1;	// precisely what is needed
		else				// glibc 2.0
			size *= 2;		// twice the old size

		MEMFREE(p);
	}

	if ( p )
	{
		iRtn = printer_write(p, iRtn);
		MEMFREE(p);
	}

	return iRtn;
}
