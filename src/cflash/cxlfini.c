/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/cxlfini.c $                                        */
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h> //isspace
#include <cxlfini.h>
#include <cxlfcommon.h>


//forward prototypes
bool section_is_valid(char* i_section);

char* trim_whitespace_inplace(char* io_buffer);

void trim_comments(char* io_buffer);

ini_dict_t* append_ini_entry(ini_dict_t** io_entry_list, char* section, char* key, char* value);



bool section_is_valid(char* i_section)
{
    bool l_rc = true;
    int i = 0;
    do
    {
        //if the pointer's invalid, the section is obv. invalid.
        if(i_section == NULL)
        {
            TRACEE("invalid input\n");
            l_rc = false;
            break;
        }
        //if the string is empty, it is not valid
        if(strlen(i_section) == 0)
        {
            l_rc = false;
            break;
        }
        //check each char to see if it's whitespace.
        for(i = 0; i < strlen(i_section); i++)
        {
            //if whitespace is found, isspace returns a non-zero
            if(isspace(i_section[i]) != 0)
            {
                TRACEE("Found whitespace in section title.\n");
                l_rc = false;
                break;
            }
            else if(i_section[i] == ';')
            {
                TRACEE("Found a comment char in the section title.\n");
                l_rc = false;
                break;
            }
        }
 
    } while (0);
    if((l_rc == false) && (i_section != NULL))
    {
        TRACEE("Section string '%s' is invalid.\n", i_section);
    }
    else if(l_rc ==true)
    {
        //TRACEV("Section string '%s' is valid.\n", i_section);
    }
    return l_rc;
}

char* trim_whitespace_inplace(char* io_buffer)
{
    bool l_found_char = false;
    char* l_start = io_buffer;
    int i=0;
    if(l_start != NULL)
    {
        //TRACEV("Input string to trim is '%s'\n",l_start);
        //search the string for white space
        for(i = 0; i<strlen(io_buffer); i++)
        {
            if(isspace(io_buffer[i]))
            {
                //if we have not found a valid character yet, we need to
                //effectively "left-trimm" space chars off the front of the string
                if(!l_found_char)
                {
                    //advance the front of the string
                    l_start++;
                    continue;
                }
                //we have at least one actual character, but found another
                //white space, so...
                else
                {
                    //mark it as the artificial end of the string
                    //and stop searching.
                    io_buffer[i] = '\0';
                    break;
                }
                
            }//end if(isspace(...))
            else
            {
                l_found_char = true;
            }
        } //end for
    } // end if
    return l_start;
}

void trim_comments(char* io_buffer)
{
    int i = 0;
    if(io_buffer != NULL)
    {
        //find the first space character, and replace it with a '\0' so that
        //we will terminate the shortened string early
        for(i = 0; i<strlen(io_buffer); i++)
        {
            if(io_buffer[i] == ';')
            {
                //found a comment char in the middle of a [ ] pair. This is bad.
                io_buffer[i] = '\0';
                //note we are guaranteeing that the input always has a null terminator.
                //if the semicolon is the last char in the string, it will be followed by a null.
                //therefore we will find io_buffer[i+1] = '\0'.
                //the only case where this can break is if the string is poorly-formatted, and the last char is
                //NOT a null. If that's happened, likely other stuff will break as well.
                //TRACEV("Found a comment char, trimming the remainder of the string: '%s'\n", &io_buffer[i+1]);
                break;
            }
        } //end for
    } // end if
    return;
}

ini_dict_t* append_ini_entry(ini_dict_t** io_entry_list, char* section, char* key, char* value)
{
    ini_dict_t* l_curr_entry = NULL;
    ini_dict_t* l_new_entry = NULL;
    if(io_entry_list == NULL)
    {
        TRACEE("Invalid input found.\n");
        l_new_entry = NULL;
    }
    else
    {
        //no list exists, so create a root node
        if(*io_entry_list == NULL)
        {
            *io_entry_list = malloc(sizeof(ini_dict_t));
            l_new_entry = *io_entry_list;
        }
        //a list exists, so we need to find the last non-empty node,
        //then add something to it.
        else
        {
            l_curr_entry = *io_entry_list;
            while((l_curr_entry != NULL) && (l_curr_entry->next != NULL))
            {
                l_curr_entry = l_curr_entry->next;
            }
            if(l_curr_entry == NULL)
            {
                TRACEE("Error occurred parsing ini entry structs - we fell off the end of the list.\n");
                l_new_entry = NULL;
            }
            else
            {
                l_new_entry = malloc(sizeof(ini_dict_t));
                l_curr_entry->next = l_new_entry;
            }
        }//end else - list exists
        
        //add the new entry!
        if(l_new_entry != NULL)
        {
            l_new_entry->section = strdup(section);
            l_new_entry->key = strdup(key);
            l_new_entry->value = strdup(value);
            l_new_entry->next = NULL;
        }
    }//end else - entry is valid
    return l_new_entry;
}

