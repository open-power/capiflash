/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/common/cflash_tools_user.c $                              */
/*                                                                        */
/* IBM Data Engine for NoSQL - Power Systems Edition User Library Project */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2014,2015                        */
/* [+] International Business Machines Corp.                              */
/*                                                                        */
/*                                                                        */
/* Licensed under the Apache License, Version 2.0 (the "License");        */
/* you may not use this file except in compliance with the License.       */
/* You may obtain a copy of the License at                                */
/*                                                                        */
/*     http://www.apache.org/licenses/LICENSE-2.0                         */
/*                                                                        */
/* Unless required by applicable law or agreed to in writing, software    */
/* distributed under the License is distributed on an "AS IS" BASIS,      */
/* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or        */
/* implied. See the License for the specific language governing           */
/* permissions and limitations under the License.                         */
/*                                                                        */
/* IBM_PROLOG_END_TAG                                                     */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <inttypes.h>
#if !defined(_AIX) && !defined(_MACOSX)
#include <linux/types.h>
#endif /* !_AIX && !_MACOSX */

#include <cflash_tools_user.h>

/*
 * NAME:        dumppage
 *
 * FUNCTION:    Dumps a page of data to the screen.
 *
 *
 * INPUTS:
 *              buffer    - Buffer to dump data.
 *              size      - Size of buffer.
 *
 * RETURNS:
 *              0 for good completion,  ERRNO on error
 *              
 */
void
dumppage(void *buffer, int size)
{
    int i;

    for (i =0 ; i<size; i+=16){
#ifndef _AIX
	fprintf(stderr, "%8lx %8x %8x %8x %8x\n",
		((uint64_t)buffer+(i)),
#else
#ifdef __64BIT__
	fprintf(stderr, "%8llx %8x %8x %8x %8x\n",
		((uint64_t)buffer+(i)),
#else
	fprintf(stderr, "%8x %8x %8x %8x %8x\n",
		((uint32_t)buffer+(i)),
#endif /* !__64BIT__ */
#endif /* _AIX */
		*(uint32_t  *)(buffer+(i)),
		*(uint32_t  *)(buffer+(i) +4),
		*(uint32_t  *)(buffer+(i) +8),
		*(uint32_t  *)(buffer+(i) +12));
    }

}


/*
 * NAME: hexdump
 *                                                                    
 * FUNCTION: Display an array of type char in ASCII, and HEX. This function
 *      adds a caller definable header to the output rather than the fixed one
 *      provided by the hexdump function.
 *                                                                    
 * EXECUTION ENVIRONMENT:
 *                                                                   
 *      This routine is ONLY AVAILABLE IF COMPILED WITH DEBUG DEFINED
 *                                                                   
 * RETURNS: NONE
 */  
void
hexdump(void *data, long len, const char *hdr)
{

        int     i,j,k;
        char    str[18];
        char    *p = (char *)data;

        i=j=k=0;
        fprintf(stderr, "%s: length=%ld\n", hdr?hdr:"hexdump()", len);

        /* Print each 16 byte line of data */
        while (i < len)
        {
                if (!(i%16))    /* Print offset at 16 byte bndry */
                        fprintf(stderr,"%03x  ",i);

                /* Get next data byte, save ascii, print hex */
                j=(int) p[i++];
                if (j>=32 && j<=126)
                        str[k++] = (char) j;
                else
                        str[k++] = '.';
                fprintf(stderr,"%02x ",j);

                /* Add an extra space at 8 byte bndry */
                if (!(i%8))
                {
                        fprintf(stderr," ");
                        str[k++] = ' ';
                }

                /* Print the ascii at 16 byte bndry */
                if (!(i%16))
                {
                        str[k] = '\0';
                        fprintf(stderr," %s\n",str);
                        k = 0;
                }
        }

        /* If we didn't end on an even 16 byte bndry, print ascii for partial
         * line. */
        if ((j = i%16)) {
                /* First, space over to ascii region */
                while (i%16)
                {
                        /* Extra space at 8 byte bndry--but not if we
                         * started there (was already inserted) */
                        if (!(i%8) && j != 8)
                                fprintf(stderr," ");
                        fprintf(stderr,"   ");
                        i++;
                }
                /* Terminate the ascii and print it */
                str[k]='\0';
                fprintf(stderr,"  %s\n",str);
        }
        fflush(stderr);

        return;
}
