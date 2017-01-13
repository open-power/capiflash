/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/cflash_eras.h $                                   */
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

#ifndef _H_CFLASH_ERAS_H
#define _H_CFLASH_ERAS_H


#include <sys/types.h>
#include <inttypes.h>
#if !defined(_AIX) && !defined(_MACOSX)
#include <linux/types.h>
#endif /* !_AIX && !_MACOSX */

typedef uint32_t eye_catch4b_t;
/*
 * Macros to define eye-catchers using character literals
 */

#define __EYEC2(__a,__b) (((__a)<< 8) | (__b))
#define __EYEC4(__a,__b,__c,__d) ((__EYEC2(__a,__b) << 16) | __EYEC2(__c,__d))
#define __EYEC8(__a,__b,__c,__d,__e,__f,__g,__h) (((unsigned long long)__EYEC4(__a,__b,__c,__d) << 32) | __EYEC4(__e,__f,__g,__h))


#endif /* _H_CFLASH_ERAS_H */