ini_dict_t* cxlfIniParse(char* i_inifilename, uint32_t* o_failed_line)
{
    FILE* l_file = NULL;
    ini_dict_t* l_first_entry = NULL;
    char l_curr_line[CXLF_MAX_LINE_SZ] = "";
    char l_section[CXLF_MAX_LINE_SZ] = "default";
    uint32_t l_line_num = 0;
    bool l_err = false;
    do
    {
        //do some basic input validation
        if((i_inifilename == NULL) || (o_failed_line == NULL))
        {
            TRACEE("Invalid argument %p or %p\n", i_inifilename, o_failed_line);
            break;
        }
        
        //open the file handle
        l_file = fopen(i_inifilename, "r");
        if(l_file == NULL)
        {
            TRACEE("Unable to find file '%s'\n",i_inifilename);
            l_err = true;
            break;
        }
        
        //read lines until we get to the end
        while(fgets(l_curr_line, CXLF_MAX_LINE_SZ, l_file) != NULL)
        {
            //advance our line counts = for humans, files start on line 1, not zero.
            l_line_num++;
            
            trim_comments(l_curr_line);
            if(strlen(l_curr_line) < 2)
            {
                //ini files are not allowed to have fewer than 2 characters ('a=')
                //if we find something that's shorter, consider it a comment or empty line
                //TRACEV("skipping line #%d.\n",l_line_num);
                //note we don't "continue" since we want to bump our line number counts...
            }
            else if(l_curr_line[0] == '[')
            {
                char* l_section_end = NULL;
                //found a section start element, so search for the matching end
                l_section_end = strchr(l_curr_line, ']');
                if(l_section_end == NULL)
                {
                    TRACEE("Parsing error: unmatched []'s found on line %d: '%s'", (uint32_t)l_line_num, l_curr_line);
                    l_err = true;
                    break;
                }
                else
                {
                    //artificially strip off the end ']' by marking it as a null char
                    *l_section_end = '\0';
                }
                //copy the section name out to our section buffer
                //note we +1 to avoid copying the leading '[' character
                strncpy(l_section, l_curr_line+1, CXLF_MAX_LINE_SZ);
                
                if(!section_is_valid(l_section))
                {
                    TRACEE("Error: section field was invalid. Unable to parse '%s'\n",l_curr_line);
                    l_err = true;
                    break;
                }
                TRACEV("New section: '%s'\n", l_section);
            }
            else
            {
                //TRACEV("Found a k-v pair line\n");
                //must be a key/value pair
                char* l_equalsign = NULL;
                char* l_value = NULL;
                char* l_key = NULL;
                l_equalsign = strchr(l_curr_line, '=');
                if(l_equalsign == NULL)
                {
                    TRACEE("No '=' found in key/value pair line. string = '%s'\n",l_curr_line);
                    l_err = true;
                    break;
                }
                //we must have a null terminator at the end of the string buffer
                //if we assume the = is the last char, then l_value will be a
                //null character.
                l_value = l_equalsign+1;
                //artificially stick a 'null' in the middle of the 'key = value' string.
                *l_equalsign = '\0';
                l_key = trim_whitespace_inplace(l_curr_line);
                if(strlen(l_key) == 0)
                {
                    TRACEE("Invalid key/value pair. Parsed and found key length == 0.\n");
                    l_err = true;
                    break;
                }
                l_value = trim_whitespace_inplace(l_value);
                //value is allowed to be empty
                append_ini_entry(&l_first_entry, l_section, l_key, l_value);
            }
            
        }//end while(fgets...)
        if(l_err)
        {
            TRACEE("Error occured parsing cfg file.\n");
            break;
        }
        
    } while (0);
    
    
    if(l_file)
    {
        //always close the file if it was opened
        fclose(l_file);
    }
    //if something broke, NULL and empty the entire linked list. it's input data
    //was not valid, so it (itself) is not valid.
    if(l_err)
    {
        cxlfIniFree(l_first_entry);
        l_first_entry = NULL;
        *o_failed_line = l_line_num;
    }
    return l_first_entry;
}



void cxlfIniFree(ini_dict_t* i_ini_entry)
{
    //Locals
    ini_dict_t* l_curr = i_ini_entry;
    ini_dict_t* l_next = NULL;
    
    while(l_curr != NULL)
    {
        //free the elements from our current entry
        free(l_curr->section);
        free(l_curr->key);
        free(l_curr->value);
        //free our book-keeping structures, but keep 'next' so we can proceed down the list
        l_next = l_curr->next;
        free(l_curr);
        //advance our way through the linked list
        l_curr = l_next;
        
    }
    
    return;
}

char* cxlfFindIniValue(ini_dict_t* i_ini_entry, char* i_section, char* i_key)
{
    //Locals
    ini_dict_t* l_curr = i_ini_entry;
    char* l_value = NULL;
    TRACEV("Searching for %s.%s \n", i_section, i_key); 
    //Code
    while(l_curr != NULL)
    {
        if((strcmp(l_curr->section, i_section) == 0) &&
           (strcmp(l_curr->key, i_key) == 0))
        {
            l_value = l_curr->value;
            //found the entry
            break;
        }
        //advance our way through the linked list
        l_curr = l_curr->next;
    }
    return l_value;
}


