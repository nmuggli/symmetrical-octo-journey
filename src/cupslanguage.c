/*
 * "cupslanguage.c 2021-05-17 15:55:05
 *  
 *  cups language for TSC Printer Driver
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

#include "config.h"
#include "common.h"
#include "debug.h"

#include "cupsinc/cups.h"
#include "cupsinc/language.h"
#include "mycups.h"

/*
 * Local functions...
 */

static void		ppd_ll_CC(char *ll_CC, int ll_CC_size,
			          char *ll, int ll_size);
static ppd_attr_t	*ppd_localized_attr(ppd_file_t *ppd,
			                    const char *keyword,
			                    const char *spec, const char *ll_CC,
				            const char *ll);

int					/* O - 0 on success, -1 on error */
my_ppdLocalize(ppd_file_t *ppd)		/* I - PPD file */
{
	return 0;
}

cups_lang_t *				/* O - Language data */
my_cupsLangDefault(void)
{
  return NULL;
}
