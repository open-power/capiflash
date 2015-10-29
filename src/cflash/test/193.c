/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/193.c $                                       */
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

//int MAX_LIMIT=1;

int change_nchan( int nchan )
{
    int rc;
    char str[80];
    char *disk_name = (char *)getenv("FLASH_DISK");
    sprintf(str, "chdev -l %s -attr nchn=%d", disk_name,nchan);
    rc=system(str);
    if ( rc != 0 )
        fprintf(stderr,"changing nchn value failed");
    return rc;
}


int max_ctx_n_res_nchan(int nchan)
{
    int rc;
    int i;
    pthread_t threads[MAX_OPENS];
    struct ctx *p_ctx;
    int MAX_CTX;

    if (mc_init() !=0 )
    {
        fprintf(stderr, "mc_init failed.\n");
        return -1;
    }

    debug("mc_init success.\n");

    rc = posix_memalign((void **)&p_ctx, 0x1000, sizeof(struct ctx)*MAX_OPENS);
    if (rc != 0)
    {
        fprintf(stderr, "Can not allocate ctx structs, errno %d\n", errno);
        return -1;
    }

    //Creating threads for ctx_init with nchan value
    // calculate how many ctx can be created

    MAX_CTX=MAX_OPENS-nchan;

    for (i = 0; i < MAX_CTX; i++)
    {
        rc = pthread_create(&threads[i],NULL, &max_ctx_res, (void *)&p_ctx[i]);
        if (rc)
        {
            fprintf(stderr, "Error creating thread %d, errno %d\n", i, errno);
            free(p_ctx);
            return -1;
        }
    }

    //joining
    for (i = 0; i < MAX_CTX; i++)
    {
        pthread_join(threads[i], NULL);
    }

    for (i = 0; i < MAX_CTX; i++)
    {
        ctx_close(&p_ctx[i]);
    }
    free(p_ctx);
    rc = g_error;
    g_error = 0;
    return rc;
}


int ioctl_7_1_193_1( int flag )
{

    return 1; // test case need to re-implemented

    int rc,i;

                    // Create MAX context & resource handlers based on default Nchn value
                            // first change the nchan value
    rc=change_nchan(atoi(getenv("NCHAN_VALUE")));
    CHECK_RC(rc, "changing nchan value failed");

    // now create max context and max resource and after than detach
    //rc=test_max_ctx_n_res_nchan("NCHAN_VALUE");   TBD no defination of the function
    //CHECK_RC(rc, "creating max context and resource handler failed");

    // change the nchan value again
    for ( i=1; i<=atoi(getenv("MAX_NCHAN_VALUE")); i++ )
    {
        rc=change_nchan(atoi(getenv("NCHAN_VALUE"))+i);
        CHECK_RC(rc, "changing nchan value failed");
        // now again create max context and max resource and after than detach
        //rc=test_max_ctx_n_res_nchan(atoi(getenv("NCHAN_VALUE"))+1); TBD no defination of the function
        CHECK_RC(rc, "creating max context and resource handler failed");
    }

    return rc;

}
// this is 5ht proicedure in 7.1.193. This is written in different case, as dont make sense with super pipe I/O
int ioctl_7_1_193_2( int flag )
{
    struct flash_disk disks[MAX_FDISK];
    int cfdisk = MAX_FDISK,rc,i;

    cfdisk = get_flash_disks(disks, FDISKS_ALL);
    // need to check the number of disks
    if (cfdisk < 2)
    {
        fprintf(stderr,"Must have 2 flash disks..\n");
        return -1;
    }
    // change nchan on one of the disk, using the first disk to 1 and other as default
    rc=change_nchan(1);
    // initiate tradition IO on both the disk.

    for ( i=0; i < 2 ; i++ )
    {
        if (0 == fork()) //child process
        {
            rc = traditional_io(i);
        }
        exit(rc);
    }
    return 0;
}



