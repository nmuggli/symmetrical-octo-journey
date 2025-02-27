/*
 * "libloader.h 2021-05-17 15:55:05
 *  
 *  gs process routines declaration for TSC Printer Driver
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

#ifndef _GSRUN_H_
#define _GSRUN_H_

int gsrun(DEVDATA *pdev);
int psrun(DEVDATA *pdev, char *line, size_t linelen, size_t linesize);

BOOL OutputDevmode(DEVDATA *pdev);

BOOL gs_write(DEVDATA *pdev, const char *s, size_t len);
BOOL gs_putchar(DEVDATA *pdev, char c);
BOOL gs_printf(DEVDATA *pdev, const char *format, ...);

#define gs_puts(pdev, s)			gs_write(pdev, s, strlen(s))

#endif	// #ifndef _GSRUN_H_
