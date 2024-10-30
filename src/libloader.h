/*
 * "libloader.h 2021-05-17 15:55:05
 *  
 *  libcups load routine declaration for TSC Printer Driver
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


#ifndef _LIBLOADER_H_
#define _LIBLOADER_H_

#include "cupsinc/cups.h"
#include "cupsinc/ppd.h"
#include "cupsinc/file.h"
#include "cupsinc/array.h"
#include "cupsinc/language.h"
#include "cupsinc/i18n.h"

// libgs header
#include "gsinc/iapi.h"
#include "gsinc/gdevdsp.h"
#include "gsinc/ierrors.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * CUPS Functions...
 */

/************ cups.h (version 1.2) ************/
typedef int	(GSDLLAPIPTR PFN_cupsGetDests)(cups_dest_t **dests);
typedef void (GSDLLAPIPTR PFN_cupsSetDests)(int num_dests, cups_dest_t *dests);
typedef void (GSDLLAPIPTR PFN_cupsFreeDests)(int num_dests, cups_dest_t *dests);
typedef cups_dest_t	*(GSDLLAPIPTR PFN_cupsGetDest)(const char *name, const char *instance,
			             int num_dests, cups_dest_t *dests);
typedef void (GSDLLAPIPTR PFN_cupsFreeOptions)(int num_options, cups_option_t *options);
typedef const char *(GSDLLAPIPTR PFN_cupsGetOption)(const char *name, int num_options,
			               cups_option_t *options);
typedef int	(GSDLLAPIPTR PFN_cupsAddOption)(const char *name, const char *value,
			              int num_options, cups_option_t **options);
typedef int	(GSDLLAPIPTR PFN_cupsParseOptions)(const char *arg, int num_options,
			                 cups_option_t **options);
typedef int	(GSDLLAPIPTR PFN_cupsMarkOptions)(ppd_file_t *ppd, int num_options,
			                cups_option_t *options);
typedef const char	*(GSDLLAPIPTR PFN_cupsGetPPD)(const char *printer);
typedef cups_file_t	*(GSDLLAPIPTR PFN_cupsTempFile2)(char *filename, int len);
	

/************ ppd.h (version 1.2) ************/
typedef ppd_file_t	*(GSDLLAPIPTR PFN_ppdOpenFile)(const char *filename);
typedef void (GSDLLAPIPTR PFN_ppdClose)(ppd_file_t *ppd);
typedef void (GSDLLAPIPTR PFN_ppdMarkDefaults)(ppd_file_t *ppd);
typedef ppd_option_t *(GSDLLAPIPTR PFN_ppdFindOption)(ppd_file_t *ppd, const char *keyword);
typedef ppd_choice_t *(GSDLLAPIPTR PFN_ppdFindChoice)(ppd_option_t *o, const char *option);
typedef int	(GSDLLAPIPTR PFN_ppdMarkOption)(ppd_file_t *ppd, const char *keyword,
			              const char *option);
typedef ppd_choice_t *(GSDLLAPIPTR PFN_ppdFindMarkedChoice)(ppd_file_t *ppd, const char *keyword);
typedef ppd_size_t *(GSDLLAPIPTR PFN_ppdPageSize)(ppd_file_t *ppd, const char *name);
typedef ppd_attr_t	*(GSDLLAPIPTR PFN_ppdFindAttr)(ppd_file_t *ppd, const char *name, const char *spec);
typedef ppd_attr_t	*(GSDLLAPIPTR PFN_ppdFindNextAttr)(ppd_file_t *ppd, const char *name, const char *spec);
typedef int		(GSDLLAPIPTR PFN_ppdIsMarked)(ppd_file_t *ppd, const char *keyword,
			            const char *option);
typedef int		(GSDLLAPIPTR PFN_ppdLocalize)(ppd_file_t *ppd);

/************ file.h (version 1.2) ************/
typedef int		(GSDLLAPIPTR PFN_cupsFileClose)(cups_file_t *fp);
typedef size_t		(GSDLLAPIPTR PFN_cupsFileGetLine)(cups_file_t *fp, char *buf,
			                size_t buflen);
typedef cups_file_t	*(GSDLLAPIPTR PFN_cupsFileOpen)(const char *filename, const char *mode);
typedef cups_file_t	*(GSDLLAPIPTR PFN_cupsFileOpenFd)(int fd, const char *mode);
typedef cups_file_t	*(GSDLLAPIPTR PFN_cupsFileStdin)(void);
typedef ssize_t (GSDLLAPIPTR PFN_cupsFileWrite)(cups_file_t *fp, const char *buf, size_t bytes);
typedef off_t (GSDLLAPIPTR PFN_cupsFileTell)(cups_file_t *fp);
typedef ssize_t		(GSDLLAPIPTR PFN_cupsFileRead)(cups_file_t *fp, char *buf, size_t bytes);
typedef off_t		(GSDLLAPIPTR PFN_cupsFileSeek)(cups_file_t *fp, off_t pos);
typedef off_t		(GSDLLAPIPTR PFN_cupsFileRewind)(cups_file_t *fp);


