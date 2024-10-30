/*
 * "debug.h 2021-05-17 15:55:05
 *  
 *  debug routine declaration for TSC Printer Driver
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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define LEVEL_DEBUG			1
#define LEVEL_DEBUG2		2
#define LEVEL_INFO			3
#define LEVEL_NOTICE		4
#define LEVEL_WARNING		5
#define LEVEL_ERROR			6
#define LEVEL_ALERT			7
#define LEVEL_EMERG			8
#define LEVEL_CRIT			9

int Error_Log(int ErrorLevel, const char* strfmt, ... );

#ifdef _DEBUG
	#include "devmode.h"

	void DebugPrintf(const char* pstrFormat, ... );
	void DumpRESUID();
	void DumpDevmode(DEVMODE *pdm);
#else
	#define	DebugPrintf
	#define	DumpRESUID
	#define	DumpDevmode
#endif	// #ifdef _DEBUG

#ifdef __cplusplus
}
#endif


#endif	// #ifndef _DEBUG_H_
