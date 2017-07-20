/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/188.c $                                       */
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

int do_microcode_update( )
{
    int rc;

#ifdef _AIX
    rc=system("diag -d capi0 -T \"download -fl latest -s /etc/microcode\" -c");
#else
    sleep(5);
    system("sync;sync;sync;sync");

    rc=system("echo 10000000  > /sys/kernel/debug/powerpc/eeh_max_freezes");
    CHECK_RC(rc, "Failed to make max_freezes");

    sleep(5);

    rc=system("lsmod | grep cxlflash && rmmod -v cxlflash");
    CHECK_RC(rc, "Failed to unload cxlflash driver");

    rc=system("/usr/bin/flash_all_adapters");
    CHECK_RC(rc, "Failed in afu update");

    rc=system("/usr/bin/reload_all_adapters");
    CHECK_RC(rc, "Failed to reload updated afu image");

    rc=system("modprobe -v cxlflash");
    CHECK_RC(rc, "Failed to load cxlflash driver");

    // enable kernel traces
    system("echo \"module cxlflash +p\" > /sys/kernel/debug/dynamic_debug/control");

    set_spio_mode();
#endif

    return rc;
}

#ifndef _AIX

int do_perst()
{
    int rc;
    int iTer   =0;
    FILE *fileP = NULL;

    char tmpBuff[MAXBUFF];
    char blockCheckP[MAXBUFF];

    sleep(5);
    /* 
         PERST test case is under maunal Cflash FVT test case.  
         Test case will by default PERST all the adapter present in the system 
         Incase user wants to perst in selective devices; then target device entry should in 
         /tmp/perstinfo_file - like 000N:01:00.0; so that cflash_perst PESRT tool can use it further 
         usage ex: cflash_perst -t 000N:01:00.0
     */

    rc=system("echo 10000000  > /sys/kernel/debug/powerpc/eeh_max_freezes");
    CHECK_RC(rc, "Failed to make max_freezes");

    sleep(5);

    rc=system("lsmod | grep cxlflash && rmmod -v cxlflash");
    CHECK_RC(rc, "Failed to unload cxlflash driver");

    // reload_all_adapters will perform the PESRT

    fileP = fopen("/tmp/perstinfo_file", "r");
    if (NULL == fileP)
    {
        /* if perstinfo_file does not present;
           User will do PERST in all adapter
        */
        rc=system("/usr/bin/reload_all_adapters");
        CHECK_RC(rc, "Failed to reload updated afu image");
        
    }
    else
    {
      
      while (fgets(tmpBuff,MAXBUFF, fileP) != NULL)
      {
         iTer = 0;
        
        while (iTer < MAXBUFF)
        {
            if (tmpBuff[iTer] =='\n')
            {
                tmpBuff[iTer]='\0';
                break;
            }
            iTer++;
        }
        
          sprintf(blockCheckP," /usr/bin/cflash_perst -t %s ", tmpBuff );
          printf("......... command : %s \n",blockCheckP);
          rc = system(blockCheckP);
          if ( rc )
          {
                   printf(".................... do_perst() - PERST-ing failed for %s ........ \n", tmpBuff );
                   return rc;
          }
       }  // End of While (fgets())
    }   // End of else 
    rc=system("modprobe -v cxlflash");
    CHECK_RC(rc, "Failed to load cxlflash driver");

    // enable kernel traces
    system("echo \"module cxlflash +p\" > /sys/kernel/debug/dynamic_debug/control");

    set_spio_mode();

    return rc;
}


#endif


int ioctl_7_1_188( int flag )
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t threadId;
    __u64 chunk;
    __u64 nlba;
    __u64 stride;

    // pid used to create unique data patterns & logging from util !
    pid = getpid();

    // just for sake of cleanup !
    set_spio_mode();

#ifndef _AIX
    char WWID[MAXBUFF];

    rc = diskToWWID ( WWID );
    CHECK_RC(rc, "diskToWWID failed");
