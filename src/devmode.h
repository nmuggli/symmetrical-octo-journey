/*
 * "devmode.h 2021-05-17 15:55:05
 *  
 *  ppd option routine declaration for TSC Printer Driver
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
 *  limitations under the License.*  	
 *
 */


#ifndef _DEVMODE_H_
#define _DEVMODE_H_
#include "config.h" 
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include "libloader.h"
#include "devoption.h"

#define DM_HEADER_MARKER   ((WORD) ('M' << 8) | 'D')

#define DM_USER_COMMOND_LENGTH			1024

typedef struct {
	WORD	dmType;					// DM_HEADER_MARKER
	WORD	dmSize;					// sizeof(DEVMODE)
	WORD	dmSizeExtra;			//
	DWORD	dmFields;

//
	WORD	dmOrientation;
	WORD	dmPaperSize;
	float	dmPaperLength;			// Point (1/72inch)
	float	dmPaperWidth;			// Point (1/72inch)

	WORD	dmMirrorImage;
	WORD	dmNegativeImage;
	WORD	dmMediaMethod;
	WORD	dmMediaType;

	float	dmGapHeight;			// Point (1/72inch)
	float	dmGapOffset;			// Point (1/72inch)
	WORD	dmPostAction;
	WORD	dmOccurrence;
	WORD	dmCutInterval;

	float	dmFeedOffset;			// Point (1/72inch)
	float	dmVerticalOffset;		// Point (1/72inch)

	WORD	dmPrintSpeed;			// 1/10 inch/sec
	WORD	dmDarkness;				// 0 - 15

	WORD	dmDirectBuffer;
	WORD	dmStoredGriphics;

	float	dmAdjustHorizontal;		// Point (1/72inch)
	float	dmAdjustVertical;		// Point (1/72inch)

	WORD	dmCmdStartJobLength;		// Bytes Length of User Command StartJob
	WORD	dmCmdStartLabelLength;		// Bytes Length of User Command StartLabel
	WORD	dmCmdEndLabelLength;		// Bytes Length of User Command EndLabel
	WORD	dmCmdEndJobLength;			// Bytes Length of User Command EndJob
	BYTE	dmCmdStartJob[DM_USER_COMMOND_LENGTH];		//  User Command StartJob
	BYTE	dmCmdStartLable[DM_USER_COMMOND_LENGTH];	//  User Command StartJob
	BYTE	dmCmdEndLable[DM_USER_COMMOND_LENGTH];		//  User Command StartJob
	BYTE	dmCmdEndJob[DM_USER_COMMOND_LENGTH];		//  User Command StartJob
	CHAR	dmCmdFuncChar;

	BYTE	dmCmdOriStartJob[DM_USER_COMMOND_LENGTH];		//  User Command StartJob
	BYTE	dmCmdOriStartLable[DM_USER_COMMOND_LENGTH];		//  User Command StartJob
	BYTE	dmCmdOriEndLable[DM_USER_COMMOND_LENGTH];		//  User Command StartJob
	BYTE	dmCmdOriEndJob[DM_USER_COMMOND_LENGTH];			//  User Command StartJob

	WORD	dmMetric;
	WORD	dmPrintQuality;
	WORD	dmXResolution;
	WORD	dmYResolution;
	WORD	dmCopies;

	// Use When Print
	WORD	dmDocPages;
	WORD	dmOutPages;
	WORD	dmCollate;

} DEVMODE;

