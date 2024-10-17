/*
 * "devmode.c 2021-05-17 15:55:05
 *  
 *  ppd option routine for TSC Printer Driver
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
 
#include "config.h" 
#include "common.h"
#include "debug.h"
#include "devmode.h"
#include "devoption.h"

float			g_fCurPaperSizeHeight = 0.0;
float			g_fCurPaperSizeWidth = 0.0;

BOOL get_option_name_byID(char* szOptionName, int opID)
{
	BOOL	bRtn = FALSE;
	int			i = 0;

//	DebugPrintf("\nBegin to Load option .\n");

	for(i=0;i<g_tsc_options_num;i++)
	{
		if ( g_tsc_options[i].id == opID )
		{
			if ( g_tsc_options[i].name )
			{
				strcpy(szOptionName, g_tsc_options[i].name);
				bRtn = TRUE;
			}			
		}
	}	

	return bRtn;
}

float OnValidValue(float fSrcValue, float fMin, float fMax)
{
	float		fRet = fSrcValue;

	if ( fSrcValue > fMax )
		fRet = fMax;
	else if ( fSrcValue < fMin )
		fRet = fMin;

	return fRet;
}

int GetPrinterOptions(const char* szPrnName, cups_option_t **options, CUPSLIB_FUNCTION *cups)
{
	int					num = 0;

/*
	int					num_dests = 0;
	cups_dest_t			*dests;
	cups_dest_t			*dest;
	int					i;

	*options = NULL;
	num_dests = cups->cupsGetDests(&dests);
	if ( (dest = cups->cupsGetDest(szPrnName, NULL, num_dests, dests)) != NULL )
	{
		for ( i=0; i<dest->num_options; i++)
		{
			num = cups->cupsAddOption(dest->options[i].name, dest->options[i].value, num, options);
			DebugPrintf("dest->options[%d].name=%s, value=%s.\n", i, dest->options[i].name, dest->options[i].value);
		}
	}

	cups->cupsFreeDests(num_dests, dests);
*/

	*options = NULL;
	{
		ppd_file_t		*ppd = NULL;
		const char		*filename;

		// Create temp PPD file by printer name
		if ( (filename = cups->cupsGetPPD(szPrnName)) != NULL )
		{
			// Open the PPD file
			ppd = cups->ppdOpenFile(filename);
			cups->ppdLocalize(ppd);
			cups->ppdMarkDefaults(ppd);

			// Delete temp PPD file
			unlink(filename);
		}
		if ( ppd )
		{
			ppd_option_t	*option;
			char			strTmp[PPD_MAX_TEXT];
			char*			strValue = NULL;
			ppd_attr_t*		attr;
	
			for (attr = cups->ppdFindAttr(ppd, PPD_TSC_ATTRDATA, PPD_TSC_ATTRDATA_OPT);
			   attr;
			   attr = cups->ppdFindNextAttr(ppd, PPD_TSC_ATTRDATA, PPD_TSC_ATTRDATA_OPT))
			{
//				if ( !strcasecmp(attr->value, "options") )
				{
					strcpy(strTmp, attr->value);
					strValue = strstr(strTmp, "=");
					if ( strValue != NULL )
					{
						*strValue = 0;
						strValue++;

						num = cups->cupsAddOption(strTmp, strValue, num, options);
					}
				}
			}

			int i;
			for (i = 0; i < g_tsc_options_num; i++ )
			{
				switch ( g_tsc_options[i].id ) {
				case OPTID_USERCMDSTARTJOB:
				case OPTID_USERCMDSTARTLABEL:
				case OPTID_USERCMDENDLABEL:
				case OPTID_USERCMDENDJOB:
				case OPTID_USERCMDSTARTJOBNOCTRL:
				case OPTID_USERCMDSTARTLABELNOCTRL:
				case OPTID_USERCMDENDLABELNOCTRL:
				case OPTID_USERCMDENDJOBNOCTRL:
					GetPrinterOptionsCommand(options, cups, ppd, g_tsc_options[i].name, &num);
					break;
				}
			}

			cups->ppdClose(ppd);		
		}		
	}

	return num;
}


int GetPrinterOptionsCommand(cups_option_t **options, CUPSLIB_FUNCTION *cups, ppd_file_t *ppd, char *OptionKey, int *num)
{
	if ( ppd )
	{
		ppd_option_t	*option;
		char			strTmp[PPD_MAX_TEXT];
		char			*strValue;
		char			strCmdData[8192];
		ppd_attr_t*		attr;
		char			strKey[32];
		int				i = 0;

		strCmdData[0] = 0;

		sprintf(strKey, "%s%d", OptionKey, i);
		for (attr = cups->ppdFindAttr(ppd, PPD_TSC_ATTRDATA, strKey);
		   attr;
		   attr = cups->ppdFindNextAttr(ppd, PPD_TSC_ATTRDATA, strKey))
		{
			strcpy(strTmp, attr->value);
			strValue = strstr(strTmp, "=");
			if ( strValue != NULL )
			{
				*strValue = 0;
				strValue++;
				strcat(strCmdData, strValue);
			}

			sprintf(strKey, "%s%d", OptionKey, ++i);
		}

		if ( strCmdData[0] != 0 )
			*num = cups->cupsAddOption(OptionKey, strCmdData, *num, options);
	}

	return (*num);
}

