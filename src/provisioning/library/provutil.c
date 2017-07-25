/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/provisioning/library/provutil.c $                         */
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


/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <unistd.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <provutil.h>

/*----------------------------------------------------------------------------*/
/* Function Prototypes                                                        */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/
int32_t                 g_traceE = 1;   /* error traces */
int32_t                 g_traceI = 0;       /* informative 'where we are in code' traces */
int32_t                 g_traceF = 0;       /* function exit/enter */
int32_t                 g_traceV = 0;       /* verbose trace...lots of information */
/*----------------------------------------------------------------------------*/
/* Struct / Typedef                                                           */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Defines                                                                    */
/*----------------------------------------------------------------------------*/

void prov_pretty_print(
                    uint8_t     *buffer,
                    uint32_t    buffer_length)
{
    /*------------------------------------------------------------------------*/
    /*  Local Variables                                                       */
    /*------------------------------------------------------------------------*/
    uint32_t                    offset = 0;
    uint32_t                    i;
    uint32_t                    j;
    char                        line_to_print[100];
    char                        sub_string[100];
    char                        char_to_print;
    uint32_t                    indent = 23;
    uint32_t                    secondary_indent = 23;
        
    /*-------------------------------------------------------------------------*/
    /*  Code                                                                   */
    /*-------------------------------------------------------------------------*/
    
    sub_string[0] = 0;
    line_to_print[0] = 0;
                                                                                                                                  
    for (j = 0; j < 80; j++)
    {
        line_to_print[j] = 0x20;
    }
    line_to_print[80] = 0;
                                                                                                                                  
    TRACED("UPPER-CASE ASCII        HEX \n");
    for (i = 0; i < buffer_length; i++)
    {
        // print byte in hex format
        sprintf (sub_string, " %.2x", buffer[i]);
                                                                                                                                  
        if (i < 16)
        {
            strncpy (&line_to_print[indent + (i % 16 * 3)], sub_string, 3);
        }
        else
        {
            strncpy (&line_to_print[secondary_indent + (i % 16 * 3)],
                     sub_string, 3);
        }

        // Only print letters in ASCII format
        char_to_print = buffer[i];

        if (((char_to_print >= 0x30) && (char_to_print <= 0x39)) ||
            ((char_to_print >= 65) && (char_to_print < 65 + 26)))
        {
            // do nothing.  Its a number or letter.
        }
        else
        {
            char_to_print = '.';
        }

        line_to_print[0 + (i % 16)] = char_to_print;    //change 60 to 0

        offset++;

        if ((i % 16) == 15)
        {
            strcat (line_to_print, "\n");
            TRACED("[0x%.4x]  %s",offset, line_to_print);

            for (j = 0; j < 80; j++)
            {
                line_to_print[j] = 0x20;
            }
            line_to_print[80] = 0;

            sprintf (sub_string, "%.4x", offset);
            strncpy (&line_to_print[0], sub_string, 4);
        }

    }

    /* Make sure any partial last lines get printed.
     */
    if (((i - 1) % 16) != 15)
    {
        strcat (line_to_print, "\n");
        TRACED("[0x%.4x]  %s", offset, line_to_print);
    }
}
