/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/test/fvt_master.C $                                */
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
/**
 *******************************************************************************
 * \file
 * \brief
 * \ingroup
 ******************************************************************************/
#include <gtest/gtest.h>

extern "C"
{
	#include "mc_test.h"
}

/*
 * NOTE:
 * IO request can be send by VLBA only
 * and read/write opcode only
 * so PLBA related TC became Error path
 * because failing with afu rc : 0x21
 * Lun discovery also error path now
 */

//mc_register tests.
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_reg_api)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_REG));
}


//mc_unregister tests.
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_unreg_api)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_UNREG));
}


//mc_open tests.
//positive test 1
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_open_api)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_OPEN));
}
//Negative test 2
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_open_rdonly_io)
{
    ASSERT_EQ(0x5,mc_test_engine(TEST_MC_OPEN_ERROR1));
}
//Negative test 3
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_open_wronly_io)
{
    ASSERT_EQ(0x5,mc_test_engine(TEST_MC_OPEN_ERROR2));
}
//Negative test 4
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_open_null_io)
{
    ASSERT_EQ(0x5,mc_test_engine(TEST_MC_OPEN_ERROR3));
}


//mc_close tests
//Positive test 1
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_close_api)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_CLOSE));
}
//Negative test 2
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_close_io)
{
    ASSERT_EQ(0x5,mc_test_engine(TEST_MC_CLOSE_ERROR1));
}


//mc_size tests.
//Positive test 1
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_size_api)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_SIZE));
}
//Negative test 2
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_size_error_null_vdisk)
{
    ASSERT_EQ(0x13, mc_test_engine(TEST_MC_SIZE_ERROR2));
}
//Negative test 3
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_size_error_out_range)
{
    ASSERT_EQ(0x13, mc_test_engine(TEST_MC_SIZE_ERROR3));
}
//Negative test 4
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_size_error_zero_vdisk)
{
    ASSERT_EQ(4,mc_test_engine(TEST_MC_SIZE_ERROR4));
}


//mc_get_size tests
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_getsize_api)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_GETSIZE));
}


//mc_xlate_lba tests
//Positive test 1
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_xlt_api_io)
{
    ASSERT_EQ(0,mc_test_engine(TEXT_MC_XLAT_IO));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_xlt_api_io_v2p)
{
    ASSERT_EQ(0x21, mc_test_engine(TEXT_MC_XLAT_IO_V2P));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_xlt_api_io_p2v)
{
    ASSERT_EQ(0x21, mc_test_engine(TEXT_MC_XLAT_IO_P2V));
}


//mc_hdup tests
//Positive test 1
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_hdup_api_org_io)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_HDUP_ORG_IO));
}
//Positive test 2
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_hdup_api_dup_io)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_HDUP_DUP_IO));
}
//Negative test 3
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_hdup_close_io)
{
    ASSERT_EQ(3,mc_test_engine(TEST_MC_HDUP_ERROR1));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_clone_rdwr)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_CLONE_RDWR));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_clone_rd)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_CLONE_READ));
}
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_clone_wr)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_CLONE_WRITE));
}
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_max_rh)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_MAX_RES_HNDL));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_aun_size)
{
    ASSERT_EQ(0,mc_test_engine(TEST_ONE_UNIT_SIZE));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_lun_size)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_LUN_SIZE));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_max_ctx_hndl)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_MAX_CTX_HNDL));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_max_ctx_n_res)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_MAX_CTX_N_RES));
}
TEST(DISABLED_Master_FVT_Suite, G_Test_max_ctx_regress)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_MAX_CTX_HNDL2));
}
TEST(DISABLED_Master_FVT_Suite, G_Test_allocate_complete_lun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_MAX_SIZE));
}
TEST(DISABLED_Master_FVT_Suite, G_Test_allocate_lun_max_ctx_prc_rch_thrd)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_CHUNK_REGRESS));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_max_ctx_max_res_both_afu)
{
	ASSERT_EQ(0,mc_test_engine(TEST_MC_CHUNK_REGRESS_BOTH_AFU));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_xlate_wo_mc_size)
{
    ASSERT_EQ(1,test_mc_xlate_error(1));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_xlate_invalid_rch)
{
    ASSERT_EQ(3,test_mc_xlate_error(3));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_xlate_invalid_vlba)
{
    ASSERT_EQ(4,test_mc_xlate_error(4));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_xlate_mch_rch_diff)
{
    ASSERT_EQ(6,test_mc_xlate_error(6));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_xlate_invalid_mch)
{
    ASSERT_EQ(7,test_mc_xlate_error(7));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_xlate_after_mc_close)
{
    ASSERT_EQ(8,test_mc_xlate_error(8));
}
TEST(DISABLED_Master_FVT_Suite, G_Test_vdisk_io)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_TEST_VDISK_IO));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_lun_discovery)
{
    ASSERT_EQ(0x21, mc_test_engine(TEST_MC_LUN_DISCOVERY));
}
TEST(DISABLED_Master_FVT_Suite, G_Test_onectx_twothrd)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_ONE_CTX_TWO_THRD));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_cln_opn_rdwr_cln_rd)
{
	ASSERT_EQ(0x5,mc_test_engine(TEST_MC_CLN_O_RDWR_CLN_RD));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_cln_opn_rdwr_cln_wr)
{
	ASSERT_EQ(0x5,mc_test_engine(TEST_MC_CLN_O_RDWR_CLN_WR));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_cln_opn_rd_cln_rd)
{
	ASSERT_EQ(0x5,mc_test_engine(TEST_MC_CLN_O_RD_CLN_RD));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_cln_opn_rd_cln_wr)
{
	ASSERT_EQ(0x5,mc_test_engine(TEST_MC_CLN_O_RD_CLN_WR));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_cln_opn_wr_cln_rd)
{
	ASSERT_EQ(0x5,mc_test_engine(TEST_MC_CLN_O_WR_CLN_RD));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_cln_opn_wr_cln_wr)
{
	ASSERT_EQ(0x5,mc_test_engine(TEST_MC_CLN_O_WR_CLN_WR));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_cln_opn_rd_cln_rdwr)
{
	ASSERT_EQ(0x5,mc_test_engine(TEST_MC_CLN_O_RD_CLN_RDWR));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_cln_opn_wr_cln_rdwr)
{
	ASSERT_EQ(0x5,mc_test_engine(TEST_MC_CLN_O_WR_CLN_RDWR));
}