int 
GetOptionIndexByChoice(CUPSLIB_FUNCTION* cups, ppd_file_t* ppd, int opID, char* szOpKey)
{	
	int				index = -1;
	char 			szOpName[128];
	ppd_option_t	*option;

	if ( szOpKey == NULL )
		return index;

	if ( get_option_name_byID(szOpName, opID) )
	{
		if ( (option = cups->ppdFindOption(ppd, szOpName)) != NULL )
		{
			int		i;
			int		j = 0;

			for(i=0;i<option->num_choices;i++)
			{
				if ( opID == OPTID_PAGESETUPNAME )
				{
					ppd_size_t		*pagesize;

					if ( strcmp(option->choices[i].choice, "Custom") )
					{
						if ( !strcmp(option->choices[i].choice, szOpKey) )
							index = j;

						pagesize = cups->ppdPageSize(ppd, szOpKey);
						g_fCurPaperSizeHeight = pagesize->length;
						g_fCurPaperSizeWidth = pagesize->width;

						j++;
					}
				}	
				else
				{
					if ( !strcmp(option->choices[i].choice, szOpKey) )
					{
						index = i;
					}
				}
			}
		}
	}

	return index;
}

BOOL 
GetOptionCodeByChoice(CUPSLIB_FUNCTION* cups, ppd_file_t* ppd, int opID, char* szOpKey, char* szOpCode)
{	
	int				bRet = FALSE;
	char 			szOpName[128];
	ppd_option_t	*option;

	if ( szOpKey == NULL )
		return bRet;

	if ( get_option_name_byID(szOpName, opID) )
	{
		if ( (option = cups->ppdFindOption(ppd, szOpName)) != NULL )
		{
			int		i;
			int		j = 0;

			for(i=0;i<option->num_choices;i++)
			{
				if ( opID == OPTID_PAGESETUPNAME )
				{
					ppd_size_t		*pagesize;

					if ( strcmp(option->choices[i].choice, "Custom") )
					{
						if ( !strcmp(option->choices[i].choice, szOpKey) )
							strcpy(szOpCode, option->choices[i].code);

						bRet = TRUE;
					}
				}	
				else
				{
					if ( !strcmp(option->choices[i].choice, szOpKey) )
					{
						strcpy(szOpCode, option->choices[i].code);
						
						bRet = TRUE;
					}
				}
			}
		}
	}

	return bRet;
}

