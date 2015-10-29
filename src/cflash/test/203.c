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
extern char cflash_path[MC_PATHLEN];
extern pid_t pid;
extern int MAX_RES_HANDLE;
#ifdef _AIX
int MAX_OPEN_1=494;
int MAX_NCHAN_VALUE=8;
#else
int MAX_OPEN_1=200;
#endif

static int res_num=1,ctx_num=1;
/*int traditional_io(int disk_num)
   {
       int rc,i;

       char *disk_name, *str;
       if ( disk_num == 1 )
           disk_name = (char *)getenv("FLASH_DISK_1");
       else
           disk_name = (char *)getenv("FLASH_DISK_2");

       sprintf(str, "dd if=/dev/hdisk0 of=/dev/%s >/tmp/read_write.log &", disk_name);
       system(str);
       for (i=0;i<360;i++)
       {
           sleep(1);
           rc=system("cat /tmp/read_write.log  | grep -i failed ");
           if ( rc == 0 )
               return 1;
       }
   }
 */
int change_nchan( int nchan )
{
    int rc;
    char *str;
    char *flash_name = (char *)getenv("FLASH_ADAPTER");
    str = (char *) malloc(100);
    // removing all disk associalted with flash
    sprintf(str, "rmdev -l %s -R", flash_name);
    rc=system(str);
    if ( rc != 0 )
    {
        fprintf(stderr,"removing all child disk failed\n");
        return rc;
    }

    sprintf(str, "chdev -l %s -a num_channel=%d", flash_name,nchan);
    debug("%s\n",str);
    rc=system(str);
    if ( rc != 0 )
    {
        fprintf(stderr,"changing nchn value failed");
        return rc;
    }
    sprintf(str, "cfgmgr -l %s", flash_name);
    rc=system(str);
    if ( rc != 0 )
        exit(rc);
    return rc;
}

int max_ctx_res()
{
    int rc;
    __u64 my_res_hndl[MAX_RES_HANDLE];
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    int i;
    pthread_t thread;

    pid = getpid();

    MAX_RES_HANDLE=get_max_res_hndl_by_capacity(cflash_path);

    debug("**********************creatin CTX NO %d******************************\n",ctx_num++);
    sleep(1);
    rc = ctx_init(p_ctx);
    CHECK_RC(rc,"ctx_init failed\n");

    pthread_create(&thread, NULL, ctx_rrq_rx, p_ctx);
    debug("pid=%d ctx:%d is attached..sahitya\n",pid,p_ctx->ctx_hndl);
    //open max allowed res for a context
    for (i=0;i<MAX_RES_HANDLE;i++)
    {
        rc = create_resource(p_ctx,0,DK_UVF_ASSIGN_PATH,LUN_VIRTUAL);
        if (rc != 0)
        {
            fprintf(stderr, "ctx: %d:create_resource: failed,rc %d\n", p_ctx->ctx_hndl,rc);
            g_error = -1;
            pthread_cancel(thread);
            return errno;
        }
        my_res_hndl[i]=p_ctx->rsrc_handle;
        debug("ctx:%d res hndl:%lu\n", p_ctx->ctx_hndl,my_res_hndl[i]);
        debug("********res_num=%d\n", res_num++);
    }

    for (i=0;i<MAX_RES_HANDLE;i++)
    {
        p_ctx->rsrc_handle=my_res_hndl[i];
        debug("ctx:%d res hndl:%lu\n",p_ctx->ctx_hndl,my_res_hndl[i]);
        rc = vlun_resize(p_ctx,p_ctx->chunk_size);
        if (rc != 0)
        {
            fprintf(stderr, "%d:resize: failed,rc %d\n", pid,rc);
            g_error = -1;
            pthread_cancel(thread);
            return errno;
        }
        //NO IO here
        //rc=do_io(p_ctx,0x100);
        //CHECK_RC(rc, "do_io failed\n");
    }

    for (i=0;i<MAX_RES_HANDLE;i++)
    {
        p_ctx->rsrc_handle=my_res_hndl[i];
        debug("ctx:%d res hndl:%lu\n",p_ctx->ctx_hndl,my_res_hndl[i]);
        rc = close_res(p_ctx);
        if (rc != 0)
        {
            fprintf(stderr, "ctx: %d:mc_close: failed,rc %d\n", p_ctx->ctx_hndl,rc);
            g_error = -1;
            pthread_cancel(thread);
            return errno;
        }
    }

    sleep(3);
    pthread_cancel(thread);
    rc = ctx_close(p_ctx);
    if (rc != 0)
    {
        fprintf(stderr, "mc_unregister failed for mc_hdl %x\n", p_ctx->ctx_hndl);
        g_error = -1;
        return errno;
    }
    debug("mc unregistered for ctx:%d\n",p_ctx->ctx_hndl);

    return 0;
}


