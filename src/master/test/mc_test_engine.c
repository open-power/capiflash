/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/test/mc_test_engine.c $                            */
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
#include "mc_test.h"

int test1()
{
	return -1;
}

int mc_test_engine(mc_test_t test_name)
{
	int rc = 0;
	if(get_fvt_dev_env()) return -1;

	if(fork() == 0){
	switch(test_name)
	{
		case TEST_MC_REG:
			rc = mc_register_tst();
			break;

		case TEST_MC_UNREG:
			rc = mc_unregister_tst();
			break;

		case TEST_MC_OPEN:
			rc = mc_open_tst(1);
			break;

		case TEST_MC_OPEN_ERROR1:
			rc = mc_open_tst(2);
			break;

		case TEST_MC_OPEN_ERROR2:
			rc = mc_open_tst(3);
			break;

		case TEST_MC_OPEN_ERROR3:
			rc = mc_open_tst(4);
			break;

		case TEST_MC_CLOSE:
			rc = mc_close_tst(1);
			break;

		case TEST_MC_CLOSE_ERROR1:
			rc = mc_close_tst(2);
			break;

		case TEST_MC_SIZE:
			rc = mc_size_tst(1);
			break;

		case TEST_MC_SIZE_ERROR2:
			rc = mc_size_tst(2);
			break;

		case TEST_MC_SIZE_ERROR3:
			rc = mc_size_tst(3);
			break;

		case TEST_MC_SIZE_ERROR4:
			rc = mc_size_tst(4);
			break;

		case TEST_MC_GETSIZE:
			rc = mc_size_tst(0);
			break;

		case TEXT_MC_XLAT_IO:
			rc = mc_xlate_tst(1);
			break;

		case TEXT_MC_XLAT_IO_V2P:
			rc = mc_xlate_tst(2);
			break;

		case TEXT_MC_XLAT_IO_P2V:
			rc = mc_xlate_tst(3);
			break;

		case TEST_MC_HDUP_ORG_IO:
			rc = mc_hdup_tst(1);
			break;

		case TEST_MC_HDUP_DUP_IO:
			rc = mc_hdup_tst(2);
			break;

		case TEST_MC_HDUP_ERROR1:
			rc = mc_hdup_tst(3);
			break;

		case TEST_MC_DUP:
			rc = test_mc_dup_api();
			break;

		case TEST_MC_CLONE_RDWR:
			rc = test_mc_clone_api(MC_RDWR);
			break;

		case TEST_MC_CLONE_READ:
			rc = test_mc_clone_api(MC_RDONLY);
			break;

		case TEST_MC_CLONE_WRITE:
			rc = test_mc_clone_api(MC_WRONLY);
			break;

		case TEST_MC_CLN_O_RDWR_CLN_RD:
			rc = test_mc_clone_error(MC_RDWR,MC_RDONLY);
			break;

		case TEST_MC_CLN_O_RDWR_CLN_WR:
			rc = test_mc_clone_error(MC_RDWR,MC_WRONLY);
			break;

		case TEST_MC_CLN_O_RD_CLN_RD:
			rc = test_mc_clone_error(MC_RDONLY,MC_RDONLY);
			break;

		case TEST_MC_CLN_O_RD_CLN_WR:
			rc = test_mc_clone_error(MC_RDWR,MC_WRONLY);
			break;

		case TEST_MC_CLN_O_WR_CLN_RD:
			rc = test_mc_clone_error(MC_WRONLY,MC_RDONLY);
			break;

		case TEST_MC_CLN_O_WR_CLN_WR:
			rc = test_mc_clone_error(MC_WRONLY,MC_WRONLY);
			break;

		case TEST_MC_CLN_O_RD_CLN_RDWR:
			rc = test_mc_clone_error(MC_RDONLY, MC_RDWR);
			break;

		case TEST_MC_CLN_O_WR_CLN_RDWR:
			rc = test_mc_clone_error(MC_WRONLY, MC_RDWR);
			break;

		case TEST_MC_MAX_SIZE:
			rc = test_mc_max_size();
			break;

		case TEST_MC_MAX_CTX_N_RES:
			rc = test_max_ctx_n_res();
			break;

		case TEST_MC_MAX_RES_HNDL:
			rc = mc_max_vdisk_thread();
			break;

		case TEST_MC_MAX_CTX_HNDL:
			rc = mc_max_open_tst();
			break;

		case TEST_MC_MAX_CTX_HNDL2:
			rc = mc_open_close_tst();
			break;

		case TEST_MC_CHUNK_REGRESS:
			rc = mc_test_chunk_regress(1);
			break;

		case TEST_MC_SIZE_REGRESS:
			rc = mc_test_chunk_regress(4);
			break;

		case TEST_MC_CHUNK_REGRESS_BOTH_AFU:
			rc = mc_test_chunk_regress_both_afu();
			break;

		case TEST_MC_SIGNAL:
			rc = test1();
			break;

		case TEST_ONE_UNIT_SIZE:
			rc = test_one_aun_size();
			break;
		case TEST_MC_LUN_SIZE:
			rc = test_mc_lun_size(0);
			break;

		case TEST_MC_PLBA_OUT_BOUND:
			rc = test_mc_lun_size(1);
			break;

		case TEST_MC_ONE_CTX_TWO_THRD:
			rc = test_onectx_twothrd(1);
			break;

		case TEST_MC_ONE_CTX_RD_WRSIZE:
			rc = test_onectx_twothrd(2);
			break;

		case TEST_MC_TWO_CTX_RD_WRTHRD:
			rc = test_two_ctx_two_thrd(1);
			break;

		case TEST_MC_TWO_CTX_RDWR_SIZE:
			rc = test_two_ctx_two_thrd(2);
			break;

		case TEST_MC_TEST_VDISK_IO:
			rc = test_vdisk_io();
			break;

		case TEST_MC_LUN_DISCOVERY:
			rc = test_lun_discovery(1);
			break;

		case TEST_MC_AFU_RC_CAP_VIOLATION:
			rc = test_lun_discovery(2);
			break;

		case TEST_MC_RW_CLS_RSH:
			rc = test_rw_close_hndl(1);
			break;

		case TEST_MC_UNREG_MC_HNDL:
			rc = test_rw_close_hndl(2);
			break;

		case TEST_MC_RW_CLOSE_CTX:
			rc = test_rw_close_hndl(3);
			break;

		case TEST_MC_CLONE_MANY_RHT:
			rc = test_mc_clone_many_rht();
			break;

		case TEST_AUN_WR_PLBA_RD_VLBA:
			rc = test_mc_lun_size(2);
			break;

		case TEST_GOOD_CTX_ERR_CTX_CLS_RHT:
			rc = test_good_ctx_err_ctx(1);
			break;

		case TEST_GOOD_CTX_ERR_CTX_UNREG_MCH:
			rc = test_good_ctx_err_ctx(2);
			break;

		case TEST_GOOD_CTX_ERR_CTX:
			rc = test_good_ctx_err_ctx(3);
			break;

		case TEST_MC_IOARCB_EA_ALGNMNT_16:
			rc = test_mc_ioarcb_ea_alignment(1);
			break;

		case TEST_MC_IOARCB_EA_ALGNMNT_128:
			rc = test_mc_ioarcb_ea_alignment(2);
			break;

		case TEST_MC_IOARCB_EA_INVLD_ALGNMNT:
			rc = test_mc_ioarcb_ea_alignment(3);
			break;


		case TEST_MC_MNY_CTX_ONE_RRQ_C_NULL:
			rc = test_many_ctx_one_rrq_curr_null();
			break;

		case TEST_MC_ALL_AFU_DEVICES:
			rc = test_all_afu_devices();
			break;

		case MC_TEST_RWBUFF_GLOBAL:
			rc = mc_test_rwbuff_global();
			break;

		case MC_TEST_RW_SIZE_PARALLEL:
			rc = test_mc_rw_size_parallel();
			break;

		case MC_TEST_RWBUFF_SHM:
			rc = test_mc_rwbuff_shm();
			break;

		case MC_TEST_GOOD_ERR_AFU_DEV:
			rc = test_mc_good_error_afu_dev();
			break;

		case TEST_MC_REGRESS_RESOURCE:
			rc = mc_test_chunk_regress_long();
			break;

		case TEST_MC_REGRESS_CTX_CRT_DSTR:
			rc = test_mc_regress_ctx_crt_dstr(1);
			break;

		case TEST_MC_REGRESS_CTX_CRT_DSTR_IO:
			rc = test_mc_regress_ctx_crt_dstr(2);
			break;

		default:
			rc = -1;
			break;
	}
	exit(rc);
	}
	wait(&rc);
	if (WIFEXITED(rc)) {
		rc =  WEXITSTATUS(rc);
	}
	return rc;
}