int 
GetItemIndexByValue(int opID, char* szOpValue)
{
	int			sel = 0;

	if ( szOpValue == NULL )
		return sel;
	
	switch ( opID ) {
	case OPTID_STOCKMETHOD:
		{
			if ( !strcmp(szOpValue, STR_MEDIAMETHOD_NORMAL ) )
				sel = DMMEDIAMETHOD_NORMAL;
			else if ( !strcmp(szOpValue, STR_MEDIAMETHOD_DIRECT ) )
				sel = DMMEDIAMETHOD_DIRECT;
			else if ( !strcmp(szOpValue, STR_MEDIAMETHOD_TRANSFER ) )
				sel = DMMEDIAMETHOD_TRANSFER;
			else
				sel = DMMEDIAMETHOD_NORMAL;
		}
		break;
	case OPTID_STOCKTYPE:
		{
			if ( !strcmp(szOpValue, STR_MEDIATYPE_LBLGAPS ) )
				sel = DMMEDIATYPE_GAPS;
			else if ( !strcmp(szOpValue, STR_MEDIATYPE_LBLMARK ) )
				sel = DMMEDIATYPE_MARK;
			else if ( !strcmp(szOpValue, STR_MEDIATYPE_CONTINUE ) )
				sel = DMMEDIATYPE_CONTINUE;
			else
				sel = DMMEDIATYPE_GAPS;
		}
		break;
	case OPTID_STOCKPOSTSCRIPTACTION:
		{
			if ( !strcmp(szOpValue, STR_POSTACTION_NONE ) )
				sel = DMPOSTACTION_NONE;
			else if ( !strcmp(szOpValue, STR_POSTACTION_TEAROFF ) )
				sel = DMPOSTACTION_TEAROFF;
			else if ( !strcmp(szOpValue, STR_POSTACTION_PEELOFF ) )
				sel = DMPOSTACTION_PEELOFF;
			else if ( !strcmp(szOpValue, STR_POSTACTION_CUT ) )
				sel = DMPOSTACTION_CUT;
			else if ( !strcmp(szOpValue, STR_POSTACTION_PARTIALCUT ) )
				sel = DMPOSTACTION_PARTIAL;
			else
				sel = DMPOSTACTION_NONE;
		}
		break;
	case OPTID_STOCKOCCURRENCE:
		{
			if ( !strcmp(szOpValue, STR_OCCURRENCE_EVERY ) )
				sel = DMOCCURRENCE_EVERY;
			else if ( !strcmp(szOpValue, STR_OCCURRENCE_COPIES ) )
				sel = DMOCCURRENCE_COPIES;
			else if ( !strcmp(szOpValue, STR_OCCURRENCE_JOB ) )
				sel = DMOCCURRENCE_JOB;
			else if ( !strcmp(szOpValue, STR_OCCURRENCE_SPECIFIED ) )
				sel = DMOCCURRENCE_SPECIFIED;
			else
				sel = DMOCCURRENCE_EVERY;
		}
		break;
	case OPTID_OPTIONPRINTERSPEED:
		{
			sel = atoi(szOpValue);
		}
		break;
	case OPTID_OPTIONDIRECTTOBUFFER:
		{
			if ( !strcmp(szOpValue, STR_DIRECTBUFFER_AUTO ) )
				sel = DMDIRECTBUFFER_AUTO;
			else if ( !strcmp(szOpValue, STR_DIRECTBUFFER_8BIT ) )
				sel = DMDIRECTBUFFER_8BIT;
			else if ( !strcmp(szOpValue, STR_DIRECTBUFFER_REL ) )
				sel = DMDIRECTBUFFER_REL;
			else if ( !strcmp(szOpValue, STR_DIRECTBUFFER_DISABLE ) )
				sel = DMDIRECTBUFFER_DISABLE;
			else
				sel = DMDIRECTBUFFER_AUTO;
		}
		break;
	case OPTID_OPTIONSTOREDGRAPHICS:
		{
			if ( !strcmp(szOpValue, STR_STOREDGRIPHICS_AUTO ) )
				sel = DMSTOREDGRIPHICS_AUTO;
			else if ( !strcmp(szOpValue, STR_STOREDGRIPHICS_PCX ) )
				sel = DMSTOREDGRIPHICS_PCX;
			else
				sel = DMSTOREDGRIPHICS_AUTO;
		}
		break;
	case OPTID_OPTIONPRINTQUALITY:
		{
			if ( !strcmp(szOpValue, STR_RESOLUTION_203DPI ) )
				sel = DPI_203;
			else if ( !strcmp(szOpValue, STR_RESOLUTION_300DPI ) )
				sel = DPI_300;
			else if ( !strcmp(szOpValue, STR_RESOLUTION_600DPI ) )
				sel = DPI_600;
			else
				sel = DPI_203;
		}
		break;
	default:
		break;
	}

	return sel;
}

int GetOptionIDByName(char* szName)
{
	int							nID = 0;
	const TSC_OPTION_NAME_T 	*pOpt = NULL;

	pOpt = GetOptionTypeByName(szName);
	if ( pOpt )
	{
		nID = pOpt->id;
	}
	return nID;
}

const TSC_OPTION_NAME_T * GetOptionTypeByName(char* szName)
{
	const TSC_OPTION_NAME_T *pOpt = NULL;
	int						nID = 0;
	int						i;

	for (i=0; i<g_tsc_options_num; i++)
	{
		if ( !strcmp(g_tsc_options[i].name, szName))
		{
			pOpt = &g_tsc_options[i];
			break;
		}
	}

	return pOpt;
}

int bGetPpdGroupOptions(ppd_group_t *groups, int num_options, ppd_option_t ***options)
{
	int				nRtn = 0;
	ppd_option_t	**options_tmp;
	int				i;

	if ( options )
	{
		if ( *options == NULL )
		{
			num_options = 0;
		}

		options_tmp = MEMALLOC(sizeof(ppd_option_t*) * (num_options + groups->num_options));
		if ( options_tmp )
		{
			if ( num_options > 0 )
			{
				memcpy(options_tmp, *options, sizeof(ppd_option_t*) * num_options);
			}
			
			for (i=0; i< groups->num_options; i++)
			{
				if ( strcmp(groups->options[i].keyword, "PageRegion") )
					options_tmp[num_options++] = &groups->options[i];
			}

			MEMFREE(*options);
			*options = options_tmp;
		}

		for (i=0; i< groups->num_subgroups; i++)
		{
			num_options = bGetPpdGroupOptions(&groups->subgroups[i], num_options, options);
		}
	}
	else
	{
		num_options = 0;
	}
	
	return num_options;
}

