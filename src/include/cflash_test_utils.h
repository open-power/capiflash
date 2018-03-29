/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/include/cflash_test_utils.h $                             */
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
#ifndef _H_CFLASH_TEST_UTILS
#define _H_CFLASH_TEST_UTILS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>

#include <errno.h>

/**
 *******************************************************************************
 * \brief
 * bld the cmd to inject EEH to dev (/dev/sgX)
 ******************************************************************************/
static inline void get_inject_EEH_cmd(char *dev, char* cmd)
{
#ifndef _AIX
    char devstr[128] = {0};

    /* if path is a symlink, follow it to allow persistent names */
    if (dev)
    {
        char *tpath=realpath(dev, NULL);
        if (tpath) {sprintf(devstr, "%s", tpath+5);}
        else
        {
            snprintf(devstr, strlen(dev)-4, "%s", dev+5);
        }
    }
    sprintf(cmd, "echo 0x8000000000000000 > /sys/kernel/debug/powerpc/"
                 "$(find /sys | grep $(find "
                 "/sys|grep %s|head -1|awk -F/ \'{print $5}\')|grep afu|grep "
                 "reset|awk -F/ \'{print $4}\'|awk -F: \'{print $1}\'|"
                 "tr /a-z/ /A-Z/)/err_injct_outbound", devstr);
#endif
    return;
}

/**
 *******************************************************************************
 * \brief
 ******************************************************************************/
static inline int inject_EEH(char *dev)
{
    int  rc         = 0;
    char sgdev[128] = {0};
    char cmd[512]   = {0};

    snprintf(sgdev, strlen(dev)-4, "%s", dev+5);

    get_inject_EEH_cmd(dev,cmd);

    printf("injecting EEH for %s", dev);
    rc=system(cmd);
    printf(" ...done\n");

    return rc;
}

#endif
