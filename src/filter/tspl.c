/*
 * "tspl.c 2021-05-17 15:55:05
 *  
 *  tsc filter routines for TSC Printer Driver
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
#include "devmode.h"
#include "device.h"
#include <stdarg.h>

#define	DRAWMODE_COPY			0
#define	DRAWMODE_OR				1
#define	DRAWMODE_XOR			2

#define DIRECTION_RIGHT_BOTTOM	0
#define DIRECTION_LEFT_TOP		1


#define	TSPL_SET_TEAR				"SET TEAR %s\r\n"
#define	TSPL_SET_PEEL				"SET PEEL %s\r\n"
#define	TSPL_SET_CUTTER				"SET CUTTER %s\r\n"
#define	TSPL_SET_PARTIAL_CUTTER		"SET PARTIAL_CUTTER %s\r\n"

static size_t printer_write(const void* pbuf, size_t cbbuf);
static size_t printer_puts(const char* str);
static int printer_printf(const char* strfmt, ...);

static int TSPL_SendBitmap1bpp(DEVMODE *pdm, BITMAPINFOHEADER* pBih, void* pBits);
static int TSPL_SendUserCommand(DEVMODE *pdm, DWORD dwField);

int TSPL_SendJobStart(DEVMODE *pdm)
{
	// Set User Command - Start Job
	TSPL_SendUserCommand(pdm, DM_CMDSTARTJOB);

	// Set Lable Size
	if ( pdm->dmMetric == DMMETRIC_INCH )
		printer_printf("SIZE %.3f,%.3f\r\n", POINT2INCH(pdm->dmPaperWidth), POINT2INCH(pdm->dmPaperLength));
	else
		printer_printf("SIZE %.1f mm,%.1f mm\r\n", POINT2MM(pdm->dmPaperWidth), POINT2MM(pdm->dmPaperLength));

	// Set Gap
	switch ( pdm->dmMediaType )
	{
	case DMMEDIATYPE_GAPS:			// Label with Gaps
		if ( pdm->dmMetric == DMMETRIC_INCH )
			printer_printf("GAP %.3f,%.3f\r\n", POINT2INCH(pdm->dmGapHeight), POINT2INCH(pdm->dmGapOffset));
		else
			printer_printf("GAP %.1f mm,%.1f mm\r\n", POINT2MM(pdm->dmGapHeight), POINT2MM(pdm->dmGapOffset));
		break;
	case DMMEDIATYPE_MARK:			// Label with Mark
		if ( pdm->dmMetric == DMMETRIC_INCH )
			printer_printf("BLINE %.3f,%.3f\r\n", POINT2INCH(pdm->dmGapHeight), POINT2INCH(pdm->dmGapOffset));
		else
			printer_printf("BLINE %.1f mm,%.1f mm\r\n", POINT2MM(pdm->dmGapHeight), POINT2MM(pdm->dmGapOffset));
		break;
	case DMMEDIATYPE_CONTINUE:		// Continue
		printer_printf("GAP 0,0\r\n");
		break;
	}

	// Set Speed
	if ( pdm->dmFields & DM_PRINTSPEED )
	{
		if ( pdm->dmPrintSpeed % 10 )
			printer_printf("SPEED %d.%d\r\n", pdm->dmPrintSpeed / 10, pdm->dmPrintSpeed % 10);
		else
			printer_printf("SPEED %d\r\n", pdm->dmPrintSpeed / 10);
	}

	// Set Density
	if ( pdm->dmFields & DM_DARKNESS )
		printer_printf("DENSITY %d\r\n", pdm->dmDarkness);

// [0002] Li Add Start
	// Set Ribbon
	switch ( pdm->dmMediaMethod )
	{
	case DMMEDIAMETHOD_DIRECT:
		printer_puts("SET RIBBON OFF\r\n");
		break;
	case DMMEDIAMETHOD_TRANSFER:
		printer_puts("SET RIBBON ON\r\n");
		break;
	}
// [0002] Li Add End

	// Set Direction
	{
		int		n = DIRECTION_LEFT_TOP;
		int		m = DMMIRRORIMAGE_OFF;
		
		if ( pdm->dmFields & DM_MIRRORIMAGE )
			m = pdm->dmMirrorImage;

		printer_printf("DIRECTION %d,%d\r\n", n, m);
	}

	// Set Reference
	printer_printf("REFERENCE %.0f,%.0f\r\n",
					POINT2DOT((double)pdm->dmAdjustHorizontal, pdm->dmPrintQuality),
					POINT2DOT((double)pdm->dmAdjustVertical, pdm->dmYResolution));

	// Set Offset
	if ( pdm->dmMetric == DMMETRIC_INCH )
		printer_printf("OFFSET %.3f\r\n", POINT2INCH(pdm->dmFeedOffset));
	else
		printer_printf("OFFSET %.1f mm\r\n", POINT2MM(pdm->dmFeedOffset));

	// Set Shift
	printer_printf("SHIFT %.0f\r\n", POINT2DOT((double)pdm->dmVerticalOffset, pdm->dmYResolution));

	// Set Action;
#if 1
	{
		char	szON[] = "ON";
		char	szOFF[] = "OFF";
		char	szNumber[16] = "1";

		const char	*szTear		= szOFF;
		const char	*szPeel		= szOFF;
		const char	*szCut		= szOFF;
		const char	*szPartCut	= szOFF;

		switch ( pdm->dmOccurrence )
		{
		case DMOCCURRENCE_EVERY:		// After Every Page
			break;
		case DMOCCURRENCE_COPIES:		// After Identical Copies
			sprintf(szNumber, "%d", pdm->dmCopies);
			break;
		case DMOCCURRENCE_JOB:			// After Job
			sprintf(szNumber, "%d", pdm->dmCopies * pdm->dmDocPages);
			break;
		case DMOCCURRENCE_SPECIFIED:	// After Specified interval
			sprintf(szNumber, "%d", pdm->dmCutInterval);
			break;
		}

		switch ( pdm->dmPostAction )
		{
		case DMPOSTACTION_NONE:			// None
			break;
		case DMPOSTACTION_TEAROFF:		// Tear Off
			szTear = szON;
			break;
		case DMPOSTACTION_PEELOFF:		// Peel Off
			szPeel = szON;
			break;
		case DMPOSTACTION_CUT:			// Cut
			szCut = szNumber;
			break;
		case DMPOSTACTION_PARTIAL:		// Partial Cut
			szPartCut = szNumber;
			break;
		}
// [0002] Li Delete Start
//		printer_printf(TSPL_SET_TEAR, szTear);
//		printer_printf(TSPL_SET_PEEL, szPeel);
//		printer_printf(TSPL_SET_CUTTER, szCut);
//		printer_printf(TSPL_SET_PARTIAL_CUTTER, szPartCut);
// [0002] Li Delete End
// [0002] Li Add Start
		{
			const char	*szCmds[][2] = {
				{TSPL_SET_TEAR, szTear},
				{TSPL_SET_PEEL, szPeel},
				{TSPL_SET_CUTTER, szCut},
				{TSPL_SET_PARTIAL_CUTTER, szPartCut},
			};
			int			i;
			for ( i=0; i<ARRAYCOUNT(szCmds); i++)
			{
				if ( szCmds[i][1] == szOFF )
				{
					printer_printf(szCmds[i][0], szCmds[i][1]);
				}
			}
			for ( i=0; i<ARRAYCOUNT(szCmds); i++)
			{
				if ( szCmds[i][1] != szOFF )
				{
					printer_printf(szCmds[i][0], szCmds[i][1]);
				}
			}
		}
// [0002] Li Add End
	}

#else

	switch ( pdm->dmPostAction )
	{
	case DMPOSTACTION_NONE:			// None
		printer_puts("SET TEAR OFF\r\n");
		printer_puts("SET PEEL OFF\r\n");
		printer_puts("SET CUTTER OFF\r\n");
		printer_puts("SET PARTIAL_CUTTER OFF\r\n");
		break;
	case DMPOSTACTION_TEAROFF:		// Tear Off
		printer_puts("SET TEAR ON\r\n");
		printer_puts("SET PEEL OFF\r\n");
		printer_puts("SET CUTTER OFF\r\n");
		printer_puts("SET PARTIAL_CUTTER OFF\r\n");
		break;
	case DMPOSTACTION_PEELOFF:		// Peel Off
		printer_puts("SET TEAR OFF\r\n");
		printer_puts("SET PEEL ON\r\n");
		printer_puts("SET CUTTER OFF\r\n");
		printer_puts("SET PARTIAL_CUTTER OFF\r\n");
		break;
	case DMPOSTACTION_CUT:			// Cut
		switch ( pdm->dmOccurrence )
		{
		case DMOCCURRENCE_EVERY:		// After Every Page
			printer_puts("SET TEAR OFF\r\n");
			printer_puts("SET PEEL OFF\r\n");
			printer_puts("SET CUTTER 1\r\n");
			printer_puts("SET PARTIAL_CUTTER OFF\r\n");
			break;
		case DMOCCURRENCE_COPIES:		// After Identical Copies
			printer_puts("SET TEAR OFF\r\n");
			printer_puts("SET PEEL OFF\r\n");
			printer_printf("SET CUTTER %d\r\n", pdm->dmCopies);
			printer_puts("SET PARTIAL_CUTTER OFF\r\n");
			break;
		case DMOCCURRENCE_JOB:			// After Job
			printer_puts("SET TEAR OFF\r\n");
			printer_puts("SET PEEL OFF\r\n");
			printer_printf("SET CUTTER %d\r\n", pdm->dmCopies * pdm->dmDocPages);
			printer_puts("SET PARTIAL_CUTTER OFF\r\n");
			break;
		case DMOCCURRENCE_SPECIFIED:	// After Specified interval
			printer_puts("SET TEAR OFF\r\n");
			printer_puts("SET PEEL OFF\r\n");
			printer_printf("SET CUTTER %d\r\n", pdm->dmCutInterval);
			printer_puts("SET PARTIAL_CUTTER OFF\r\n");
			break;
		}
		break;
	case DMPOSTACTION_PARTIAL:		// Partial Cut
		switch ( pdm->dmOccurrence )
		{
		case DMOCCURRENCE_EVERY:		// After Every Page
			printer_puts("SET TEAR OFF\r\n");
			printer_puts("SET PEEL OFF\r\n");
			printer_puts("SET CUTTER OFF\r\n");
			printer_puts("SET PARTIAL_CUTTER 1\r\n");
			break;
		case DMOCCURRENCE_COPIES:		// After Identical Copies
			printer_puts("SET TEAR OFF\r\n");
			printer_puts("SET PEEL OFF\r\n");
			printer_puts("SET CUTTER OFF\r\n");
			printer_printf("SET PARTIAL_CUTTER %d\r\n", pdm->dmCopies);
			break;
		case DMOCCURRENCE_JOB:			// After Job
			printer_puts("SET TEAR OFF\r\n");
			printer_puts("SET PEEL OFF\r\n");
			printer_puts("SET CUTTER OFF\r\n");
			printer_printf("SET PARTIAL_CUTTER %d\r\n", pdm->dmCopies * pdm->dmDocPages);
			break;
		case DMOCCURRENCE_SPECIFIED:	// After Specified interval
			printer_puts("SET TEAR OFF\r\n");
			printer_puts("SET PEEL OFF\r\n");
			printer_puts("SET CUTTER OFF\r\n");
			printer_printf("SET PARTIAL_CUTTER %d\r\n", pdm->dmCutInterval);
			break;
		}
		break;
	}
#endif

// [0002] Li Delete Start
//	// Set Ribbon
//	switch ( pdm->dmMediaMethod )
//	{
//	case DMMEDIAMETHOD_DIRECT:
//		printer_puts("SET RIBBON OFF\r\n");
//		break;
//	case DMMEDIAMETHOD_TRANSFER:
//		printer_puts("SET RIBBON ON\r\n");
//		break;
//	}
// [0002] Li Delete End
}

int TSPL_SendJobEnd(DEVMODE *pdm)
{
	// Set User Command - End Job
	TSPL_SendUserCommand(pdm, DM_CMDENDJOB);
}

int TSPL_SendPageStart(DEVMODE *pdm)
{
	// Cls
	printer_puts("CLS\r\n");

	// Set User Command - Start Label
	TSPL_SendUserCommand(pdm, DM_CMDSTARTLABEL);
}

int TSPL_SendPageEnd(DEVMODE *pdm)
{
	// REVERSE
	if( (pdm->dmFields & DM_NEGATIVEIMAGE) && (pdm->dmNegativeImage != DMNEGATIVEIMAGE_OFF))
	{
		printer_printf("REVERSE 0,0,%.0f,%.0f\r\n",
						POINT2DOT(pdm->dmPaperWidth, pdm->dmPrintQuality),
						POINT2DOT(pdm->dmPaperLength, pdm->dmYResolution));
	}

	// PRINT
	printer_printf("PRINT %d,%d\r\n", 1, pdm->dmCollate ? 1 : pdm->dmCopies);
	
	// Set User Command - End Label
	TSPL_SendUserCommand(pdm, DM_CMDENDLABEL);
}

int TSPL_SendPage(DEVMODE *pdm, BITMAPINFOHEADER* pBih, RGBQUAD *pColorTable, void* pBits)
{
	DebugPrintf("Enter TSPL_SendPage\n");

	if ( pdm->dmOutPages == 0 )
	{
		TSPL_SendJobStart(pdm);
	}
	pdm->dmOutPages ++;
	TSPL_SendPageStart(pdm);

	switch( pBih->biBitCount )
	{
	case 1:
		TSPL_SendBitmap1bpp(pdm, pBih, pBits);
		break;
	case 8:
		break;
	}

	TSPL_SendPageEnd(pdm);

	return 1;
}

int TSPL_SendBitmap1bpp(DEVMODE *pdm, BITMAPINFOHEADER* pBih, void* pBits)
{
	int		ix = 0;									// x-coordinate
	int		iy = 0;									// y-coordinate
	int		iWidth = WIDTHBYTES_8(pBih->biWidth);	// The width of the image in bytes
	int		iHeight = pBih->biHeight;				// The height of the image in dot
	int		x, y;
	DWORD	cbWidthBytes = WIDTHBYTES_32(pBih->biWidth);
	BYTE*	pBitsLine;

	pBitsLine = MEMALLOC(iWidth);
	if ( pBitsLine )
	{
		printer_printf("BITMAP %d,%d,%d,%d,%d,", ix, iy, iWidth, iHeight, DRAWMODE_OR);
	
		for(y=0; y<pBih->biHeight; y++)
		{
			memcpy(pBitsLine, pBits + cbWidthBytes * y, iWidth);
			for (x=0; x<iWidth; x++)
				pBitsLine[x] = ~pBitsLine[x];
			printer_write(pBitsLine, iWidth);
		}
		printer_printf("\r\n");
		MEMFREE(pBitsLine);
	}
	return 1;
}

int TSPL_SendUserCommand(DEVMODE *pdm, DWORD dwField)
{
	WORD	wLength = 0;
	LPBYTE	pCmdDat = NULL;

	if ( pdm->dmFields & dwField )
	{
		switch (dwField)
		{
		case DM_CMDSTARTJOB:
			pCmdDat = pdm->dmCmdStartJob;
			wLength = pdm->dmCmdStartJobLength;
			break;
		case DM_CMDSTARTLABEL:
			pCmdDat = pdm->dmCmdStartLable;
			wLength = pdm->dmCmdStartLabelLength;
			break;
		case DM_CMDENDLABEL:
			pCmdDat = pdm->dmCmdEndLable;
			wLength = pdm->dmCmdEndLabelLength;
			break;
		case DM_CMDENDJOB:
			pCmdDat = pdm->dmCmdEndJob;
			wLength = pdm->dmCmdEndJobLength;
			break;
		}
	}	

	if ( pCmdDat && wLength > 0 )
	{
		printer_write(pCmdDat, wLength);
	}
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

size_t printer_puts(const char* str)
{
	return printer_write(str, strlen(str));
}
