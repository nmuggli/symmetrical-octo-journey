/*
 * "gsrun.c 2021-05-17 15:55:05
 *  
 *  gs process routines for TSC Printer Driver
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


#include "config.h"
#include "common.h"
#include "debug.h"
#include "device.h"
#include "libloader.h"
#include "gsrun.h"

static BOOL gsEnable(DEVDATA *pdev);
static void gsDisable(DEVDATA *pdev);
static BOOL gsClose(DEVDATA *pdev);

static int handleExit(int code, int outerr);
static int GSDLLCALL my_stdin(void *instance, char *buf, int len);
static int GSDLLCALL my_stdout(void *instance, const char *str, int len);
static int GSDLLCALL my_stderr(void *instance, const char *str, int len);

int gsrun(DEVDATA *pdev)
{
	int				nRtn = -1;
	char			line[8192];		/* Line buffer */
	size_t			len;			/* Length of line buffer */
	int				needPS2PS = 0;

	DebugPrintf("\n#ENTER: gsrun()\n");

	if ((len = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, sizeof(line))) == 0)
	{
		Error_Log(LEVEL_ERROR, "Empty print file!\n");
	}
	else if ( strncmp(line, "%!PS-Adobe-", 11) )
	{
		Error_Log(LEVEL_ERROR, "Unknow file content!\n");
	}
	else if ( gsEnable(pdev) )
	{
		nRtn = 0;

#ifdef FILTER_NOT_PSTOPS

		OutputDevmode(pdev);
		do
		{
			if ( ! gs_write(pdev, line, len) )
			{
				nRtn = 1;
				break;
			}
		} while ((len = pdev->lib_cups.cupsFileGetLine(pdev->fpPS, line, sizeof(line))) > 0);

#else	// #ifndef FILTER_NOT_PSTOPS

		nRtn = psrun(pdev, line, len, sizeof(line));

#endif	// #ifndef FILTER_NOT_PSTOPS

		if ( !nRtn && !gsClose(pdev))
		{
			nRtn = 1;
		}

		gsDisable(pdev);
	}

	DebugPrintf("#LEAVE: gsrun(), return %d\n\n", nRtn);
	return nRtn;
}

BOOL gs_write(
	DEVDATA			*pdev,
    const char		*s,
	size_t			len
)
{
#ifdef FILTER_NOT_PS2BMP
    fwrite(s, 1, len, stdout);
#else
	if (pdev->gsdata.exit_code && !handleExit(pdev->gsdata.exit_code, 0))
		return FALSE;

	pdev->lib_gs.gsapi_run_string_continue(pdev->gsdata.gsInstance, s, len, 0, &pdev->gsdata.exit_code);
	if (pdev->gsdata.exit_code && !handleExit(pdev->gsdata.exit_code, 1))
	{
		return FALSE;
	}
#endif	// #ifndef FILTER_NOT_PS2BMP
	return TRUE;
}

BOOL gs_putchar(DEVDATA *pdev, char c)
{
	return gs_write(pdev, &c, 1);
}

BOOL gs_printf(
	DEVDATA			*pdev,
	const char		*format,
	...
)
{
	va_list		ap;					/* Pointer to arguments */
	char		buffer[1024];		/* Output buffer */
	size_t		bytes;				/* Number of bytes to write */

	va_start(ap, format);
	bytes = vsnprintf(buffer, sizeof(buffer), format, ap);
	va_end(ap);

	if (bytes > sizeof(buffer))
	{
		Error_Log(LEVEL_ERROR, "gs_printf overflow (%d bytes) detected, aborting!\n", (int)bytes);
		return FALSE;
	}

	return gs_write(pdev, buffer, bytes);
}

BOOL gsEnable(DEVDATA *pdev)
{
	char	arg_device[32];
	char*	gsargv[] = {
		"gs",
		"-q",
		"-dBATCH",
		"-dNOPAUSE",
		"-dPARANOIDSAFER",
		arg_device,
		"-sOutputFile=-",
	};
	int		gsargc = sizeof(gsargv)/sizeof(gsargv[0]);

	memset(&pdev->gsdata, 0, sizeof(GSDATA));

#ifdef FILTER_NOT_PS2BMP
	DebugPrintf("== FILTER_NOT_PS2BMP ==\n");
#else	// #ifdef FILTER_NOT_PS2BMP

#ifdef _DEBUG
	{
		gsapi_revision_t	r;
		if (pdev->lib_gs.gsapi_revision(&r, sizeof(r)) == 0)
		{
			DebugPrintf("product  : %s\n", r.product);
			DebugPrintf("copyright: %s\n", r.copyright);
			DebugPrintf("revision : %d\n", r.revision);
		}
		else
		{
			DebugPrintf("revision structure size is incorrect");
		}
	}
#endif

	sprintf(arg_device, "-sDEVICE=%s", pdev->gsdevice);
	DebugPrintf("\t%s\n", arg_device);
	pdev->gsdata.exit_code = pdev->lib_gs.gsapi_new_instance(&pdev->gsdata.gsInstance, NULL);
	if (pdev->gsdata.exit_code == 0 || handleExit(pdev->gsdata.exit_code, 0))
	{
		pdev->lib_gs.gsapi_set_stdio(pdev->gsdata.gsInstance, &my_stdin, &my_stdout, &my_stderr);

		pdev->gsdata.exit_code = pdev->lib_gs.gsapi_init_with_args(pdev->gsdata.gsInstance, gsargc, gsargv);
		if (pdev->gsdata.exit_code == 0 || handleExit(pdev->gsdata.exit_code, 0))
		{
			pdev->lib_gs.gsapi_run_string_begin (pdev->gsdata.gsInstance, 0, &pdev->gsdata.exit_code);
		}
	}

	if (pdev->gsdata.exit_code && !handleExit(pdev->gsdata.exit_code, 1))
	{
		return FALSE;
	}
#endif	// #ifndef FILTER_NOT_PS2BMP

	return TRUE;
}

