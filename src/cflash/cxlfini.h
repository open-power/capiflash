/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/cxlfini.h $                                        */
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

/**
 * @file cxlfini.h
 * @brief Contains all internal headers for cxlfisioning afu logic
 *
 *
 */

#ifndef _CXLFINI_H
#define _CXLFINI_H
/*----------------------------------------------------------------------------*/
/* Includes                                                                   */
/*----------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <cxlfcommon.h>
/*----------------------------------------------------------------------------*/
/* Constants                                                                  */
/*----------------------------------------------------------------------------*/
#define CXLF_MAX_LINE_SZ 150
/*----------------------------------------------------------------------------*/
/* Structures                                                                 */
/*----------------------------------------------------------------------------*/


/* Each key=value of an INI file is parsed into an ini_element. The pointers 
 * here are dynamically-allocated, which means we must free() the indiv. element 
 * members prior to freeing the linked list overall! Destroy this list by calling 
 */ 
typedef struct ini_dict 
{ 
    char* section; 
    char* key; 
    char* value; 
    struct ini_dict* next; 
} ini_dict_t; 
 


/*----------------------------------------------------------------------------*/
/* Function Prototypes                                                        */
/*----------------------------------------------------------------------------*/


/**
 * @brief Free an ini_dict_t* structure and associated members
 * Frees the internal data structures that make up the ini dictionary that is
 * returned by cxlfIniParse. cxlfIniFree must be called to avoid memory leaks.
 * @param i_ini_entry ini_dict_t* to be freed.
 */

void cxlfIniFree(ini_dict_t* i_ini_entry);


/**
 * @brief Parse an ini file and create a dictionary of the found contents
 * This dynamically-allocates data structures to describe the contents of
 * an ini file. The caller MUST call cxlfIniFree() on the returned pointer
 * when the dictionary is no longer needed to avoid a memory leak.
 * @param i_inifilename file to parse
 * @param o_failed_line Line we failed to parse on (if any). Check this if
 *          the return value is NULL.
 * @returns a valid dictionary on sucess, NULL on failure.
 */
ini_dict_t* cxlfIniParse(char* i_inifilename, uint32_t* o_failed_line);


/**
 * @brief find a desired ini value, given a section and key
 * @param i_ini_entry  dictionary to process
 * @param i_section section header (if any) for the key. Enter "default" if no section is specified in the source file.
 * @param i_key key to be found
 * @returns valid char* pointer on success, or NULL on error or key / section not found. Note that empty strings e.g. "" may be returned if a key is present, but not set in the ini file.
 */
char* cxlfFindIniValue(ini_dict_t* i_ini_entry, char* i_section, char* i_key);


#endif 
