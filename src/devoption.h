/*
 * "devoption.h 2021-05-17 15:55:05
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
 *  limitations under the License.
 *
 */
#ifndef _DEVOPTION_H
#define _DEVOPTION_H

#include "common.h"

#define	PPD_TSC_ATTR			"TscAttr"
#define	PPD_ATTR_SPCFILE		"SPCFILE"
#define	PPD_ATTR_JBIGLEVER		"JBIGLever"
#define	PPD_TSCATTR_UTILITY		"TscUtility"
//#define	PPD_TSCATTR_UTILITY		"tscUtility"


#define PPD_TSC_ATTRDATA		"TscAttrData"
#define PPD_TSC_ATTRDATA_OPT	"Options"

// Page Setup
#define	OPTID_PAGESETUPNAME						1		// PPD Name
#define	OPTID_PAGEPORTRAIT						2		// PPD Orientation---Portrait
#define	OPTID_PAGELANDSCAPE						3		// PPD Orientation---Landscape
#define	OPTID_PAGEPORTRAITRESERVE				4		// PPD Orientation---Portrait Reserve
#define	OPTID_PAGELANDSCAPERESERVE				5		// PPD Orientation---Landscape Reserve
#define	OPTID_PAGEMIRRORIMAGE					6		// PPD Mirror Image
#define	OPTID_PAGENEGATIVEIMAGE					7		// PPD Negative Image
#define OPTID_PAGORIENTATION					8

#define	OPTID_PAGESETUPPAGEREGION				9		// PPD PageRegion

// Stock
#define	OPTID_STOCKMETHOD						101		// PPD Method
#define	OPTID_STOCKTYPE							102		// PPD Type
#define	OPTID_STOCKMARKHEIGHT					103		// PPD Mark Height
#define	OPTID_STOCKMARKOFFSET					104		// PPD Mark Offset
#define	OPTID_STOCKPOSTSCRIPTACTION				105		// PPD Mark Post Script Action
#define	OPTID_STOCKOCCURRENCE					106		// PPD Occurrence
#define	OPTID_STOCKINTERVAL						107		// PPD Interval
#define	OPTID_STOCKFEEDOFFSET					108		// PPD Feed Offset
#define	OPTID_STOCKVERTICALOFFSET				109		// PPD Vertical Offset

// Option
#define	OPTID_OPTIONPRINTERSPEED				201		// PPD Printer Speed
#define	OPTID_OPTIONDARKNESS					202		// PPD Darkness
#define	OPTID_OPTIONDIRECTTOBUFFER				203		// PPD Direct to Buffer
#define	OPTID_OPTIONSTOREDGRAPHICS				204		// PPD Store Graphics
#define	OPTID_OPTIONPRINTQUALITY				205		// PPD Print Quality
#define	OPTID_OPTIONXRESOLUTION					206		// PPD X Resolution
#define	OPTID_OPTIONYRESOLUTION					207		// PPD Y Resolution

// Printing Postion
#define	OPTID_PRNPOSHORIZONTALOFFSET			301		// PPD Horiaontal Offset
#define	OPTID_PRNPOSVERTICALOFFSET				302		// PPD Vertical Offset

// User Command
//#define	OPTID_USERCMD						401		// PPD User Command
#define	OPTID_USERCMDCOMMENT					402		// PPD User Command Comment
#define	OPTID_USERCMDFUNCHARACTER				403		// PPD User Command Fun Character
#define	OPTID_LENUSERCMDSTARTJOB				404
#define	OPTID_USERCMDSTARTJOB					405
#define OPTID_LENUSERCMDSTARTLABEL				406
#define OPTID_USERCMDSTARTLABEL					407
#define OPTID_LENUSERCMDENDLABEL				408
#define OPTID_USERCMDENDLABEL					409
#define OPTID_LENUSERCMDENDJOB					410
#define OPTID_USERCMDENDJOB						411

#define	OPTID_USERCMDSTARTJOBNOCTRL				450
#define OPTID_USERCMDSTARTLABELNOCTRL			451
#define OPTID_USERCMDENDLABELNOCTRL				452
#define OPTID_USERCMDENDJOBNOCTRL				453

// Stock Edit/New
#define	OPTID_EDITSTOCKNAME						501
#define	OPTID_EDITSTOCKWIDTH					502
#define	OPTID_EDITSTOCKHEIGHT					503
#define	OPTID_EDITSTOCKLINERL					504
#define OPTID_EDITSTOCKLINERR					505