int max_ctx_n_res_nchan(int nchan)
{
    int rc;
    int i;
    int MAX_CTX;

    //Creating threads for ctx_init with nchan value
    // calculate how many ctx can be created

    MAX_CTX=(MAX_OPEN_1-(8 - nchan));
    debug("*****MAX_CTX=%d\n", MAX_CTX);
    sleep(3);
    for (i = 0; i < MAX_CTX; i++)
    {
        if ( 0 == fork() )
        {
            rc = max_ctx_res();
            exit(rc);
        }
    }
    rc=wait4all();
    return rc;
}

int ioctl_7_1_193_1( int flag   )
{

    int rc;
#ifdef _AIX
    int  i;
#endif

    // Create MAX context & resource handlers based on default Nchn value
    // first change the nchan value
    if ( flag != 1 )
    {
        rc=change_nchan(1);
        CHECK_RC(rc, "changing nchan value failed");
    }
    // now create max context and max resource and after than detach
    rc=max_ctx_n_res_nchan(1);
    CHECK_RC(rc, "creating max context and resource handler failed");
    // change the nchan value again
#ifdef _AIX
    if ( flag != 1 )
    {
        for (i=2;i<MAX_NCHAN_VALUE;i++)
        {
            rc=change_nchan(i);
            CHECK_RC(rc, "changing nchan value failed");
            // now again create max context and max resource and after than detach
            rc=max_ctx_n_res_nchan(i);
            CHECK_RC(rc, "creating max context and resource handler failed");
            i=i+1;
        }
    }
#endif
    return rc;

}
#ifdef _AIX
// this is 5th procedure in 7.1.193. This is written in different case, as dont make sense with super pipe I/O
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

    /*    for ( i=0; i < 2 ; i++ )
        {
            if (0 == fork()) //child process
            {
                rc = traditional_io(i);
            }
            exit(rc);
        }
     */   return 0;
}
#endif