int GetPpdOptions(ppd_file_t *ppd, ppd_option_t ***options)
{
	int		num_options = 0;
	int		i;
	
	if ( ppd && options )
	{
		*options = NULL;
	
		for (i=0; i< ppd->num_groups; i++)
		{
			num_options = bGetPpdGroupOptions(&ppd->groups[i], num_options, options);
		}
	}
	return num_options;
}

BOOL 
SetDevmodeFromOptions(CUPSLIB_FUNCTION* cups, ppd_file_t* ppd, DEVMODE* devMode, int num_options, cups_option_t *options)
{
	BOOL			bRet = TRUE;
	int				i;
	char			szBuf[128];
	int				num_ppdoptions;
	ppd_option_t	**ppdoption = NULL;
	ppd_choice_t	*ppdchoice;

	// Restore the default
	cups->ppdMarkDefaults(ppd);
	// Mark PPD options by printer options
	cups->cupsMarkOptions(ppd, num_options, options);

//	DebugPrintf("over to mark the devMode.\n");
//	memset(devMode, 0, sizeof(DEVMODE));
	
	devMode->dmGapHeight = 0.12*72;
	
	num_ppdoptions = GetPpdOptions(ppd, &ppdoption);
	for (i = 0; i < num_ppdoptions; i ++)
	{
		if ( (ppdchoice = cups->ppdFindMarkedChoice(ppd, ppdoption[i]->keyword)) != NULL )
//		if ( (ppdchoice = cups->ppdFindMarkedChoice(ppd, options[i].name)) != NULL )
		{
			int				nOptID;

			nOptID = GetOptionIDByName(ppdoption[i]->keyword);
//			nOptID = GetOptionIDByName(options[i].name);
			if ( nOptID )
			{
//				DebugPrintf("SET(PPD) %s=%s\n", ppdoption[i]->keyword, ppdchoice->choice);	
				bRet &= SetDevmodeFromOption(cups, ppd, devMode, nOptID, ppdchoice->choice);
			}
		}
	}

	for (i = 0; i < num_options; i ++)
	{
		if ( (ppdchoice = cups->ppdFindMarkedChoice(ppd, options[i].name)) == NULL )
		{
			const TSC_OPTION_NAME_T *	pID;
			pID = GetOptionTypeByName(options[i].name);
//			if ( pID && ((pID->flag & OPTION_FLG_NOSAVE) != OPTION_FLG_NOSAVE ) )
			if ( pID )
			{
//				DebugPrintf("SET(OPT) %s=%s\n", options[i].name, options[i].value);
				bRet &= SetDevmodeFromOption(cups, ppd, devMode, pID->id, options[i].value);
			}
		}
	}

	// Valid the value to check whether is on the range or not
	{
		float			fValueTmp;

		// dmGapOffset
		fValueTmp = devMode->dmGapHeight;
		devMode->dmGapHeight = OnValidValue(fValueTmp, GAPHEIGHT_MIN_VALUE*72, GAPHEIGHT_MAX_VALUE*72);

		// dmGapOffset
		fValueTmp = devMode->dmGapOffset;
		if ( devMode->dmMediaType == DMMEDIATYPE_GAPS )
		{
			devMode->dmGapOffset = OnValidValue(fValueTmp, ((-1)*g_fCurPaperSizeHeight), g_fCurPaperSizeHeight);
		}
		else if ( devMode->dmMediaType == DMMEDIATYPE_MARK )
		{
			devMode->dmGapOffset = OnValidValue(fValueTmp, 0, g_fCurPaperSizeHeight);
		}

		// dmFeedOffset
		fValueTmp = devMode->dmFeedOffset;
		devMode->dmFeedOffset = OnValidValue(fValueTmp, (FEEDOFFSET_MIN_VALUE)*72, FEEDOFFSET_MAX_VALUE*72);

		// dmVerOffset
		fValueTmp = devMode->dmVerticalOffset;
		devMode->dmVerticalOffset = OnValidValue(fValueTmp, (VEROFFSET_MIN_VALUE)*72, VEROFFSET_MAX_VALUE*72);

		// dmAdjustHorizontal
		fValueTmp = devMode->dmAdjustHorizontal;
		devMode->dmAdjustHorizontal = OnValidValue(fValueTmp, 0, g_fCurPaperSizeWidth);

		// dmAdjustHorizontal
		fValueTmp = devMode->dmAdjustVertical;
		devMode->dmAdjustVertical = OnValidValue(fValueTmp, 0, g_fCurPaperSizeHeight);
	}

	return bRet;
}

