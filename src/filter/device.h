/*
 * "device.h 2021-05-17 15:55:05
 *  
 *  device routine declaration for TSC Printer Driver
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
 
 */
#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "devmode.h"

#ifdef _DEBUG
//	#define FILTER_NOT_PSTOPS
//	#define FILTER_NOT_PS2BMP
//	#define FILTER_NOT_BMP2TSPL
#endif

#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')

#pragma pack(2)
typedef struct tagBITMAPFILEHEADER {
	WORD	bfType;
	DWORD	bfSize;
	WORD	bfReserved1;
	WORD	bfReserved2;
	DWORD	bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
	DWORD	biSize;
	LONG	biWidth;
	LONG	biHeight;
	WORD	biPlanes;
	WORD	biBitCount;
	DWORD	biCompression;
	DWORD	biSizeImage;
	LONG	biXPelsPerMeter;
	LONG	biYPelsPerMeter;
	DWORD	biClrUsed;
	DWORD	biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
	BYTE	rgbBlue; 
	BYTE	rgbGreen; 
	BYTE	rgbRed; 
	BYTE	rgbReserved; 
} RGBQUAD, *PRGBQUAD;
#pragma pack()

typedef struct _GSDATA
{
	void			*gsInstance;
	int				exit_code;
} GSDATA;

typedef struct _DEVDATA
{
	cups_file_t			*fpPS;				// Print file

	CUPSLIB_FUNCTION	lib_cups;			// CUPS lib
	GSLIB_FUNCTION		lib_gs;				// GS lib

	char				*szPrinterName;		// Printer name

	int					num_options;		// CUPS option count
	cups_option_t		*options;			// CUPS options

	ppd_file_t			*ppd;				// PPD File

	DEVMODE				dm;

	LPCSTR				gsdevice;			// 
	GSDATA				gsdata;

} DEVDATA;


int ps2bmp(int argc, char *argv[]);
int bmp2tspl(int fdIn);

int TSPL_SendPage(DEVMODE *pdm, BITMAPINFOHEADER* pBih, RGBQUAD *pColorTable, void* pBits);
int TSPL_SendJobEnd(DEVMODE *pdm);

#endif	// #ifndef _DEVICE_H_