void gsDisable(DEVDATA *pdev)
{
#ifndef FILTER_NOT_PS2BMP
	if ( pdev->gsdata.gsInstance )
	{
		pdev->lib_gs.gsapi_set_stdio(pdev->gsdata.gsInstance, NULL, NULL, NULL);
		pdev->lib_gs.gsapi_exit(pdev->gsdata.gsInstance);

		DebugPrintf("CALL: gsapi_delete_instance()\n");
		pdev->lib_gs.gsapi_delete_instance(pdev->gsdata.gsInstance);
	}
#endif	// #ifndef FILTER_NOT_PS2BMP
	memset(&pdev->gsdata, 0, sizeof(pdev->gsdata));
}

BOOL gsClose(DEVDATA *pdev)
{
#ifndef FILTER_NOT_PS2BMP
	if (pdev->gsdata.exit_code == 0 || handleExit(pdev->gsdata.exit_code, 0))
	{
		DebugPrintf("CALL: gsapi_run_string_end()\n");
		pdev->lib_gs.gsapi_run_string_end (pdev->gsdata.gsInstance, 0, &pdev->gsdata.exit_code);
		if (pdev->gsdata.exit_code == 0 || handleExit(pdev->gsdata.exit_code, 1))
		{
			return TRUE;
		}
	}
	return FALSE;
#else
	return TRUE;
#endif	// #ifndef FILTER_NOT_PS2BMP
}

BOOL OutputDevmode(DEVDATA *pdev)
{
	BOOL				bRtn = TRUE;
#if defined(FILTER_NOT_PS2BMP) || defined(FILTER_NOT_BMP2TSPL)
#else
	pdev->dm.dmType = DM_HEADER_MARKER;
	pdev->dm.dmSize = sizeof(pdev->dm);
	pdev->dm.dmSizeExtra = 0;

	bRtn = fwrite(&pdev->dm, 1, sizeof(pdev->dm), stdout) > 0;
#endif
	return bRtn;
}

static int GSDLLCALL
my_stdin(void *instance, char *buf, int len)
{
	int	ch;
	int	count = 0;

//	DebugPrintf("STDIN: , instance=%p, len=%d bytes\n", instance, len);

	while (count < len)
	{
		ch = fgetc(stdin);
		if (ch == EOF)
			return 0;
		*buf++ = ch;
		count++;
		if (ch == '\n')
			break;
	}
	return count;
}

static int GSDLLCALL
my_stdout(void *instance, const char *str, int len)
{
//	DebugPrintf("STDOUT: %d bytes, instance=%p\n", len, instance);
//	DebugWrite(str, len);
	return len;
}

static int GSDLLCALL
my_stderr(void *instance, const char *str, int len)
{
//	DebugPrintf("STDERR:");
//	DebugWrite(str, len);
	return len;
}

static int handleExit(int code, int outerr)
{
	if ( code>=0 )
		return 1;
	else if ( code <= -100 )
	{
		switch (code)
		{
			case e_Fatal:
				// fatal internal error
			    if ( outerr )
			    {
					Error_Log(LEVEL_ERROR, "GSLIB fatal internal error\n");
				}
				return 0;
			case e_ExecStackUnderflow:
				// stack overflow
			    if ( outerr )
			    {
					Error_Log(LEVEL_ERROR, "GSLIB stack overflow\n");
				}
				return 0;
			// no error or not important
			case e_Quit:
			case e_Info:
			case e_InterpreterExit:
			case e_RemapColor:
			case e_NeedInput:
			case e_NeedStdin:
			case e_NeedStdout:
			case e_NeedStderr:
			case e_VMreclaim:
			default:
				return 1;
		}
	}
	else
	{
		if ( outerr )
		{
			const char* errors[]= { "", ERROR_NAMES };
			Error_Log(LEVEL_ERROR, "GSLIB %d, %s\n\n", code, errors[-code]);
		}
		return 0;
	}
}
