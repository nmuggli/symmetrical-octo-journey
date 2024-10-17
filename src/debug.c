/*
 * "debug.c 2021-05-17 15:55:05
 *  
 *  debug routine for TSC Printer Driver
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
 *
 *	
 *
 */

#include "config.h"
#include "common.h"

#ifndef _DEBUG
	#define _DEBUG
#else
	#define	IS_DEBUG
#endif
#include "debug.h"

struct {
	short		level;
	const char*	str;
} ListErrorLevel[] = {
	{ LEVEL_DEBUG,		"DEBUG: "},
	{ LEVEL_DEBUG2,		"DEBUG2: "},
	{ LEVEL_INFO,		"INFO: "},
	{ LEVEL_NOTICE,		"NOTICE: "},
	{ LEVEL_WARNING,	"WARNING: "},
	{ LEVEL_ERROR,		"ERROR: "},
	{ LEVEL_ALERT,		"ALERT: "},
	{ LEVEL_EMERG,		"EMERG: "},
	{ LEVEL_CRIT,		"CRIT: "},
};
#ifdef IS_DEBUG
	#define		PRE_MSG		"TSC: "
#else
	#define		PRE_MSG		""
#endif	// #ifdef IS_DEBUG

static char* Error_Format_Message(int ErrorLevel, const char *strfmt);
static int Error_Log_v(int ErrorLevel, const char* strfmt, va_list args);

void DebugPrintf(const char* pstrFormat, ... )
{
	va_list args;

	va_start(args, pstrFormat);
	Error_Log_v(LEVEL_DEBUG, pstrFormat, args);
	va_end(args);
}

int Error_Log(int ErrorLevel, const char* strfmt, ... )
{
	int		nRtn = 0;
	va_list args;

	va_start(args, strfmt);
	nRtn = Error_Log_v(ErrorLevel, strfmt, args);
	va_end(args);

	return nRtn;
}

char* Error_Format_Message(int ErrorLevel, const char *strfmt)
{
	int			i;
	int			nLineCnt = 0;
	size_t		nPreLen;
	int			nPreLen2;
	const char	*szPre = "";
	const char	*s;
	char		*p;
	char		*buff = NULL;

	for (i=0; i<sizeof(ListErrorLevel)/sizeof(ListErrorLevel[0]); i++)
	{
		if ( ErrorLevel == ListErrorLevel[i].level )
		{
			szPre = ListErrorLevel[i].str;
			break;
		}
	}
	nPreLen = strlen(szPre);
	nPreLen2 = strlen(PRE_MSG);

	for (s=strfmt, nLineCnt=0; *s; s++)
	{
		if ( *s == '\n' )
			nLineCnt ++;
	}
	nLineCnt++;

	buff = (char*)MEMALLOC(strlen(strfmt) + nLineCnt * (nPreLen + nPreLen2)+ 5 );
	if ( buff )
	{
		p=buff;
		strcpy(p, szPre);
		p += nPreLen;
		strcpy(p, PRE_MSG);
		p += nPreLen2;

		for (s=strfmt; *s; s++)
		{
			switch ( *s )
			{
			case '\r':
				break;
			case '\n':
				if ( *(s+1) )
				{
					*p ++ = *s;
					strcpy(p, szPre);
					p += nPreLen;
					strcpy(p, PRE_MSG);
					p += nPreLen2;
				}
				break;
			default:
				*p ++ = *s;
				break;
			}
		}

		if ( p != buff && *(p-1) != '\n')
			*p ++ = '\n';

		*p = 0;
	}
	return buff;
}

int Error_Log_v(int ErrorLevel, const char* strfmt, va_list args)
{
	int		nRtn = 0;
	char	*buff = NULL;
	char	*pre = "";
	char	*fmtbuff = NULL;

	fmtbuff = Error_Format_Message(ErrorLevel, strfmt);
	if ( fmtbuff )
	{
		nRtn = vfprintf(stderr, fmtbuff, args);
		MEMFREE(fmtbuff);
	}
	return nRtn;
}

void DumpRESUID()
{
	uid_t	ruid;
	uid_t	euid;
	uid_t	suid;

	if ( getresuid(&ruid, &euid, &suid) )
		DebugPrintf("getresuid error: %s\n", strerror(errno));
	else
		DebugPrintf("ruid = %d, euid = %d, suid = %d\n", ruid, euid, suid);
}

