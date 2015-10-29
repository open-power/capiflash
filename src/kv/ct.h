/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/ct.h $                                                 */
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
#ifndef __CT_H__
#define __CT_H__

#define C_Reset   "\x1b[0m"
#define C_Black   "\x1b[30m"
#define C_Red     "\x1b[31m"
#define C_Green   "\x1b[32m"
#define C_Yellow  "\x1b[33m"
#define C_Blue    "\x1b[34m"
#define C_Cyan    "\x1b[35m"
#define C_Magenta "\x1b[36m"
#define C_White   "\x1b[37m"

char *c_strings[9] = { C_Reset   , C_Black   , C_Red     , C_Green   , C_Yellow  , C_Blue    , C_Cyan    , C_Magenta , C_White};

#endif