#define OPTID_METRIC							601


typedef struct {
	DWORD	id;
	WORD	flag;
	char*	name;
} TSC_OPTION_NAME_T;

//#ifdef _DEF_GLOBAL_DEVOPTION_
static TSC_OPTION_NAME_T g_tsc_options[] = {
		// Page Setup
		{OPTID_PAGESETUPNAME, 						0,	"PageSize"},
		{OPTID_PAGEMIRRORIMAGE,						0,	"MirrorImage"},
		{OPTID_PAGENEGATIVEIMAGE,					0,	"NegativeImage"},
		{OPTID_PAGORIENTATION,						0,	"Orientation"},

		// Stock
		{OPTID_STOCKMETHOD,	 						0,	"MediaMethod"},				// PPD Method
		{OPTID_STOCKTYPE, 							0,	"PaperType"},				// PPD Type
		{OPTID_STOCKMARKHEIGHT,						0,	"MarkHeight"},				// PPD Mark Height
		{OPTID_STOCKMARKOFFSET,						0,	"MarkOffset"},				// PPD Mark Offset
		{OPTID_STOCKPOSTSCRIPTACTION,				0,	"PostAction"},				// PPD Mark Post Script Action
		{OPTID_STOCKOCCURRENCE,						0,	"Occurrence"},				// PPD Occurrence
		{OPTID_STOCKINTERVAL, 						0,	"Interval"},				// PPD Interval
		{OPTID_STOCKFEEDOFFSET,						0,	"FeedOffset"},				// PPD Feed Offset
		{OPTID_STOCKVERTICALOFFSET,					0,	"VerticalOffset"},			// PPD Vertical Offset

		// Option
		{OPTID_OPTIONPRINTERSPEED, 					0,	"PrintSpeed"},				// PPD Printer Speed
		{OPTID_OPTIONDARKNESS, 						0,	"Darkness"},				// PPD Darkness
		{OPTID_OPTIONDIRECTTOBUFFER, 				0,	"DirectBuffer"},			// PPD Direct to Buffer
		{OPTID_OPTIONSTOREDGRAPHICS, 				0,	"StoredGraphics"},			// PPD Store Graphics
		{OPTID_OPTIONPRINTQUALITY, 					0,	"Resolution"},				// PPD Print Quality
		{OPTID_OPTIONXRESOLUTION,					0,	"XResolution"},
		{OPTID_OPTIONYRESOLUTION,					0,	"YResolution"},

		// Printing Postion
		{OPTID_PRNPOSHORIZONTALOFFSET,				0,	"AdjustHoriaontal"},		// PPD Horiaontal Offset
		{OPTID_PRNPOSVERTICALOFFSET, 				0,	"AdjustVertical"},			// PPD Vertical Offset

		// User Command
		{OPTID_USERCMDFUNCHARACTER,					0,	"FunctionCharacter"},		// PPD User Command Fun Character
		{OPTID_USERCMDSTARTJOB,						0,	"StartJob"},
		{OPTID_USERCMDSTARTLABEL,					0,	"StartLabel"},
		{OPTID_USERCMDENDLABEL,						0,	"EndLabel"},
		{OPTID_USERCMDENDJOB,						0,	"EndJob"},
		{OPTID_LENUSERCMDSTARTJOB,					0,	"LenStartJob"},
		{OPTID_LENUSERCMDSTARTLABEL,				0,	"LenStartLabel"},
		{OPTID_LENUSERCMDENDLABEL,					0,	"LenEndLabel"},
		{OPTID_LENUSERCMDENDJOB,					0,	"LenEndJob"},

		{OPTID_USERCMDSTARTJOBNOCTRL,				0,	"OriStartJob"},
		{OPTID_USERCMDSTARTLABELNOCTRL,				0,	"OriStartLabel"},
		{OPTID_USERCMDENDLABELNOCTRL,				0,	"OriEndLabel"},
		{OPTID_USERCMDENDJOBNOCTRL,					0,	"OriEndJob"},
		{OPTID_METRIC,								0,  "OptionDisplayUnit"}

};

static int						g_tsc_options_num = sizeof(g_tsc_options)/sizeof(TSC_OPTION_NAME_T);

//#else

//extern TSC_OPTION_NAME_T		g_tsc_options[];
//extern int 						g_tsc_options_num;
//#endif


#endif /* _DEVOPTION_H */