void DumpDevmode(DEVMODE *pdm)
{
	int		ErrorLevel = LEVEL_DEBUG;
	if ( pdm )
	{
		Error_Log(ErrorLevel, "DEVMODE.dmSize             = %d\n", pdm->dmSize);
		Error_Log(ErrorLevel, "DEVMODE.dmFields           = %08X\n", pdm->dmFields);

		Error_Log(ErrorLevel, "DEVMODE.dmOrientation      = %d\n", pdm->dmOrientation);
		Error_Log(ErrorLevel, "DEVMODE.dmPaperSize        = %d\n", pdm->dmPaperSize);
		Error_Log(ErrorLevel, "DEVMODE.dmPaperLength      = %f\n", pdm->dmPaperLength);
		Error_Log(ErrorLevel, "DEVMODE.dmPaperWidth       = %f\n", pdm->dmPaperWidth);

		Error_Log(ErrorLevel, "DEVMODE.dmMirrorImage      = %d\n", pdm->dmMirrorImage);
		Error_Log(ErrorLevel, "DEVMODE.dmNegativeImage    = %d\n", pdm->dmNegativeImage);
		Error_Log(ErrorLevel, "DEVMODE.dmMediaMethod      = %d\n", pdm->dmMediaMethod);
		Error_Log(ErrorLevel, "DEVMODE.dmMediaType        = %d\n", pdm->dmMediaType);

		Error_Log(ErrorLevel, "DEVMODE.dmGapHeight        = %f\n", pdm->dmGapHeight);
		Error_Log(ErrorLevel, "DEVMODE.dmGapOffset        = %f\n", pdm->dmGapOffset);
		Error_Log(ErrorLevel, "DEVMODE.dmPostAction       = %d\n", pdm->dmPostAction);
		Error_Log(ErrorLevel, "DEVMODE.dmOccurrence       = %d\n", pdm->dmOccurrence);
		Error_Log(ErrorLevel, "DEVMODE.dmCutInterval      = %d\n", pdm->dmCutInterval);

		Error_Log(ErrorLevel, "DEVMODE.dmFeedOffset       = %f\n", pdm->dmFeedOffset);
		Error_Log(ErrorLevel, "DEVMODE.dmVerticalOffset   = %f\n", pdm->dmVerticalOffset);

		Error_Log(ErrorLevel, "DEVMODE.dmPrintSpeed       = %d\n", pdm->dmPrintSpeed);
		Error_Log(ErrorLevel, "DEVMODE.dmDarkness         = %d\n", pdm->dmDarkness);
		Error_Log(ErrorLevel, "DEVMODE.dmDirectBuffer     = %d\n", pdm->dmDirectBuffer);
		Error_Log(ErrorLevel, "DEVMODE.dmStoredGriphics   = %d\n", pdm->dmStoredGriphics);

		Error_Log(ErrorLevel, "DEVMODE.dmAdjustHorizontal = %f\n", pdm->dmAdjustHorizontal);
		Error_Log(ErrorLevel, "DEVMODE.dmAdjustVertical   = %f\n", pdm->dmAdjustVertical);

		Error_Log(ErrorLevel, "DEVMODE.dmCmdStartJobLength   = %d\n", pdm->dmCmdStartJobLength);
		Error_Log(ErrorLevel, "DEVMODE.dmCmdStartLabelLength = %d\n", pdm->dmCmdStartLabelLength);
		Error_Log(ErrorLevel, "DEVMODE.dmCmdEndLabelLength   = %d\n", pdm->dmCmdEndLabelLength);
		Error_Log(ErrorLevel, "DEVMODE.dmCmdEndJobLength     = %d\n", pdm->dmCmdEndJobLength);

		Error_Log(ErrorLevel, "DEVMODE.dmMetric       = %d\n", pdm->dmMetric);
		Error_Log(ErrorLevel, "DEVMODE.dmPrintQuality = %d\n", pdm->dmPrintQuality);
		Error_Log(ErrorLevel, "DEVMODE.dmYResolution  = %d\n", pdm->dmYResolution);
		Error_Log(ErrorLevel, "DEVMODE.dmCopies       = %d\n", pdm->dmCopies);
	}
	else
	{
		Error_Log(ErrorLevel, "DEVMODE is NULL\n");
	}
}