int max_ctx_res_190(struct ctx *p_ctx)
{
    int rc;
    __u64 my_res_hndl[MAX_RES_HANDLE];
    // struct ctx *p_ctx = (struct ctx *)arg;
    int i;
    __u64 actual_size;
    __u64 nlba, st_lba;
    __u64 stride;
    mc_stat_t l_mc_stat;

    pid = getpid();

    debug("**********************creatin CTX NO %d******************************\n",ctx_num++);
    sleep(1);
    rc = ctx_init(p_ctx);
    if (rc != 0)
    {
        fprintf(stderr, "Context init failed, errno %d\n", errno);
        g_error =-1;
        return errno;
    }
    debug("pid=%d ctx:%d is attached..sahitya\n",pid,p_ctx->ctx_hndl);
    //open max allowed res for a context
    for (i=0;i<MAX_RES_HANDLE;i++)
    {
        rc = create_resource(p_ctx,0,DK_UVF_ASSIGN_PATH,LUN_VIRTUAL);
        if (rc != 0)
        {
            fprintf(stderr, "ctx: %d:create_resource: failed,rc %d\n", p_ctx->ctx_hndl,rc);
            g_error = -1;
            return errno;
        }
        my_res_hndl[i]=p_ctx->rsrc_handle;
        debug("ctx:%d res hndl:%lu\n", p_ctx->ctx_hndl,my_res_hndl[i]);
        debug("********res_num=%d\n", res_num++);
    }


    nlba = l_mc_stat.size * (1 << l_mc_stat.nmask);
    stride = 1 << l_mc_stat.nmask;
    nlba = 0; //NO IO here
    for (st_lba = 0;st_lba < nlba; st_lba += (NUM_CMDS*stride))
    {
        rc = send_write(p_ctx, st_lba, stride, pid);
        if ((rc != 0) && (actual_size == 0))
        {
            printf("%d : Fine, IO @(0X%lX) but range is(0X%lX)\n",pid, st_lba, nlba-1);
            break;
        }
        else
        {
            fprintf(stderr,"%d : Send write failed @ (0X%lX) LBA\n", pid, st_lba);
            g_error = -1;
            return errno;
        }
        rc = send_read(p_ctx, st_lba, stride);
        rc += rw_cmp_buf(p_ctx, st_lba);
        if (rc)
        {
            g_error = -1;
            return errno;
        }
    }

    debug("ctx:%d res_hand:%lu size:%lu\n",p_ctx->ctx_hndl,my_res_hndl[i],p_ctx->lun_size);
    //size += 16;


    /*    for (i=0;i<MAX_RES_HANDLE;i++)
        {
                    p_ctx->rsrc_handle=my_res_hndl[i];
                    debug("ctx:%d res hndl:%u\n",p_ctx->ctx_hndl,my_res_hndl[i]);
            rc = close_res(p_ctx);
            if (rc != 0)
            {
                fprintf(stderr, "ctx: %d:mc_close: failed,rc %d\n", p_ctx->ctx_hndl,rc);
                g_error = -1;
                return errno;
            }
        }
     */
    sleep(3);
    /*   rc = ctx_close(p_ctx);
       if (rc != 0)
       {
           fprintf(stderr, "mc_unregister failed for mc_hdl %p\n", p_ctx->ctx_hndl);
           g_error = -1;
           return errno;
       }
       debug("mc unregistered for ctx:%d\n",p_ctx->ctx_hndl);
     */
    return 0;
}



int max_ctx_n_res_190()
{
    int rc;
    int i;
    int MAX_CTX;

    MAX_CTX=MAX_OPEN_1;
    debug("*****MAX_CTX=%d\n", MAX_CTX);
    sleep(3);
    for (i = 0; i < MAX_CTX; i++)
    {
        if ( 0 == fork() )
        {
            rc = max_ctx_res();
            exit(rc);
        }
    }
    rc=wait4all();
    return rc;
}


int ioctl_7_1_190(  )
{

    // Create MAX context & resource handlers based on default Nchn value
    // first change the nchan value
    // rc=change_nchan(atoi(getenv("NCHAN_VALUE")));
    // CHECK_RC(rc, "changing nchan value failed");

    // now create max context and max resource and after than detach

    // change the nchan value again
    debug("E_test_SPIO_RLS_DET\n");
    return max_ctx_n_res_190();
}


int ioctl_7_1_203()
{

    int rc=0;
    rc = setenv("NCHAN_VALUE", "0", true);
    CHECK_RC(rc, "NCHAN_VALUE env value setting failed \n");
    MAX_RES_HANDLE=get_max_res_hndl_by_capacity(cflash_path);
    if (MAX_RES_HANDLE <= 0)
    {
        fprintf(stderr,"Unable to run ioctl_7_1_203.. refere prior error..\n");
        return -1;
    }
    rc=ioctl_7_1_193_1(1);
    CHECK_RC(rc,"ioctl_7_1_193_1 1st call failed..\n");
    rc=ioctl_7_1_190();
    CHECK_RC(rc,"ioctl_7_1_190 1st call failed..\n");
    rc=ioctl_7_1_190();
    CHECK_RC(rc,"ioctl_7_1_190 2nd call failed..\n");
    rc=ioctl_7_1_190();
    CHECK_RC(rc,"ioctl_7_1_190 3rd call failed..\n");
    rc=ioctl_7_1_193_1(1);
    CHECK_RC(rc,"ioctl_7_1_193_1 last called failed\n");
    return 0;
}
