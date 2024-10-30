/*
 * "bmp2tspl.c 2021-05-17 15:55:05
 *  
 *  bmp convert to tspl command routines for TSC Printer Driver
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

static size_t ReadPipe(int fd, void* buffer, size_t size);
static size_t SkipPipe(int fd, size_t size);
static int ReadDevmode(int fdIn, DEVMODE **pdm);
static int ReadBitmapData(int fdIn, BITMAPFILEHEADER *pbmfHeader, BITMAPINFOHEADER *pbiHeader, RGBQUAD **ppColorTable, LPVOID* ppBits);

int bmp2tspl(int fdIn)
{
	DEVMODE				*pdm = NULL;
	int					iRtn = -1;
	BITMAPFILEHEADER	bmfHeader;
	BITMAPINFOHEADER	biHeader;
	RGBQUAD				*pColorTable = NULL;
	LPVOID				pBits = NULL;

	DebugPrintf("Enter bmp2tspl\n");

	iRtn = ReadDevmode(fdIn, &pdm);

	if ( pdm->dmCopies == 0 )
		pdm->dmCopies = 1;
	
	if ( pdm->dmPrintQuality == 0 )
		pdm->dmPrintQuality = DPI_300;
	if ( pdm->dmFields & DM_YRESOLUTION )
	{
		if ( pdm->dmYResolution == 0 )
			pdm->dmYResolution = DPI_300;
	}
	else
	{
		pdm->dmYResolution = pdm->dmPrintQuality ;
	}

	pdm->dmOutPages = 0;

	for ( ; iRtn > 0 ;)
	{
		iRtn = ReadBitmapData(fdIn, &bmfHeader, &biHeader, &pColorTable, &pBits);
		if ( iRtn == 0 )
		{
			// Input stream end
			// Send Job End
			TSPL_SendJobEnd(pdm);
		}
		else if ( iRtn > 0 )
		{
			// Send Page
			TSPL_SendPage(pdm, &biHeader, pColorTable, pBits);
		}
		MEMFREE(pColorTable);
		MEMFREE(pBits);
	}
	MEMFREE(pdm);

	DebugPrintf("Leave bmp2tspl, return %d\n", iRtn);
	return iRtn;
}

int ReadDevmode(
	int			fdIn,
	DEVMODE		**ppdm
)
{
	size_t			cbReaded;
	DEVMODE			dm;

	DebugPrintf("Enter ReadDevmode\n");

	*ppdm = NULL;
	cbReaded = ReadPipe(fdIn, &dm, sizeof(DEVMODE));
	if ( sizeof(DEVMODE) != cbReaded || sizeof(DEVMODE) != dm.dmSize || dm.dmType != DM_HEADER_MARKER )
	{
		return -1;
	}
	
	*ppdm = MEMALLOC(dm.dmSize + dm.dmSizeExtra);
	if ( *ppdm == NULL )
	{
		return -1;
	}
	memcpy(*ppdm, &dm, sizeof(DEVMODE));
	cbReaded = ReadPipe(fdIn, ((LPBYTE)*ppdm) + sizeof(DEVMODE), dm.dmSizeExtra);
	if ( cbReaded != dm.dmSizeExtra )
	{
		return -1;
	}

	DebugPrintf("==== DEVMODE from pipe ====\n");
	DumpDevmode(&dm);
	
	return 1;
}

int ReadBitmapData(
	int					fdIn,
	BITMAPFILEHEADER	*pbmfHeader,
	BITMAPINFOHEADER	*pbiHeader,
	RGBQUAD				**ppColorTable,
	LPVOID				*ppBits
)
{
	int					iRtn = -1;
	size_t				cbReaded;
	size_t				cbPageRemainSize;

	DebugPrintf("Enter ReadBitmapData\n");

	*ppBits = NULL;
	if ( ppColorTable )
		*ppColorTable = NULL;

	// read bitmap file header
	cbReaded = ReadPipe(fdIn, pbmfHeader, sizeof(BITMAPFILEHEADER));
	if ( cbReaded <= 0 )
	{
		// no data readed, file end
		return 0;
	}
	pbmfHeader->bfType = ENDIEN16(pbmfHeader->bfType);
	pbmfHeader->bfSize = ENDIEN32(pbmfHeader->bfSize);
	pbmfHeader->bfOffBits = ENDIEN32(pbmfHeader->bfOffBits);
	if ( sizeof(BITMAPFILEHEADER) != cbReaded || pbmfHeader->bfType != DIB_HEADER_MARKER )
	{
		Error_Log(LEVEL_ERROR, "Invaild Data Header\n");
		return -1;
	}

	// calculate current bitmap file remain size
	cbPageRemainSize = pbmfHeader->bfSize - sizeof(BITMAPFILEHEADER);

	// read bitmap info header
	cbReaded = ReadPipe(fdIn, pbiHeader, sizeof(BITMAPINFOHEADER));

	pbiHeader->biSize = ENDIEN32(pbiHeader->biSize);
	pbiHeader->biWidth = ENDIEN32(pbiHeader->biWidth);
	pbiHeader->biHeight = ENDIEN32(pbiHeader->biHeight);
	pbiHeader->biPlanes = ENDIEN16(pbiHeader->biPlanes);
	pbiHeader->biBitCount = ENDIEN16(pbiHeader->biBitCount);
	pbiHeader->biCompression = ENDIEN32(pbiHeader->biCompression);
	pbiHeader->biSizeImage = ENDIEN32(pbiHeader->biSizeImage);
	pbiHeader->biXPelsPerMeter = ENDIEN32(pbiHeader->biXPelsPerMeter);
	pbiHeader->biYPelsPerMeter = ENDIEN32(pbiHeader->biYPelsPerMeter);
	pbiHeader->biClrUsed = ENDIEN32(pbiHeader->biClrUsed);
	pbiHeader->biClrImportant = ENDIEN32(pbiHeader->biClrImportant);

	if ( sizeof(BITMAPINFOHEADER) != cbReaded || sizeof(BITMAPINFOHEADER) != pbiHeader->biSize )
	{
		Error_Log(LEVEL_ERROR, "Invaild Data BITMAPINFOHEADER\n");
		return -1;
	}
	cbPageRemainSize -= cbReaded;

	// read color table
	switch ( pbiHeader->biBitCount )
	{
	case 1:
	case 4:
	case 8:
		{
			size_t			nColorCount;

			nColorCount = (1 << pbiHeader->biBitCount);
			if ( pbiHeader->biClrUsed != 0 && pbiHeader->biClrUsed < nColorCount)
			{
				nColorCount = pbiHeader->biClrUsed;
			}
			nColorCount = min(nColorCount, 256);

			// read color table
			if ( ppColorTable )
			{
				*ppColorTable = MEMALLOC(sizeof(RGBQUAD) * nColorCount);
				if ( *ppColorTable == NULL )
				{
					Error_Log(LEVEL_ERROR, "Can not Alloc Memroy %d Bytes\n", sizeof(RGBQUAD) * nColorCount);
					return -1;
				}
				cbReaded = ReadPipe(fdIn, *ppColorTable, sizeof(RGBQUAD) * nColorCount);
			}
			else
			{
				cbReaded = SkipPipe(fdIn, sizeof(RGBQUAD) * nColorCount);
			}
			if ( cbReaded <= 0 )
			{
				Error_Log(LEVEL_ERROR, "Invaild Bitmap Data(Color Table)\n");
				return cbReaded;
			}
			cbPageRemainSize -= cbReaded;
		}
		break;
	case 16:
	case 24:
		// true color, no color table
		break;
	}

	// Skip to bits data start
	if ( pbmfHeader->bfOffBits > (pbmfHeader->bfSize - cbPageRemainSize) )
	{
		size_t	nSikp;

		nSikp = pbmfHeader->bfOffBits - (pbmfHeader->bfSize - cbPageRemainSize);
		cbReaded = SkipPipe(fdIn, nSikp);
		if ( cbReaded <= 0 )
		{
			Error_Log(LEVEL_ERROR, "Invaild Bitmap Data (Skip Data)\n");
			return -1;
		}
		cbPageRemainSize -= cbReaded;
	}

	{
		DWORD				cbWidthBytes;
		DWORD				cbBmpBytes;
		LPVOID				pBitsLine;

		cbWidthBytes = WIDTHBYTES_32(pbiHeader->biWidth);
		cbBmpBytes = cbWidthBytes * pbiHeader->biHeight;

		*ppBits = MEMALLOC(cbBmpBytes);
		if ( *ppBits )
		{
			LONG		y;

			iRtn = 1;
			for(y = pbiHeader->biHeight-1; y>=0; y--)
			{
				pBitsLine = (LPBYTE)(*ppBits) + cbWidthBytes * y;
				cbReaded = ReadPipe(fdIn, pBitsLine, cbWidthBytes);
				if ( cbReaded != cbWidthBytes )
				{
					iRtn = -1;
					MEMFREE(*ppBits);
					break;
				}
			}
		}
		else
		{
			Error_Log(LEVEL_ERROR, "Can not Alloc Memory %d Bytes\n", cbBmpBytes);
		}
	}
	return iRtn;
}

size_t ReadPipe(int fd, void* buffer, size_t size)
{
	size_t	nBytes;
	size_t	nRemain = size;
	size_t	nReaded = 0;

	do {
		nBytes = read(fd, buffer + nReaded, nRemain);
		if ( nBytes <= 0 )
		{
			// pipe is closed
			return 0;
		}
		nRemain -= nBytes;
		nReaded += nBytes;
	}
	while ( nReaded < size );

	return nReaded;
}

size_t SkipPipe(int fd, size_t size)
{
	size_t	nRemain = size;
	size_t	nReaded = size;
	size_t	nRead;
	BYTE	buff[256];

	while (nRemain > 0 && nReaded > 0)
	{
		nRead = nRemain > sizeof(buff) ? sizeof(buff) : nRemain;

		nReaded = ReadPipe(fd, buff, nRead);
		nRemain -= nReaded;
	}

	return nReaded > 0 ? size : nReaded;
}