#endif

    //ctx_init with default flash disk & devno
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    rc = pthread_create(&threadId, NULL, ctx_rrq_rx, p_ctx);
    CHECK_RC(rc, "pthread_create failed");

    switch ( flag )
    {
        case 188:
#ifndef _AIX
            chunk = (p_ctx->last_phys_lba+1)/p_ctx->chunk_size;
            nlba = chunk * p_ctx->chunk_size;
            //create vlun
            rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");

            stride=p_ctx->block_size;
            // Check that IO works
            rc = do_io(p_ctx, stride);
            CHECK_RC(rc, "do_io() failed");

            // flash disk must be released before afu update
            pthread_cancel(threadId);
            rc=close_res(p_ctx);
            CHECK_RC(rc, "close_res() failed");
            rc=ctx_close(p_ctx);
            CHECK_RC(rc, "ctx_close() failed");

            // Perform microcode update
            rc = do_microcode_update();
            CHECK_RC(rc, "do_microcode_update failed");

            //ctx_init with default flash disk & devno
            rc = ctx_init(p_ctx);
            CHECK_RC(rc, "Context init failed");

            //thread to handle AFU interrupt & events
            rc = pthread_create(&threadId, NULL, ctx_rrq_rx, p_ctx);
            CHECK_RC(rc, "pthread_create failed");

            //create vlun
            rc = create_resource(p_ctx, nlba, DK_UVF_ALL_PATHS, LUN_VIRTUAL);
            CHECK_RC(rc, "create LUN_VIRTUAL failed");

            // Check if IO is back to normal after afu update
            rc = do_io(p_ctx, stride);
            CHECK_RC(rc, "do_io() failed");
#else
            rc=create_multiple_vluns(p_ctx);
            if ( rc != 0 )
                return 2;
            rc=do_microcode_update();
#endif
            break;

        case 189:
#ifndef _AIX
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");

            stride=0x16;
            // Just perform io write now.
            rc = do_write_or_read(p_ctx, stride, WRITE);
            CHECK_RC(rc, "io write failed");

            pthread_cancel(threadId);
            rc=close_res(p_ctx);
            CHECK_RC(rc, "resource cleanup failed");
            rc=ctx_close(p_ctx);
            CHECK_RC(rc, "context cleanup failed");

            rc = do_microcode_update();
            CHECK_RC(rc, "do_microcode_update failed");

#ifndef _AIX
            rc = WWIDtoDisk ( WWID);
            CHECK_RC(rc, "WWIDToDisk failed");
#endif
            //ctx_init with default flash disk & devno
            rc = ctx_init(p_ctx);
            CHECK_RC(rc, "Context init failed");

            // Create a pLUN over same disk with new context again
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");

            //thread to handle AFU interrupt & events
            rc = pthread_create(&threadId, NULL, ctx_rrq_rx, p_ctx);
            CHECK_RC(rc, "pthread_create failed");

            // Now perform io read & data compare test.
            rc = do_write_or_read(p_ctx, stride, READ);
            CHECK_RC(rc, "io read failed");
#else
            rc=create_direct_lun(p_ctx);
            if ( rc != 0 )
                return 2;
            rc=do_microcode_update();
#endif
            break;

#ifndef _AIX
        case E_CAPI_LINK_DOWN:
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");

            /* be default IO will happen; until we export NO_IO */
            char * noIOP   = getenv("NO_IO");

            if ( noIOP == NULL )
            {
                stride=0x16;
                // Just perform io write now.
                rc = do_write_or_read(p_ctx, stride, WRITE);
                CHECK_RC(rc, "io write failed");
            }

            pthread_cancel(threadId);
            rc=close_res(p_ctx);
            CHECK_RC(rc, "resource cleanup failed");
            rc=ctx_close(p_ctx);
            CHECK_RC(rc, "context cleanup failed");

            rc = do_perst();
            CHECK_RC(rc, "do_perst() failed");

            //ctx_init with default flash disk & devno
            rc = ctx_init(p_ctx);
            CHECK_RC(rc, "Context init failed");

            // Create a pLUN over same disk with new context again
            rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
            CHECK_RC(rc, "create LUN_DIRECT failed");

            //thread to handle AFU interrupt & events
            rc = pthread_create(&threadId, NULL, ctx_rrq_rx, p_ctx);
            CHECK_RC(rc, "pthread_create failed");
            if ( noIOP == NULL )
            {
                // Now perform io read & data compare test.
                rc = do_write_or_read(p_ctx, stride, READ);
                CHECK_RC(rc, "io read failed");
            }

            break;

#endif
        default:

            printf("Flag not correct \n");
            CHECK_RC(1,"Usage failed");
            break;

    }

    pthread_cancel(threadId);
    rc=close_res(p_ctx);
    CHECK_RC(rc, "close_res() failed");
    rc=ctx_close(p_ctx);
    CHECK_RC(rc, "ctx_close() failed");

    return rc;
}