WORD   
SetMetricString(char* szSysLang, CUPSLIB_FUNCTION* cups)
{
	cups_lang_t*		lang;
	WORD				retMetric = DMMETRIC_INCH;

	lang = cups->cupsLangDefault();

	if (lang)
		sprintf(szSysLang, "%s", lang->language);

	if ( szSysLang )
	{
		if ( (!strncasecmp(szSysLang,TSC_LANG_ZH_CN,TSC_LANG_CN_STRING_NUM)) || 
			 (!strncasecmp(szSysLang,TSC_LANG_ZH_TW,TSC_LANG_CN_STRING_NUM)) )
		{
			retMetric = DMMETRIC_MM;
		}
		else if ( !strncasecmp(szSysLang,TSC_LANG_EN,TSC_LANG_EN_STRING_NUM) )
		{
			retMetric = DMMETRIC_INCH;
		}
		else
		{
			retMetric = DMMETRIC_INCH;
		}
	}
	else
	{
		retMetric = DMMETRIC_INCH;
	}

	cups->cupsLangFree(lang);

	return retMetric;
}

BOOL 
SetDevmodeFromOption(CUPSLIB_FUNCTION* cups, ppd_file_t* ppd, DEVMODE* devMode, int opID, char* szOpValue)
{
	BOOL			bRet = TRUE;
	float			fValue = 0.0;
	char 			szOpCmdValue[DM_USER_COMMOND_LENGTH];


	switch ( opID ) {
	case OPTID_PAGESETUPNAME:
		{
//			DebugPrintf("dmPaperSize_key=%s\n", szOpValue);
			if ( szOpValue )
				devMode->dmPaperSize = GetOptionIndexByChoice(cups, ppd, opID, szOpValue);
			else
				devMode->dmPaperSize = -1;

//			DebugPrintf("dmPaperSize=%d\n", devMode->dmPaperSize);
		}
		break;
	case OPTID_PAGEMIRRORIMAGE:
		{
//			DebugPrintf("show->dmMirrorImage=%s\n", szOpValue);
			devMode->dmFields |= DM_MIRRORIMAGE;

			if ( szOpValue == NULL )
			{
				devMode->dmMirrorImage = DMMIRRORIMAGE_OFF;
			}
			else
			{				
				if ( !strcmp(szOpValue, DMBOOL_FALSE) )
					devMode->dmMirrorImage = DMMIRRORIMAGE_OFF;
				else
					devMode->dmMirrorImage = DMMIRRORIMAGE_ON;
			}
//			DebugPrintf("show->dmMirrorImage=%d\n", devMode->dmMirrorImage);
		}
		break;
	case OPTID_PAGENEGATIVEIMAGE:
		{
//			DebugPrintf("show->dmNegativeImage=%s\n", szOpValue);
			devMode->dmFields |= DM_NEGATIVEIMAGE;

			if ( szOpValue == NULL )
			{
				devMode->dmNegativeImage = DMNEGATIVEIMAGE_OFF;
			}
			else
			{
				if ( !strcmp(szOpValue, DMBOOL_FALSE) )
					devMode->dmNegativeImage = DMNEGATIVEIMAGE_OFF;
				else
					devMode->dmNegativeImage = DMNEGATIVEIMAGE_ON;
			}
		}
		break;
	case OPTID_PAGORIENTATION:
		{
//			DebugPrintf("szOpValue->dmOrientation=%s\n", szOpValue);
			devMode->dmFields |= DM_ORIENTATION;

			if ( szOpValue == NULL )
			{
				devMode->dmOrientation = DMORIENT_PORTRAIT;
			}
			else
			{
				int 	index = atoi(szOpValue);

				if ( index == DMORIENT_PORTRAIT )
					devMode->dmOrientation = DMORIENT_PORTRAIT;
				else if ( index == DMORIENT_LANDSCAPE )
					devMode->dmOrientation = DMORIENT_LANDSCAPE;
				else if ( index == DMORIENT_PORTRAIT_180 )
					devMode->dmOrientation = DMORIENT_PORTRAIT_180;
				else if ( index == DMORIENT_LANDSCAPE_180 )
					devMode->dmOrientation = DMORIENT_LANDSCAPE_180;
				else 
					devMode->dmOrientation = DMORIENT_PORTRAIT;
				DebugPrintf("devMode->dmOrientation=%d\n", devMode->dmOrientation);
			}
		}
		break;
		// Stock
	case OPTID_STOCKMETHOD:
		{
//			devMode->dmMediaMethod = GetOptionIndexByChoice(cups, ppd, opID, szOpValue);
			devMode->dmMediaMethod = GetItemIndexByValue(opID, szOpValue);
			devMode->dmFields |= DM_MEDIAMETHOD;
//			DebugPrintf("show->dmMediaMethod=%d\n", devMode->dmMediaMethod);
		}
		break;
	case OPTID_STOCKTYPE:
		{
//			DebugPrintf("show->dmMediaType=%s\n", szOpValue);
//			devMode->dmMediaType = GetOptionIndexByChoice(cups, ppd, opID, szOpValue);
			devMode->dmFields |= DM_MEDIATYPE;
			devMode->dmMediaType = GetItemIndexByValue(opID, szOpValue);
		}
		break;
	case OPTID_STOCKMARKHEIGHT:
		{
			devMode->dmFields |= DM_GAPHEIGHT;

			if ( szOpValue == NULL )
			{
				devMode->dmGapHeight = 0.0; 
			}
			else
			{
				fValue = atof(szOpValue);
				devMode->dmGapHeight = fValue; 
			
///				DebugPrintf("opValue->dmGapHeight=%.5f\n", fValue);
//				if ( devMode->dmMetric == DMMETRIC_INCH )
//					devMode->dmGapHeight = POINT2INCH(fValue); 
//				else
//					devMode->dmGapHeight = POINT2MM(fValue); 
//				DebugPrintf("show->dmGapHeight=%.2f\n", devMode->dmGapHeight);
			}
		}
		break;
	case OPTID_STOCKMARKOFFSET:
		{
			devMode->dmFields |= DM_GAPOFFSET;

			if ( szOpValue == NULL )
			{
				devMode->dmGapOffset = 0.0; 
			}
			else
			{
				fValue = atof(szOpValue);
				devMode->dmGapOffset = fValue;
//				DebugPrintf("opValue->dmGapOffset=%.5f\n", fValue);

//				if ( devMode->dmMetric == DMMETRIC_INCH )
//					devMode->dmGapOffset = POINT2INCH(fValue);
//				else
//					devMode->dmGapOffset = POINT2MM(fValue); 
//				DebugPrintf("show->dmGapHeight=%.2f\n", devMode->dmGapHeight);
			}
		}
		break;
	case OPTID_STOCKPOSTSCRIPTACTION:
		{
//			DebugPrintf("show->dmPostAction=%s\n", szOpValue);
//			devMode->dmPostAction = GetOptionIndexByChoice(cups, ppd, opID, szOpValue);
			devMode->dmFields |= DM_POSTACTION;
			devMode->dmPostAction = GetItemIndexByValue(opID, szOpValue);
		}
		break;
	case OPTID_STOCKOCCURRENCE:
		{
//			DebugPrintf("show->dmOccurrence=%s\n", szOpValue);
//			devMode->dmOccurrence = GetOptionIndexByChoice(cups, ppd, opID, szOpValue);
			devMode->dmFields |= DM_OCCURRENCE;
			devMode->dmOccurrence = GetItemIndexByValue(opID, szOpValue);
		}
		break;
	case OPTID_STOCKINTERVAL:
		{
			devMode->dmFields |= DM_CUTINTERVAL;
			if ( szOpValue == NULL )
				devMode->dmCutInterval = 1; 
			else
				devMode->dmCutInterval = atoi(szOpValue);
//			DebugPrintf("show->dmCutInterval=%d\n", devMode->dmCutInterval);
		}
		break;
	case OPTID_STOCKFEEDOFFSET:
		{
//			DebugPrintf("show->dmFeedOffset=%s\n", szOpValue);
			devMode->dmFields |= DM_FEEDOFFSET;
			if ( szOpValue == NULL )
			{
				devMode->dmFeedOffset = 0.0;
			}
			else
			{
				fValue = atof(szOpValue);
				devMode->dmFeedOffset = fValue; 

//				if ( devMode->dmMetric == DMMETRIC_INCH )
//					devMode->dmFeedOffset = POINT2INCH(fValue); 
//				else
//					devMode->dmFeedOffset = POINT2MM(fValue); 
			}
		}
		break;
	case OPTID_STOCKVERTICALOFFSET:
		{
			devMode->dmFields |= DM_VERTICALOFFSET;
			if ( szOpValue == NULL )
			{
				devMode->dmVerticalOffset = 0.0;
			}
			else
			{
				fValue = atof(szOpValue);
				devMode->dmVerticalOffset = fValue; 
//				DebugPrintf("fValue=%.5f\n", fValue);

//				if ( devMode->dmMetric == DMMETRIC_INCH )
//					devMode->dmVerticalOffset = POINT2INCH(fValue); 
//				else
//					devMode->dmVerticalOffset = POINT2MM(fValue); 
//				DebugPrintf("show->dmVerticalOffset=%s\n", devMode->dmVerticalOffset);
			}
		}
		break;

		// Option
	case OPTID_OPTIONPRINTQUALITY:
		{
			int			xRes, yRes;
			char		szCode[256];
			
			memset(szCode, 0, sizeof(szCode));
//			DebugPrintf("show->dmPrintQuality=%s\n", szOpValue);
//			devMode->dmPrintQuality = GetOptionIndexByChoice(cups, ppd, opID, szOpValue);
			devMode->dmPrintQuality = GetItemIndexByValue(opID, szOpValue);
			
			if ( GetOptionCodeByChoice(cups, ppd, OPTID_OPTIONPRINTQUALITY, szOpValue, szCode) )
			{
				if ( sscanf(szCode, "<</HWResolution[%d %d]", &xRes, &yRes) == 2 )
				{
					devMode->dmPrintQuality = xRes;
					devMode->dmYResolution = yRes;
					devMode->dmFields |= DM_PRINTQUALITY;
					devMode->dmFields |= DM_YRESOLUTION;
				}
//				DebugPrintf("show->dmXResolution=%s\n", szOpValue);
			}
		}
		break;
/*	case OPTID_OPTIONXRESOLUTION:
		{
			if ( szOpValue == NULL )
				devMode->dmXResolution = 0;
			else
				devMode->dmXResolution = atoi(szOpValue); 
		}
		break;
	case OPTID_OPTIONYRESOLUTION:
		{
			devMode->dmFields |= DM_DM_PRINTQUALITY;

			if ( szOpValue == NULL )
				devMode->dmYResolution = 0;
			else
				devMode->dmYResolution = atoi(szOpValue); 
		}
		break;
*/
	case OPTID_OPTIONPRINTERSPEED:
		{
//			DebugPrintf("show->dmPrintSpeed=%s\n", szOpValue);
//			devMode->dmPrintSpeed = GetOptionIndexByChoice(cups, ppd, opID, szOpValue);
			devMode->dmFields |= DM_PRINTSPEED;
			if ( szOpValue )
				devMode->dmPrintSpeed = atoi(szOpValue);
			else
				devMode->dmPrintSpeed = 20;
		}
		break;
	case OPTID_OPTIONDARKNESS:
		{
//			DebugPrintf("show->dmDarkness=%s\n", szOpValue);
			devMode->dmFields |= DM_DARKNESS;
			if ( szOpValue == NULL )
				devMode->dmDarkness = 1;
			else
				devMode->dmDarkness = atoi(szOpValue);
		}
		break;
	case OPTID_OPTIONDIRECTTOBUFFER:
		{
//			DebugPrintf("show->dmDirectBuffer=%s\n", szOpValue);
//			devMode->dmDirectBuffer = GetOptionIndexByChoice(cups, ppd, opID, szOpValue);
			devMode->dmFields |= DM_DIRECTBUFFER;
			devMode->dmDirectBuffer = GetItemIndexByValue(opID, szOpValue);
		}
		break;
	case OPTID_OPTIONSTOREDGRAPHICS:
		{
//			DebugPrintf("show->dmStoredGriphics=%s\n", szOpValue);
//			devMode->dmStoredGriphics = GetOptionIndexByChoice(cups, ppd, opID, szOpValue);
			devMode->dmFields |= DM_STOREDGRIPHICS;
			devMode->dmStoredGriphics = GetItemIndexByValue(opID, szOpValue);
		}
		break;

		// Printing Postion
	case OPTID_PRNPOSHORIZONTALOFFSET:
		{
//			DebugPrintf("show->dmAdjustHorizontal=%s\n", szOpValue);
			devMode->dmFields |= DM_ADJUSTHORIZONTAL;
			if ( szOpValue == NULL )
			{
				devMode->dmAdjustHorizontal = 0.0;
			}
			else
			{
				fValue = atof(szOpValue);
				devMode->dmAdjustHorizontal = fValue; 

//				if ( devMode->dmMetric == DMMETRIC_INCH )
//					devMode->dmAdjustHorizontal = POINT2INCH(fValue); 
//				else
//					devMode->dmAdjustHorizontal = POINT2MM(fValue); 
			}
		}
		break;
	case OPTID_PRNPOSVERTICALOFFSET:
		{
//			DebugPrintf("show->dmAdjustVertical=%s\n", szOpValue);
			devMode->dmFields |= DM_ADJUSTVERTICAL;
			if ( szOpValue == NULL )
			{
				devMode->dmAdjustVertical = 0.0;
			}
			else
			{
				fValue = atof(szOpValue);
				devMode->dmAdjustVertical = fValue; 

//				if ( devMode->dmMetric == DMMETRIC_INCH )
//					devMode->dmAdjustVertical = POINT2INCH(fValue); 
//				else
//					devMode->dmAdjustVertical = POINT2MM(fValue); 
			}
		}
		break;

		// User Command
	case OPTID_USERCMDFUNCHARACTER:
		{
//			DebugPrintf("show->dmCmdFuncChar=%s\n", szOpValue);
			devMode->dmFields |= DM_CMDFUNCCHAR;
			if ( szOpValue == NULL )
				devMode->dmCmdFuncChar = ' ';
			else
				devMode->dmCmdFuncChar = szOpValue[0];
		}
		break;
	case OPTID_LENUSERCMDSTARTJOB:		//"LenStartJob"
		{
			if ( szOpValue == NULL )
				devMode->dmCmdStartJobLength = 0; 
			else
				devMode->dmCmdStartJobLength = atoi(szOpValue);
		}
		break;
	case OPTID_LENUSERCMDSTARTLABEL:	//"LenStartLabel"
		{
			if ( szOpValue == NULL )
				devMode->dmCmdStartLabelLength = 0; 
			else
				devMode->dmCmdStartLabelLength = atoi(szOpValue);
		}
		break;
	case OPTID_LENUSERCMDENDLABEL:		// "LenEndLabel"
		{
			if ( szOpValue == NULL )
				devMode->dmCmdEndLabelLength = 0; 
			else
				devMode->dmCmdEndLabelLength = atoi(szOpValue);
		}
		break;
	case OPTID_LENUSERCMDENDJOB:		// "LenEndJob"
		{
			if ( szOpValue == NULL )
				devMode->dmCmdEndJobLength = 0; 
			else
				devMode->dmCmdEndJobLength = atoi(szOpValue);
		}
		break;
	case OPTID_USERCMDSTARTJOB:
		{
//			char strStartJob[DM_USER_COMMOND_LENGTH];
		
			devMode->dmFields |= DM_CMDSTARTJOB;
			memset(szOpCmdValue, 0, sizeof(szOpCmdValue));			
			Decode(szOpValue, devMode->dmCmdStartJob);
		}
		break;
	case OPTID_USERCMDSTARTLABEL:
		{
//			char strStartLabel[DM_USER_COMMOND_LENGTH];
			devMode->dmFields |= DM_CMDSTARTLABEL;
		
			memset(szOpCmdValue, 0, sizeof(szOpCmdValue));			
			Decode(szOpValue, devMode->dmCmdStartLable);
		}
		break;
	case OPTID_USERCMDENDLABEL:
		{
			devMode->dmFields |= DM_CMDENDLABEL;
			memset(szOpCmdValue, 0, sizeof(szOpCmdValue));			
			Decode(szOpValue, devMode->dmCmdEndLable);
		}
		break;
	case OPTID_USERCMDENDJOB:
		{
//			DebugPrintf("USERCMDENDJOB, No1\n");
			devMode->dmFields |= DM_CMDENDJOB;
			memset(szOpCmdValue, 0, sizeof(szOpCmdValue));			
			Decode(szOpValue, devMode->dmCmdEndJob);
		}
		break;
	case OPTID_USERCMDSTARTJOBNOCTRL:	//			"OriStartJob"
		{
//			memset(oriCmdEndLabel, 0, sizeof(oriCmdStartJob));
			if ( szOpValue )
				Decode(szOpValue, devMode->dmCmdOriStartJob);
//			DebugPrintf("Load, devMode->dmCmdOriStartJob=%s\n", devMode->dmCmdOriStartJob);
		}
		break;
	case OPTID_USERCMDSTARTLABELNOCTRL: // 			"OriStartLabel"
		{
			if ( szOpValue )
				Decode(szOpValue, devMode->dmCmdOriStartLable);
//			DebugPrintf("Load, devMode->dmCmdOriStartLable=%s\n", devMode->dmCmdOriStartLable);
		}
		break;
	case OPTID_USERCMDENDLABELNOCTRL:	// 			"OriEndLabel"
		{
			if ( szOpValue )
				Decode(szOpValue, devMode->dmCmdOriEndLable);
//			DebugPrintf("Load, devMode->dmCmdOriEndLable=%s\n", devMode->dmCmdOriEndLable);
		}
		break;
	case OPTID_USERCMDENDJOBNOCTRL:		// 			"OriEndJob"
		{
			if ( szOpValue )
				Decode(szOpValue, devMode->dmCmdOriEndJob);
//			DebugPrintf("Load, devMode->dmCmdOriEndJob=%s\n", devMode->dmCmdOriEndJob);
		}
		break;
	case OPTID_METRIC:
		{
			if ( szOpValue )
			{
				if ( !strcmp(szOpValue, STR_PPDMETRIC_AUTO) )
				{
					char			syslang[128];

					memset(syslang, 0, sizeof(syslang));

					devMode->dmMetric = SetMetricString(syslang, cups);
				}
				else if ( !strcmp(szOpValue, STR_PPDMETRIC_MM) )
				{
					devMode->dmMetric = DMMETRIC_MM;
				}
				else
				{
					devMode->dmMetric = DMMETRIC_INCH;
				}
			}
		}
		break;
	default:
		break;
	}

	return bRet;
}




