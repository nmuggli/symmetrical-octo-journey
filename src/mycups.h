/*
 * "mycups.h 2021-05-17 15:55:05
 *  
 *  cups routine declaration for TSC Printer Driver
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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

cups_file_t *my_cupsTempFile2(char *filename, int len);
int my_cupsFileClose(cups_file_t *fp);
int my_cupsFileFlush(cups_file_t *fp);
size_t my_cupsFileGetLine(cups_file_t *fp, char *buf, size_t buflen);
cups_file_t *my_cupsFileOpen(const char *filename, const char *mode);
cups_file_t *my_cupsFileOpenFd(int fd, const char *mode);
ssize_t my_cupsFileRead(cups_file_t *fp, char *buf, size_t bytes);
off_t my_cupsFileRewind(cups_file_t *fp);
off_t my_cupsFileSeek(cups_file_t *fp, off_t pos);
cups_file_t *my_cupsFileStdin(void);
off_t my_cupsFileTell(cups_file_t *fp);
ssize_t	my_cupsFileWrite(cups_file_t *fp, const char *buf, size_t bytes);

int my_cupsArrayAdd(cups_array_t *a, void *e);
void my_cupsArrayClear(cups_array_t *a);
int	my_cupsArrayCount(cups_array_t *a);
void *my_cupsArrayCurrent(cups_array_t *a);
void *my_cupsArrayFirst(cups_array_t *a);
void *my_cupsArrayIndex(cups_array_t *a, int n);
void *my_cupsArrayLast(cups_array_t *a);
cups_array_t *my_cupsArrayNew(cups_array_func_t f, void *d);
cups_array_t *my_cupsArrayNew2(cups_array_func_t f, void *d, cups_ahash_func_t h, int hsize);


int my_ppdLocalize(ppd_file_t *ppd);
cups_lang_t *my_cupsLangDefault(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */
