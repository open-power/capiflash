/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/174_175.c $                                   */
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

#include "cflash_test.h"
#include <pthread.h>

extern int g_error;
extern pid_t pid;

int ioctl_7_1_174_175( int flag )
{
    int rc=0,rc1=0;
    struct ctx myctx;
    struct ctx *p_ctx;

    struct flash_disk disks[MAX_FDISK];
    int cfdisk = MAX_FDISK;
    p_ctx=&myctx;
    cfdisk = get_flash_disks(disks, FDISKS_ALL);
    //need to check the number of disks
    if (cfdisk < 1)
    {
        debug("Must have 2 flash disks..\n");
        return -1;
    }

    switch ( flag )
    {
        case 1 :
            // 7.1.174
            // start super pipe io and then try to do traditional io.
            // traditional i/o should fail
            if ( 0 == fork())
            {
                rc=create_multiple_vluns(p_ctx);
                if ( rc != 0 )
                {
                    debug("create_multiple_vluns failed\n");
                    return 1;
                }
                exit(rc);
            }

            if ( 0 == fork())
            {
                rc=traditional_io(1);
                if ( rc != 0 )
                {
                    debug(" traditional I/O failed \n");
                    rc=0;
                }
                else
                {
                    debug("traditional io successful \n");
                    rc=1;
                }
                exit(rc);
            }
            wait4all();
            return rc;

            // 7.1.175
        case 2 :
            // start traditional I/O and then try to create multiple context. context creation should fail
            if ( 0 == fork())
            {
                rc=traditional_io(1);
                if ( rc != 0 )
                {
                    debug(" traditional I/O failed \n");
                }
                else
                {
                    debug("traditional io successful \n");
                    rc=0;
                }
                exit(rc);
            }
            sleep(6);
            // try to create luns with multiple context. all should fail

            rc1=create_multiple_vluns(p_ctx);
            if ( rc1 == 0 )
            {
                debug(" creation of VLUN succeeded, should have failed");
                return 1;
            }

            wait4all();
            return 0;

    }
    return 0;
}
