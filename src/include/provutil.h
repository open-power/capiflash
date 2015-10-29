/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/provutil.h $                                      */
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

#ifndef _PROVUTIL_H
#define _PROVUTIL_H
/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <stdint.h>
/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/* Globals                                                                    */
/*----------------------------------------------------------------------------*/

extern int32_t                 g_traceE;       /* error traces */
extern int32_t                 g_traceI;       /* informative 'where we are in code' traces */
extern int32_t                 g_traceF;       /* function exit/enter */
extern int32_t                 g_traceV;       /* verbose trace...lots of information */

/*----------------------------------------------------------------------------*/
/* Defines                                                                    */
/*----------------------------------------------------------------------------*/

#define TRACEE(FMT, args...) if(g_traceE) \
do \
{  \
char __data__[256];                                             \
memset(__data__,0,256);                                         \
sprintf(__data__,"%s %s: " FMT, __FILE__, __func__, ## args);   \
perror(__data__);                                               \
} while(0)

#define TRACEF(FMT, args...) if(g_traceF) \
{ \
printf("%s %s: " FMT, __FILE__, __func__ ,## args); \
}

#define TRACEI(FMT, args...) if(g_traceI) \
{ \
printf("%s %s: " FMT, __FILE__, __func__ ,## args); \
}

#define TRACEV(FMT, args...) if(g_traceV) printf("%s %s: " FMT, __FILE__, __func__ ,## args)

#define TRACED(FMT, args...) printf(FMT,## args)

#if 0

#define TRACEE(FMT, args...)
#define TRACEF(FMT, args...)
#define TRACEI(FMT, args...)
#define TRACEV(FMT, args...)
#define TRACED(FMT, args...)

#endif

/*----------------------------------------------------------------------------*/
/* Function Prototypes                                                        */
/*----------------------------------------------------------------------------*/


void prov_pretty_print(uint8_t *buffer,
                       uint32_t buffer_length);


#endif
