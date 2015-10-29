/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/manual.c $                                    */
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

// M_TEST_7_5_13_1 & M_TEST_7_5_13_2 cases
// i.e. M_7_5_13_1 & M_7_5_13_2 TC names
int ioctl_7_5_13(int ops)
{
    int rc;
    struct ctx myctx;
    struct ctx *p_ctx = &myctx;
    pthread_t threadId;
    __u64 stride;

    // pid used to create unique data patterns & logging from util !
    pid = getpid();

    //ctx_init with default flash disk & devno
    rc = ctx_init(p_ctx);
    CHECK_RC(rc, "Context init failed");

    //thread to handle AFU interrupt & events
    rc = pthread_create(&threadId, NULL, ctx_rrq_rx, p_ctx);
    CHECK_RC(rc, "pthread_create failed");

    rc = create_resource(p_ctx, 0, DK_UDF_ASSIGN_PATH, LUN_DIRECT);
    CHECK_RC(rc, "create LUN_DIRECT failed");

    stride=0x16;

    // To be used before reboot, for writing data
    if (ops == 1)
    {
        // Just perform io write now.
        rc = do_write_or_read(p_ctx, stride, 1);
        CHECK_RC(rc, "io write failed");

        printf("Now, reboot the system..\n");
        sleep(1000);
    }
    // To be used after reboot, for verification of data
    else
    {
        // Now perform io read & data compare test.
        rc = do_write_or_read(p_ctx, stride, 2);
        CHECK_RC(rc, "io read failed");
    }

    // Marking fail always for manual veirification.
    return -1;
}

