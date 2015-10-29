/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/209.c $                                       */
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


int ioctl_7_1_209( int flag )
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx;
    char *disk_name, *str=NULL;
    struct flash_disk disks[MAX_FDISK];  // flash disk struct


    pid = getpid();
    str = (char *) malloc(100);
    p_ctx=&myctx;
    get_flash_disks(disks, FDISKS_ALL);
    pid = getpid();

    disk_name = strtok(disks[0].dev,"/");
    disk_name = strtok(NULL,"/");

    // create a VG on flash disk and then varyoff it
    sprintf(str, "mkvg -f -y NEW_VG %s", disk_name);
    rc=system(str);

    if ( rc !=0 )
        return 1;

    rc=system("lspv | grep NEW_VG | grep active");
    /*if ( rc !=0 )
        return 2;*/
    // varyoff vg
    rc|=system("varyoffvg NEW_VG; exportvg NEW_VG");
    /*if ( rc !=0 )
        return 3;*/
    // now try creating super pipes
    sprintf(str, "chdev -l %s -a pv=clear", disk_name);
    rc|=system(str);
    if ( rc !=0 ) return 3;
    rc=create_multiple_vluns(p_ctx);
    if ( rc != 0 )
        flag=1;
    rc=create_direct_lun(p_ctx);
    if ( rc != 0 )
        flag=2;
    if ( flag == 1 )
    {
        printf( "some error with LUN_VIRTUAL \n");
        return 1;
    }
    if ( flag == 2 )
    {
        printf("some error with LUN_DIRECT \n");
        return 1;
    }

    printf("all case succeeded \n");
    return 0;
}