// dmFields
#define DM_ORIENTATION      	0x00000001L
#define DM_PAPERLENGTH      	0x00000002L
#define DM_PAPERWIDTH       	0x00000004L
#define	DM_MIRRORIMAGE			0x00000008L
#define	DM_NEGATIVEIMAGE		0x00000010L
#define	DM_MEDIAMETHOD			0x00000020L
#define	DM_MEDIATYPE			0x00000040L
#define	DM_GAPHEIGHT			0x00000080L
#define	DM_GAPOFFSET			0x00000100L
#define	DM_POSTACTION			0x00000200L
#define	DM_OCCURRENCE			0x00000400L
#define	DM_CUTINTERVAL			0x00000800L
#define	DM_FEEDOFFSET			0x00001000L
#define	DM_VERTICALOFFSET		0x00002000L
#define	DM_PRINTSPEED			0x00004000L
#define	DM_DARKNESS				0x00008000L
#define	DM_DIRECTBUFFER			0x00010000L
#define	DM_STOREDGRIPHICS		0x00020000L
#define	DM_ADJUSTHORIZONTAL		0x00040000L
#define	DM_ADJUSTVERTICAL		0x00080000L
#define	DM_CMDSTARTJOB			0x00100000L
#define	DM_CMDSTARTLABEL		0x00200000L
#define	DM_CMDENDLABEL			0x00400000L
#define	DM_CMDENDJOB			0x00800000L
#define	DM_CMDFUNCCHAR			0x01000000L
#define	DM_METRIC				0x02000000L
#define	DM_PRINTQUALITY			0x04000000L
#define	DM_YRESOLUTION			0x08000000L
#define	DM_COPIES				0x10000000L
#define DM_PAPERSIZE	      	0x20000000L

// dmOrientation
#define DMORIENT_PORTRAIT			1
#define DMORIENT_LANDSCAPE			2
#define DMORIENT_PORTRAIT_180		3
#define DMORIENT_LANDSCAPE_180		4

// dmMirrorImage
#define DMMIRRORIMAGE_OFF			0
#define DMMIRRORIMAGE_ON			1

// dmNegativeImage;
#define DMNEGATIVEIMAGE_OFF			0
#define DMNEGATIVEIMAGE_ON			1

// dmMediaMethod
#define DMMEDIAMETHOD_NORMAL		0		// Use Currently Printer Setting
#define DMMEDIAMETHOD_DIRECT		1		// Direct Thermal
#define DMMEDIAMETHOD_TRANSFER		2		// Thermal Transfer

// dmMediaType
#define DMMEDIATYPE_GAPS			0		// Label with Gaps
#define DMMEDIATYPE_MARK			1		// Label with Mark
#define DMMEDIATYPE_CONTINUE		2		// Continue

// dmPostAction
#define DMPOSTACTION_NONE			0		// None
#define DMPOSTACTION_TEAROFF		1		// Tear Off
#define DMPOSTACTION_PEELOFF		2		// Peel Off
#define DMPOSTACTION_CUT			3		// Cut
#define DMPOSTACTION_PARTIAL		4		// Partial Cut

// dmOccurrence
#define DMOCCURRENCE_EVERY			0		// After Every Page
#define DMOCCURRENCE_COPIES			1		// After Identical Copies
#define DMOCCURRENCE_JOB			2		// After Job
#define DMOCCURRENCE_SPECIFIED		3		// After Specified interval

// dmDirectBuffer
#define DMDIRECTBUFFER_AUTO			0		// Automatic
#define DMDIRECTBUFFER_8BIT			1		// Uncompressed 8-bit
#define DMDIRECTBUFFER_REL			2		// RLE Compression
#define DMDIRECTBUFFER_DISABLE		3		// Disable

// dmStoredGriphics
#define DMSTOREDGRIPHICS_AUTO		0		// Automatic
#define DMSTOREDGRIPHICS_PCX		1		// PCX

// dmMetric
#define DMMETRIC_INCH				0		// inch
#define DMMETRIC_MM					1		// mm

#define TSC_LANG_ZH_CN				"zh_CN"
#define TSC_LANG_ZH_TW				"zh_TW"
#define TSC_LANG_EN					"en"

#define TSC_LANG_CN_STRING_NUM				5
#define TSC_LANG_EN_STRING_NUM				2

#define	DMBOOL_TRUE					"True"
#define	DMBOOL_FALSE				"False"

