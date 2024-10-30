/*
 * "main.c 2021-05-17 15:55:05
 *  
 *  main filter entry for TSC Printer Driver
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

/*
	argc = 6 or 7
	argv[0] = 
	argv[1] = Job ID
	argv[2] = User ID
	argv[3] = Job Name
	argv[4] = Copies
	argv[5] = job-uuid
	argv[6] = cups options
*/
int main(int argc, char *argv[], char *env[])
{
	int			fd[2];
	pid_t		childpid;

	// Make sure status messages are not buffered...
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	DebugPrintf("Start TSC Printer Filter on %s\n", argv[0]);

	// Make sure we have the right number of arguments for CUPS!
	if (argc < 6 || argc > 7)
	{
		Error_Log(LEVEL_ERROR, "Usage: %s job user title copies options [filename]\n", argv[0]);
		return 0;
	}

	if ( pipe(fd) != -1 && (childpid=fork()) != -1)
	{
		if ( childpid == 0 )
		{
			// Child process cloes up input side of pipe
			close(fd[0]);

			// redirect stdout to pipe fd[1].
			if ( dup2(fd[1], fileno(stdout)) != -1 )
			{
				ps2bmp(argc, argv);
			}

			// Child process cloes up output side of pipe
			close(fd[1]);
		}
		else
		{
			int		nError;

			// Parent process cloes up output side of pipe
			close(fd[1]);
#if defined(FILTER_NOT_PS2BMP) || defined(FILTER_NOT_BMP2TSPL)
			{
				int		nBytes;
				int		nTotal = 0;
				char	buffer[4096];

				while(1)
				{
					nBytes = read(fd[0], buffer, sizeof(buffer));
					if ( nBytes <= 0 )
					{
						break;
					}
				    fwrite(buffer, 1, nBytes, stdout);
					nTotal += nBytes;
				}
			}
#else
			bmp2tspl(fd[0]);
#endif
			DebugPrintf("End TSC Printer Filter on %s\n", argv[0]);

			// Parent process cloes up input side of pipe
			close(fd[0]);
		}
	}

	return 0;
}
