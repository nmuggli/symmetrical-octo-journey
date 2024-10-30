/*
 * "common.h 2021-05-17 15:55:05
 *  
 *  common routines declaration for TSC Printer Driver
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
 *
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <fcntl.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <unistd.h>

#define	UNREFERENCED_PARAMETER(x)		x

#define MEMALLOC(x)			calloc(1, x)
#define MEMFREE(x)			if(x){ free(x); x=NULL; }

//////////////////////////////////////////////////

#define	CONST				const

#define	PASCAL
#define	FAR
#define	NEAR
#define	WINAPI				FAR PASCAL

//////////////////////////////////////////////////

typedef unsigned int		DWORD;
typedef unsigned short		WORD;
typedef unsigned char		BYTE;
typedef int					LONG;
typedef unsigned int		ULONG;
typedef short				SHORT;
typedef unsigned short		USHORT;
typedef char				CHAR;
#ifndef VOID
typedef void				VOID;
#endif
typedef int					INT;
typedef unsigned int		UINT;

typedef BYTE				*PBYTE;
typedef BYTE				FAR *LPBYTE;
typedef WORD				FAR *LPWORD;
typedef DWORD				FAR *LPDWORD;
typedef LONG				FAR *LPLONG;
typedef VOID				*PVOID, FAR *LPVOID;
typedef CHAR				*PSTR;
typedef CHAR				FAR *LPSTR;
typedef CONST CHAR *LPCSTR, *PCSTR;

//typedef long				INT_PTR, *PINT_PTR;
//typedef unsigned long		UINT_PTR, *PUINT_PTR;

#ifndef BOOL
	typedef short				BOOL;
#endif

#ifndef TRUE
	#define	TRUE				-1
#endif
#ifndef FALSE
	#define	FALSE				0
#endif

#ifndef WORD_MAX
#define WORD_MAX			USHRT_MAX
#endif	// #ifndef WORD_MAX

typedef DWORD COLORREF;

typedef struct tagRECT
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECT, *PRECT, NEAR *NPRECT, FAR *LPRECT;

typedef const RECT FAR* LPCRECT;

typedef struct _RECTL       /* rcl */
{
    LONG    left;
    LONG    top;
    LONG    right;
    LONG    bottom;
} RECTL, *PRECTL, *LPRECTL;

typedef const RECTL FAR* LPCRECTL;

typedef struct tagPOINT
{
    LONG  x;
    LONG  y;
} POINT, *PPOINT, NEAR *NPPOINT, FAR *LPPOINT;

typedef struct _POINTL      /* ptl  */
{
    LONG  x;
    LONG  y;
} POINTL, *PPOINTL;

typedef struct tagSIZE
{
    LONG        cx;
    LONG        cy;
} SIZE, *PSIZE, *LPSIZE;

typedef SIZE               SIZEL;
typedef SIZE               *PSIZEL, *LPSIZEL;

//////////////////////////////////////////////////
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)           ((WORD)((DWORD)(l) & 0xffff))
#define HIWORD(l)           ((WORD)((DWORD)(l) >> 16))
#define LOBYTE(w)           ((BYTE)((DWORD)(w) & 0xff))
#define HIBYTE(w)           ((BYTE)((DWORD)(w) >> 8))

#define	max(a, b)			((a) > (b) ? (a) : (b))
#define	min(a, b)			((a) < (b) ? (a) : (b))

#define WIDTHBYTES_8(bits)		(((bits) + 7) >> 3)
#define WIDTHBYTES_16(bits)		(((bits) + 16) >> 4 << 1)
#define WIDTHBYTES_32(bits)		(((bits) + 31) >> 5 << 2)
#define WIDTHBYTES(bits)    	WIDTHBYTES_32(bits)

#define ARRAYCOUNT(x)	(sizeof(x)/sizeof(x[0]))

#define ASSERT(f)          ((void)0)
#define VERIFY(f)          ((void)(f))
#define ASSERT_VALID(pOb)  ((void)0)

#define	POINT2INCH(x)			((x)/72)
#define	POINT2MM(x)				((x)*25.4/72)
#define	POINT2DOT(x, dpi)		((x)*(dpi)/72)

#ifdef __cplusplus
extern "C" {
#endif

WORD	ENDIEN16(WORD x);
DWORD	ENDIEN32(DWORD x);

#ifndef HAVE_STRLCPY
	size_t my_strlcpy(char *dst, const char *src, size_t dst_sz);
	#define strlcpy		my_strlcpy
#endif /* !HAVE_STRLCPY */

#ifdef __IS_TRIAL_VERSION__
	int CheckTrialTime();
#else
	#define CheckTrialTime()	0
#endif

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _COMMON_H_