// dmPrintQuality
#define DPI_203						203
#define DPI_300						300
#define DPI_600						600

#define STR_MEDIAMETHOD_NORMAL		"Normal"
#define STR_MEDIAMETHOD_DIRECT		"Direct"
#define STR_MEDIAMETHOD_TRANSFER	"Transfer"

#define STR_MEDIATYPE_LBLGAPS		"LabelGaps"
#define STR_MEDIATYPE_LBLMARK		"LabelMark"
#define STR_MEDIATYPE_CONTINUE		"Continue"

#define STR_POSTACTION_NONE			"None"
#define STR_POSTACTION_TEAROFF		"TearOff"
#define STR_POSTACTION_PEELOFF		"PeelOff"
#define STR_POSTACTION_CUT			"Cut"
#define STR_POSTACTION_PARTIALCUT	"PartialCut"

#define	STR_OCCURRENCE_EVERY		"Every"
#define	STR_OCCURRENCE_COPIES		"Copies"
#define	STR_OCCURRENCE_JOB			"Job"
#define	STR_OCCURRENCE_SPECIFIED	"Specified"

#define STR_RESOLUTION_203DPI		"203dpi"
#define STR_RESOLUTION_300DPI		"300dpi"
#define STR_RESOLUTION_600DPI		"600dpi"


// dmDirectBuffer
#define STR_DIRECTBUFFER_AUTO		"Auto"
#define STR_DIRECTBUFFER_8BIT		"8bit"
#define STR_DIRECTBUFFER_REL		"Rel"
#define STR_DIRECTBUFFER_DISABLE	"Disable"

// dmStoredGriphics
#define STR_STOREDGRIPHICS_AUTO		"Auto"
#define STR_STOREDGRIPHICS_PCX		"Pcx"

// dmMetric
#define STR_PPDMETRIC_AUTO		"AUTO"
#define STR_PPDMETRIC_MM		"MM"
#define STR_PPDMETRIC_INCH		"INCH"

// range of dmGapHeight value
#define GAPHEIGHT_MAX_VALUE			1		//(in)
#define GAPHEIGHT_MIN_VALUE			0

// range of dmFeedOffset value
#define FEEDOFFSET_MAX_VALUE		1		//(in)
#define FEEDOFFSET_MIN_VALUE		-1

// range of dmFeedOffset value
#define VEROFFSET_MAX_VALUE			1		//(in)
#define VEROFFSET_MIN_VALUE			-1

extern float			g_fCurPaperSizeHeight;
extern float			g_fCurPaperSizeWidth;


BOOL get_option_name_byID(char* szOptionName, int opID);
int	GetPrinterOptions(const char* szPrnName, cups_option_t **options, CUPSLIB_FUNCTION *cups);
int GetItemIndexByValue(int opID, char* szOpValue);
int GetOptionIndexByChoice(CUPSLIB_FUNCTION* cups, ppd_file_t* ppd, int opID, char* szOpKey);
BOOL GetOptionCodeByChoice(CUPSLIB_FUNCTION* cups, ppd_file_t* ppd, int opID, char* szOpKey, char* szOpCode);
int GetOptionIDByName(char* szName);
const TSC_OPTION_NAME_T* GetOptionTypeByName(char* szName);
BOOL SetDevmodeFromOptions(CUPSLIB_FUNCTION* cups, ppd_file_t* ppd, DEVMODE* devMode, int num_options, cups_option_t *options);
BOOL SetDevmodeFromOption(CUPSLIB_FUNCTION* cups, ppd_file_t* ppd, DEVMODE* devMode, int opID, char* szOpValue);

int GetPrinterOptionsCommand(cups_option_t **options, CUPSLIB_FUNCTION *cups, ppd_file_t *ppd, char *OptionKey, int *num);

float OnValidValue(float fSrcValue, float fMin, float fMax);
WORD SetMetricString(char* szSysLang, CUPSLIB_FUNCTION* cups);

#endif	// #ifndef _DEVMODE_H_