TEST(DISABLED_Master_FVT_Suite, E_Test_mc_reg_afu_dev)
{
	ASSERT_EQ(3, test_mc_reg_error(3));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_reg_invalid_dev)
{
	ASSERT_EQ(4,test_mc_reg_error(4));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_reg_invalid_ctx)
{
	ASSERT_EQ(5,test_mc_reg_error(5));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_reg_twice)
{
	ASSERT_EQ(7,test_mc_reg_error(7));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_reg_twice_diff_prc)
{
	ASSERT_EQ(8,test_mc_reg_error(8));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_two_ctx_two_thrd_r_wrsize)
{
	ASSERT_EQ(0,mc_test_engine(TEST_MC_TWO_CTX_RD_WRTHRD));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_two_ctx_two_thrd_rw_plysize)
{
	ASSERT_EQ(0, mc_test_engine(TEST_MC_TWO_CTX_RDWR_SIZE));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_one_thd_rw_one_size)
{
    ASSERT_EQ(0, mc_test_engine(TEST_MC_ONE_CTX_RD_WRSIZE));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_plba_out_of_bound)
{
	ASSERT_EQ(0x21, mc_test_engine(TEST_MC_PLBA_OUT_BOUND));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_rw_close_res_hndl)
{
	int res = mc_test_engine(TEST_MC_RW_CLS_RSH);
	if(res == 0x13 || res == 0x05)
		res =1;
	ASSERT_EQ(1, res);
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_rw_unreg_mc_hndl)
{
	int res = mc_test_engine(TEST_MC_UNREG_MC_HNDL);
	if(res == 0x13 || res == 0x21 || res == 0x3 || res == 0x5 || res == 0x4)
		res =1;
	ASSERT_EQ(1, res);
}
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_clone_many_rht)
{
	ASSERT_EQ(0,mc_test_engine(TEST_MC_CLONE_MANY_RHT));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_aun_wr_plba_rd_vlba)
{
	ASSERT_EQ(0x21, mc_test_engine(TEST_AUN_WR_PLBA_RD_VLBA));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_good_ctx_err_ctx)
{
	ASSERT_EQ(0,mc_test_engine(TEST_GOOD_CTX_ERR_CTX));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_good_ctx_err_ctx_unrg_mch)
{
	ASSERT_EQ(0,mc_test_engine(TEST_GOOD_CTX_ERR_CTX_UNREG_MCH));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_good_ctx_err_ctx_cls_rht)
{
	ASSERT_EQ(0,mc_test_engine(TEST_GOOD_CTX_ERR_CTX_CLS_RHT));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_afu_rc_invalid_opcode)
{
	ASSERT_EQ(0x21, test_mc_invalid_ioarcb(1));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_ioarcb_ea_null)
{
	//ASSERT_EQ(0x31, test_mc_invalid_ioarcb(2));
	int res = test_mc_invalid_ioarcb(2);
	if(res == 0x0 || res == 0x31)
		res =1;
	ASSERT_EQ(1, res);
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_ioarcb_invalid_flags)
{
	ASSERT_EQ(0x58, test_mc_invalid_ioarcb(3));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_ioarcb_invalid_lun_fc_port)
{
	ASSERT_EQ(0x21, test_mc_invalid_ioarcb(4));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_afu_rc_rht_invalid)
{
	ASSERT_EQ(0x5, test_mc_invalid_ioarcb(5));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_afu_rc_rht_out_of_bounds)
{
	ASSERT_EQ(0x3, test_mc_invalid_ioarcb(6));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_error_page_fault)
{
	//ASSERT_EQ(0x31, test_mc_invalid_ioarcb(7));
	ASSERT_EQ(0x0, test_mc_invalid_ioarcb(7));

}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_ioarcb_invalid_ctx_id)
{
	ASSERT_EQ(0x21, test_mc_invalid_ioarcb(8));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_rc_flags_underrun)
{
	ASSERT_EQ(0x2, test_mc_invalid_ioarcb(9));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_scsi_rc_check)
{
	ASSERT_EQ(0x2, test_mc_invalid_ioarcb(11));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_ioarcb_d_len_0)
{
	ASSERT_EQ(0x2, test_mc_invalid_ioarcb(12));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_ioarcb_blk_len_0)
{
	ASSERT_EQ(0x55, test_mc_invalid_ioarcb(13));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_ioarcb_intr_prcs_ctx)
{
	ASSERT_EQ(0x21, test_mc_inter_prcs_ctx(1));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_ioarcb_intr_prcs_ctx_rsh_clsd)
{
	ASSERT_EQ(0x21, test_mc_inter_prcs_ctx(2));
}
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_ioarcb_ea_alignmnt_16)
{
	ASSERT_EQ(0,mc_test_engine(TEST_MC_IOARCB_EA_ALGNMNT_16));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_ioarcb_ea_alignmnt_128)
{
	ASSERT_EQ(0,mc_test_engine(TEST_MC_IOARCB_EA_ALGNMNT_128));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_ioarcb_ea_invalid_alignmnt)
{
	ASSERT_EQ(3,mc_test_engine(TEST_MC_IOARCB_EA_INVLD_ALGNMNT));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_all_afu_devices)
{
	ASSERT_EQ(0,mc_test_engine(TEST_MC_ALL_AFU_DEVICES));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_rwbuff_in_global)
{
	ASSERT_EQ(0,mc_test_engine(MC_TEST_RWBUFF_GLOBAL));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_rwbuff_in_shared_memory)
{
	ASSERT_EQ(0,mc_test_engine(MC_TEST_RWBUFF_SHM));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_rw_size_parallel_sync)
{
	ASSERT_EQ(0,mc_test_engine(MC_TEST_RW_SIZE_PARALLEL));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_afu_rc_cap_violation)
{
	ASSERT_EQ(0x21,mc_test_engine(TEST_MC_AFU_RC_CAP_VIOLATION));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_good_path_afu_err_path_afu)
{
	ASSERT_EQ(0,mc_test_engine(MC_TEST_GOOD_ERR_AFU_DEV));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_size_invalid_mch)
{
	ASSERT_EQ(1,test_mc_size_error(1));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_size_invalid_rsh)
{
	ASSERT_EQ(2,test_mc_size_error(2));
}

TEST(DISABLED_Master_FVT_Suite, E_Test_mc_size_after_mc_close)
{
	ASSERT_EQ(4,test_mc_size_error(4));
}
TEST(DISABLED_Master_FVT_Suite, E_Test_mc_size_after_mc_unreg)
{
	ASSERT_EQ(5,test_mc_size_error(5));
}
TEST(DISABLED_Master_FVT_Suite, G_Test_mc_regress_resource)
{
	ASSERT_EQ(0,mc_test_engine(TEST_MC_REGRESS_RESOURCE));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_regress_ctx_create_destroy)
{
	ASSERT_EQ(0,mc_test_engine(TEST_MC_REGRESS_CTX_CRT_DSTR));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_regress_ctx_create_destroy_io)
{
	ASSERT_EQ(0,mc_test_engine(TEST_MC_REGRESS_CTX_CRT_DSTR_IO));
}

TEST(DISABLED_Master_FVT_Suite, G_Test_mc_size_regress)
{
    ASSERT_EQ(0, mc_test_engine(TEST_MC_SIZE_REGRESS));
}