/************ array.h (version 1.2) ************/
typedef int		(GSDLLAPIPTR PFN_cupsArrayAdd)(cups_array_t *a, void *e);
typedef cups_array_t	*(GSDLLAPIPTR PFN_cupsArrayNew)(cups_array_func_t f, void *d);
typedef int		(GSDLLAPIPTR PFN_cupsArrayCount)(cups_array_t *a);
typedef void		*(GSDLLAPIPTR PFN_cupsArrayFirst)(cups_array_t *a);
typedef void		*(GSDLLAPIPTR PFN_cupsArrayLast)(cups_array_t *a);
typedef void		*(GSDLLAPIPTR PFN_cupsArrayIndex)(cups_array_t *a, int n);

/************ language.h (version 1.2) ************/
typedef cups_lang_t	*(GSDLLAPIPTR PFN_cupsLangDefault)(void);
typedef void (GSDLLAPIPTR PFN_cupsLangFree)(cups_lang_t *lang);

typedef struct _CUPSLIB_FUNCTION {
	void						*hmodule;

	// cups.h
	PFN_cupsGetDests			cupsGetDests;
	PFN_cupsSetDests			cupsSetDests;
	PFN_cupsFreeDests			cupsFreeDests;
	PFN_cupsGetDest				cupsGetDest;
	PFN_cupsAddOption			cupsAddOption;
	PFN_cupsParseOptions		cupsParseOptions;
	PFN_cupsMarkOptions			cupsMarkOptions;
	PFN_cupsGetOption			cupsGetOption;
	PFN_cupsFreeOptions			cupsFreeOptions;
	PFN_cupsGetPPD				cupsGetPPD;
	PFN_cupsTempFile2			cupsTempFile2;

	// ppd.h
	PFN_ppdOpenFile				ppdOpenFile;
	PFN_ppdClose				ppdClose;
	PFN_ppdMarkDefaults			ppdMarkDefaults;
	PFN_ppdFindOption			ppdFindOption;
	PFN_ppdFindChoice			ppdFindChoice;
	PFN_ppdMarkOption			ppdMarkOption;
	PFN_ppdFindMarkedChoice		ppdFindMarkedChoice;
	PFN_ppdPageSize				ppdPageSize;
	PFN_ppdFindAttr				ppdFindAttr;
	PFN_ppdFindNextAttr			ppdFindNextAttr;
	PFN_ppdIsMarked				ppdIsMarked;
	PFN_ppdLocalize				ppdLocalize;

	// file.h
	PFN_cupsFileClose			cupsFileClose;
	PFN_cupsFileGetLine			cupsFileGetLine;
	PFN_cupsFileOpen			cupsFileOpen;
	PFN_cupsFileOpenFd			cupsFileOpenFd;
	PFN_cupsFileStdin			cupsFileStdin;
	PFN_cupsFileWrite			cupsFileWrite;
	PFN_cupsFileTell			cupsFileTell;
	PFN_cupsFileRead			cupsFileRead;
	PFN_cupsFileSeek			cupsFileSeek;
	PFN_cupsFileRewind			cupsFileRewind;
	
	// array.h
	PFN_cupsArrayAdd			cupsArrayAdd;
	PFN_cupsArrayNew			cupsArrayNew;
	PFN_cupsArrayCount			cupsArrayCount;
	PFN_cupsArrayFirst			cupsArrayFirst;
	PFN_cupsArrayLast			cupsArrayLast;
	PFN_cupsArrayIndex			cupsArrayIndex;

	// language.h
	PFN_cupsLangDefault			cupsLangDefault;
	PFN_cupsLangFree			cupsLangFree;

} CUPSLIB_FUNCTION, * LPCUPSLIB_FUNCTION;

typedef struct _GSLIB_FUNCTION{
	void								*hmodule;

	PFN_gsapi_revision					gsapi_revision;
	PFN_gsapi_new_instance				gsapi_new_instance;
	PFN_gsapi_delete_instance			gsapi_delete_instance;
	PFN_gsapi_set_stdio					gsapi_set_stdio;
	PFN_gsapi_set_poll					gsapi_set_poll;
	PFN_gsapi_set_display_callback		gsapi_set_display_callback;
	PFN_gsapi_init_with_args			gsapi_init_with_args;
	PFN_gsapi_run_string				gsapi_run_string;
	PFN_gsapi_run_string_with_length	gsapi_run_string_with_length;
	PFN_gsapi_run_string_begin			gsapi_run_string_begin;
	PFN_gsapi_run_string_continue		gsapi_run_string_continue;
	PFN_gsapi_run_string_end			gsapi_run_string_end;
	PFN_gsapi_exit						gsapi_exit;

} GSLIB_FUNCTION, * LPGSLIB_FUNCTION;

extern int	LoadCupsLibrary(CUPSLIB_FUNCTION* cupsfun);
extern void	FreeCupsLibrary(CUPSLIB_FUNCTION* cupsfun);
extern int	LoadGsLibrary(GSLIB_FUNCTION* gsfun);
extern void	FreeGsLibrary(GSLIB_FUNCTION* gsfun);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif	// #ifndef _LIBLOADER_H_
