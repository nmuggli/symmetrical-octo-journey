/*
 * "ps2bmp.c 2021-05-17 15:55:05
 *  
 *  ps to bmp routines for TSC Printer Driver
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
 *
 */

#include "config.h"
#include "common.h"
#include "debug.h"
#include "libloader.h"
#include "devmode.h"
#include "device.h"
#include "gsrun.h"

#define		GSDEVICE_BMP_MONO	"bmpmono"
#define		GSDEVICE_BMP_GRAY	"bmpgray"

static DEVDATA* DrvEnable(int argc, char *argv[]);
static void DrvDisable(DEVDATA *pdev);
static BOOL bInitCupsOptions(DEVDATA *pdev, char *argv[]);

int ps2bmp(int argc, char *argv[])
{
	int					iRtn = -1;
	DEVDATA				*pdev = NULL;
	DEVMODE				dm;
	int					fdIn;

	DebugPrintf("Enter ps2bmp\n");

	pdev = DrvEnable(argc, argv);

	iRtn = gsrun(pdev);
	
	DrvDisable(pdev);
	DebugPrintf("Leave ps2bmp, return %d\n", iRtn);

	return iRtn;
}

DEVDATA* DrvEnable(int argc, char *argv[])
{
	DEVDATA*	pdev = NULL;

	DebugPrintf("\n#ENTER:DrvEnable\n");

	// Make sure we have the right number of arguments for CUPS!
	if (argc < 6 || argc > 7)
	{
		Error_Log(LEVEL_ERROR, "Usage: %s job user title copies options [filename]\n", argv[0]);
		return NULL;
	}

	pdev = MEMALLOC(sizeof(DEVDATA));
	if ( pdev )
	{
		memset(pdev, 0, sizeof(DEVDATA));

		pdev->gsdevice = GSDEVICE_BMP_MONO;

		// Load CUPS lib
		if ( LoadCupsLibrary(&pdev->lib_cups) )
		{
			Error_Log(LEVEL_ERROR, "Cannot load libcups or libcups version too old then 1.1.19\n");
			DrvDisable(pdev);
			pdev = NULL;
		}
		// Load GS lib
		if ( pdev && LoadGsLibrary(&pdev->lib_gs) )
		{
			Error_Log(LEVEL_ERROR, "Cannot load libgs or libgs version too old then 8.0\n");
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

		if ( pdev )
		{
			// Open Input Postscript File Stream
			if (argc == 6)
			{
				if ( (pdev->fpPS = pdev->lib_cups.cupsFileStdin()) == NULL )
				{
					Error_Log(LEVEL_ERROR, "Unable to open spool file\n", strerror(errno));
					DrvDisable(pdev);
					pdev = NULL;
				}
			}
			else
			{
				if ((pdev->fpPS = pdev->lib_cups.cupsFileOpen(argv[6], "r")) == NULL)
				{
					Error_Log(LEVEL_ERROR, "Unable to open file \"%s\" - %s\n", argv[6], strerror(errno));
					DrvDisable(pdev);
					pdev = NULL;
				}
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
		if ( pdev->fpPS && pdev->lib_cups.cupsFileClose)
		{
			pdev->lib_cups.cupsFileClose(pdev->fpPS);
		}
		if ( pdev->ppd && pdev->lib_cups.ppdClose )
		{
			pdev->lib_cups.ppdClose(pdev->ppd);
		}
		if ( pdev->options && pdev->lib_cups.cupsFreeOptions )
		{
			pdev->lib_cups.cupsFreeOptions(pdev->num_options, pdev->options);
		}

		FreeCupsLibrary(&pdev->lib_cups);
		FreeGsLibrary(&pdev->lib_gs);

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
		ppd_file_t		*ppd;
	
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

				pagesize = pdev->lib_cups.ppdPageSize(pdev->ppd, NULL);
				
				pdev->dm.dmFields |= DM_COPIES;
				pdev->dm.dmCopies = atoi(argv[4]);
				if ( pdev->dm.dmCopies == 0 )
					pdev->dm.dmCopies = 1;

				if ( pagesize )
				{
					pdev->dm.dmPaperWidth  = pagesize->width;
					pdev->dm.dmPaperLength = pagesize->length;
					pdev->dm.dmFields |= DM_PAPERLENGTH | DM_PAPERWIDTH;
				}
				else
				{
					bRtn = FALSE;
				}
			}
		}
	}

	return bRtn;
}
