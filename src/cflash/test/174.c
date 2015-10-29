/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/174.c $                                       */
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
extern char cflash_path[MC_PATHLEN];
int MAX_LIMIT=1;

int test_traditional_IO( int flag, int disk_num )
{


    int rc;
    struct ctx myctx[20];
    struct ctx *p_ctx[20];
    char *disk_name,*disk_name1,temp[MC_PATHLEN], *str=NULL;
    __u64 chunk = 0x10;


    pid = getpid();
    str = (char *) malloc(100);

    pthread_t thread[20];
    __u64 nlba=0;
    __u64 flags=0;

    struct flash_disk disks[MAX_FDISK];  // flash disk struct
    strcpy(temp,cflash_path);
    get_flash_disks(disks, FDISKS_ALL);
    pid = getpid();
    disk_name = strtok(temp,"/");
    disk_name = strtok(NULL,"/");
    disk_name1  = strtok(disks[1].dev,"/");
    disk_name1 = strtok(NULL,"/");
    debug("cflash_path=%s\n", cflash_path);
    int i=0,j=0;


    switch ( flag )
    {

            // 7.1.180: Do rmdev -l hdisk# while super-pipe IO(root user)
        case 180 :
            // virtual LUN
            // create multiple vlun and do io
            i=0;
            p_ctx[i]=&myctx[i];
            rc = ctx_init(p_ctx[i]);
            //rc = ctx_init2(p_ctx, flash_dev, flags, path_id);
            CHECK_RC(rc, "Context init failed");
            //thread to handle AFU interrupt & events
            pthread_create(&thread[i], NULL, ctx_rrq_rx, p_ctx[i]);
            nlba=p_ctx[i]->chunk_size;
            rc = create_resource(p_ctx[i], nlba, flags, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            // do io on context
            // We wish to do IO in a different thread... Setting up for that !

            // try to detach the disk using rmdev command
            sprintf(str, "rmdev -l %s >rmdev.log 2>&1 ", disk_name);
            rc=system(str);

            if ( rc == 0 )
            {
                debug(" rmdev succeeded, though super pipe io was ongoing \n");
                rc=1;
            }
            else
            {
                debug("rmdev failed as expected \n");
                rc=system("cat rmdev.log | grep \"device is busy\"");
                if ( rc !=0 )
                {
                    debug("error message is not proper \n");
                    rc=1;
                }
                else
                {
                    debug(" error message was thrown properly \n");
                    rc=0;
                }
            }
            sprintf(str, "cfgmgr -l %s ", disk_name);
            system(str);
            // Wait for IO thread to complete
            break;

            // 7.1.180 with direct LUN. This is a new scenario, and not part of testcase. Repeat of 180 with DIRECT_LUN
        case 1801:
            // Direct LUN
            // create DIRECT_LUN and do io
            i=0;
            p_ctx[i]=&myctx[i];
            rc = ctx_init(p_ctx[i]);
            //rc = ctx_init2(p_ctx, flash_dev, flags, path_id);
            CHECK_RC(rc, "Context init failed");
            //thread to handle AFU interrupt & events
            nlba=p_ctx[i]->lun_size;
            rc = create_resource(p_ctx[i], nlba, flags, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");
            // do io on context
            // try to detach the disk using rmdev command
            sprintf(str, "rmdev -l %s >rmdev.log 2>&1  ", disk_name);
            debug("%s\n",str);
            rc=system(str);
            if ( rc == 0 )
            {
                debug(" rmdev succeeded, though super pipe io was ongoing \n");
                rc=1;
            }
            else
            {
                debug("rmdev failed as expected \n");
                rc=system("cat rmdev.log | grep \"device is busy\"");
                if ( rc !=0 )
                {
                    debug("error message is not proper \n");
                    rc=1;
                }
                else
                {
                    debug(" error message was thrown properly \n");
                    rc=0;
                }
            }
            sprintf(str, "cfgmgr -l %s ", disk_name);
            system(str);

            break;

            // 7.1.181
            // Test lsmpio, devrsrv & iostat commands while super-pipe IO

        case 181:
            // create VLUns
            i=0;
            p_ctx[i]=&myctx[i];

            rc = ctx_init(p_ctx[i]);
            //rc = ctx_init2(p_ctx, flash_dev, flags, path_id);
            CHECK_RC(rc, "Context init failed");
            //thread to handle AFU interrupt & events
            pthread_create(&thread[i], NULL, ctx_rrq_rx, p_ctx[i]);
            nlba=p_ctx[i]->chunk_size;
            rc = create_resource(p_ctx[i], nlba, flags, LUN_DIRECT);
            sleep(10);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");
            rc=0;
            // now do lsmpio on flash disk
            sprintf(str, "lsmpio -l %s >/tmp/lsmpio.log 2>&1", disk_name);
            rc=system(str);

            if ( rc != 0 )
                rc=1;
            else
            {   sprintf(str, "grep %s /tmp/lsmpio.log", disk_name);
                rc=system(str);

                if ( rc !=0 )
                    rc=2;
            }

            // devrsrv
            sprintf(str, "devrsrv -c query -l %s>/tmp/devrsrv.log 2>&1", disk_name);
            rc=system(str);

            if ( rc != 0 )
                rc=1;
            else
            {   sprintf(str, "grep %s /tmp/devrsrv.log", disk_name);
                rc=system(str);

                if ( rc !=0 )
                    rc=2;
            }

            // iostat
            sprintf(str, "iostat %s>/tmp/iostat.log 2>&1", disk_name);
            rc=system(str);

            if ( rc != 0 )
                rc=1;
            else
            {
                sprintf(str, "grep %s /tmp/iostat.log", disk_name);
                rc=system(str);

                if ( rc !=0 )
                    rc=2;
            }


            break;

            // 7.1.182
            // Test VLUN creation, if flash disk inactive opened
        case 182:
            // create a VG on flash disk and then varyoff it
            sprintf(str, "mkvg -f -y NEW_VG %s; varyoffvg NEW_VG", disk_name);
            rc=system(str);

            i=0;
            p_ctx[i]=&myctx[i];

            rc = ctx_init(p_ctx[i]);
            if ( rc == 0 )
            {
                debug("context creation succeeded, should have failed \n");
                ctx_close(p_ctx[i]);
                rc=1;
            }
            else rc=0;
            close(p_ctx[i]->fd);

            system("varyonvg NEW_VG; lspv");
            system("varyoffvg NEW_VG; exportvg NEW_VG");
            sprintf(str, "chdev -l %s -a pv=clear", disk_name);
            system(str);

            return rc;
            //7.1.187
            //While super-pipe IO on a LUN, keep configuring/unconfiguring other hdisk (root user)
        case 187:
            // creating many VLUN
            i=0;
            p_ctx[i]=&myctx[i];

            rc = ctx_init(p_ctx[i]);
            //rc = ctx_init2(p_ctx, flash_dev, flags, path_id);
            CHECK_RC(rc, "Context init failed");
            //thread to handle AFU interrupt & events
            pthread_create(&thread[i], NULL, ctx_rrq_rx, p_ctx[i]);
            nlba = chunk * (p_ctx[i]->chunk_size);
            rc = create_resource(p_ctx[i], nlba, flags, LUN_VIRTUAL);


            // now initiate rmdev and cfgdev for other // need to run it for longer time
            for ( i=0; i<10; i++ )
            {
                for ( j=0;j<10;j++)
                {
                    sprintf(str, "rmdev -l %s", disk_name1);
                    rc=system(str);

                    if ( rc != 0 )
                        return 1;
                    sleep(1);
                    sprintf(str, "cfgmgr -l %s", disk_name1);
                    rc=system(str);

                    if ( rc != 0 )
                        return 1;
                }
            }
            // Wait for IO thread to complete
            i=0;
            break;


            // 7.1.212
            //To call IOCTL DK_CAPI_PATH_QUERY  on a adapter with disks reserve_policy set to no_reserve and one disk reserve_policy set to single_path

        case 212:
                        // setting disk2 reserve policy to single_path
            i=0;

            sprintf(str, "chdev -l %s -a reserve_policy=no_reserve", disk_name);
            rc=system(str);

            if ( rc != 0 )
                rc=1;
            else
            {

                sprintf(str, "chdev -l %s -a reserve_policy=single_path", disk_name1);
                rc=system(str);

                if ( rc != 0 )
                    rc=2;
                else
                {
                    // call dk_capi_query_path
#ifdef _AIX
                    p_ctx[i]=&myctx[i];

                    rc = ctx_init(p_ctx[i]);
                    rc=ioctl_dk_capi_query_path_check_flag(p_ctx[0],0,0);
                    if ( rc == 1 )
                        return 1;
                    else
                        return 0;
#endif
                    // will handle for Linux later
                }
            }
            //7.1.213
        case 213:

            p_ctx[0]=&myctx[0];

            // disabling disk 1
            // changing reserve policy to no_reserve
            sprintf(str," chdev -l %s -a reserve_policy=no_reserve", disk_name);
            system(str);
            sprintf(str, "chpath -s disable -l %s -i 0", disk_name);
            rc=system(str);

            if ( rc != 0 )
                rc=1;
            else
            {
                sprintf(str, "lspath -l  %s | grep -i Disabled", disk_name);
                rc=system(str);

                if ( rc != 0 )
                    rc=2;
                else
                {
                    // call dk_capi_query_path
#ifdef _AIX
                    ctx_init(p_ctx[0]);
                    ioctl_dk_capi_detach(p_ctx[0]);

                    rc=ioctl_dk_capi_query_path_check_flag(p_ctx[0],DK_CPIF_DISABLED,0);
                    if ( rc == 1 )
                        rc=3;
                    else
                        rc=0;
#endif
                    // will handle for Linux later
                }
            }
            sprintf(str, "chpath -s enable -l %s -i 0", disk_name);
            system(str);

            return rc;
            //7.1.214
        case 214:
                        // cable pull adapter
            rc=system("cable_pull");
            if ( rc != 0 )
                rc=1;
            else
            {
                sprintf(str, "lspath %s | grep -i failed", disk_name);
                rc=system(str);

                if ( rc != 0 )
                    rc=2;
                else
                {
                    // call dk_capi_query_path
#ifdef _AIX
                    rc=ioctl_dk_capi_query_path_check_flag(p_ctx[0],DK_CPIF_CLOSED,DK_CPIF_CLOSED);
                    if ( rc == 1 )
                        rc=3;
                    else
                        rc=0;
#endif
                    // will handle for Linux later
                }
            }
            return rc;
        default:
            debug( "please enter correct flag \n");
            rc=1;
            break;
    }
    for (j=0;j<=i;j++ )
    {
        pthread_cancel(thread[j]);
        close_res(p_ctx[j]);
        ctx_close(p_ctx[j]);
    }
    return rc;


}

