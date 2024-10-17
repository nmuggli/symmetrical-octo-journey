/*
 * "common.c 2021-05-17 15:55:05
 *  
 *  common routines for TSC Printer Driver
 *  
 *  Copyright (c) 2005, by TSC Printronix Auto ID .
 *  
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
 *
 */
 
#include "config.h"
#include "common.h"
#include <time.h>

WORD	ENDIEN16(WORD x)
{
	BYTE	*p = (BYTE*)&x;

	return (((WORD)p[1]) << 8) | p[0];
}

DWORD	ENDIEN32(DWORD x)
{
	BYTE	*p = (BYTE*)&x;

	return (((DWORD)p[3]) << 24) | (((DWORD)p[2]) << 16) | (((DWORD)p[1]) << 8) | p[0];
}

size_t my_strlcpy(char *dst, const char *src, size_t dst_sz)
{
    size_t n;

    for (n=0; n<dst_sz; n++)
    {
		if ((*dst++ = *src++ ) == '\0')
			break;
	}

    if (n < dst_sz)
	    return n;

    if (n > 0)
	    *(dst - 1) = '\0';

    return n;
}

const char g_alphabet[] = 
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void Encode(const unsigned char *pSrc, int srclen, char *szOut)
{
	unsigned int buf;
	int i, nOffset;
	unsigned char *p = (unsigned char *)pSrc;      
	char *r = szOut;
	while(p - pSrc < srclen)
	{
		//3 * 8 ==> 4 * 6
		if(p - pSrc +3 < srclen)
		{
			buf = (*p << 16) | (*(p + 1) << 8) | *(p + 2);
			p += 3;
		}
		else
		{
			buf = 0;
			nOffset = p - pSrc;
			while(p - pSrc < srclen)
			{
				buf <<= 8;
				buf |= *p++;
			}
			buf <<= ((srclen - nOffset + 1) * 6) % 8;
			for(i = srclen - nOffset; i >= 0; i--)
				*r++ = g_alphabet[(buf >> (i * 6)) & 0x3F];
			break;
		}
		*r++ = g_alphabet[(buf >> 18) & 0x3F];
		*r++ = g_alphabet[(buf >> 12) & 0x3F];
		*r++ = g_alphabet[(buf >> 6) & 0x3F];
		*r++ = g_alphabet[buf & 0x3F];
	}      
	*r = 0;
}

// Base64
void Decode(const char *szCoded, unsigned char *pOut)
{      
	short nDecTab[256];
	short i;
	unsigned int buf;
	int nOffset, len = strlen(szCoded);
	char *p = (char *)szCoded;      
	unsigned char *r = pOut;
	memset(nDecTab, -1, sizeof(short) * 256);
	for(i = 0; i < 64; i++)
	{
		nDecTab[g_alphabet[i]] = i;
	}
	nDecTab['='] = -1;
	while(*p)
	{      
		//4 * 6 ==> 3 * 8
		if(p + 4 - szCoded <= len)
		{
			buf = ((nDecTab[*p] & 0x3F) << 18) | ((nDecTab[*(p + 1)] & 0x3F) << 12) |
				((nDecTab[*(p + 2)] & 0x3F) << 6) | (nDecTab[*(p + 3)] & 0x3F);
			p += 4;
		}
		else
		{
			nOffset = p - szCoded;
			if(nOffset != len)
			{
				buf = 0;
				while(*p)
				{
					buf <<= 6;
					buf |= (nDecTab[*p++] & 0x3F);      
				}
				buf >>= ((len - nOffset) * 6) % 8;
				for(i = ((len - nOffset) * 6) >> 3; i > 0; i--)
					*r++ = (buf >> ((i - 1) << 3)) & 0xFF;
				break;
			}                  
		}
		*r++ = (buf >> 16) & 0xFF;
		*r++ = (buf >> 8) & 0xFF;
		*r++ = buf & 0xFF;
	}      
	*r = 0;
}

#ifdef __IS_TRIAL_VERSION__
int CheckTrialTime()
{
	time_t		t1, t2;
	struct tm	tm;
	
	time(&t1);
	memset(&tm, 0, sizeof(tm));
	tm.tm_year = 110;
	tm.tm_mday = 1;
	t2 = mktime(&tm);
	
	return ( t1 > t2 ) ? 1 : 0;
}
#endif	// #ifdef __IS_TRIAL_VERSION__
