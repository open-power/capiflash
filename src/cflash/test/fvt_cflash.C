/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/fvt_cflash.C $                                */
/*                                                                        */
/* IBM Data Engine for NoSQL - Power Systems Edition User Library Project */
/*                                                                        */
/* Contributors Listed Below - COPYRIGHT 2015                             */
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
#include "cflash_test.h"
}

TEST(Cflash_FVT_Suite, E_MC_test_Ioctl_Invalid_Versions)
{
#ifdef _AIX
    ASSERT_EQ(22,mc_test_engine(TEST_IOCTL_INVALID_VERSIONS));
#else
     ASSERT_NE(0,mc_test_engine(TEST_IOCTL_INVALID_VERSIONS));
#endif
}

#ifdef _AIX
/*** DK_CAPI_QUERY_PATH  ****/
TEST(Cflash_FVT_Suite, G_MC_test_DCQP_Path_Count_Valid)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCQP_VALID_PATH_COUNT));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCQP_Path_Count_Invalid)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCQP_INVALID_PATH_COUNT));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCQP_Dual_Path_Count)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCQP_DUAL_PATH_COUNT));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCQP_Dk_Cpif_Reserved)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCQP_DK_CPIF_RESERVED));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCQP_Dk_Cpif_Failed)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCQP_DK_CPIF_FAILED));
}
/*** DK_CAPI_ATTACH  ****/

TEST(Cflash_FVT_Suite, G_MC_test_DCA_Other_Devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_OTHER_DEVNO));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCA_Invalid_Devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_INVALID_DEVNO));
}
#endif

TEST(Cflash_FVT_Suite, E_MC_test_DCA_Invalid_Intrpt_Num)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_INVALID_INTRPT_NUM));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCA_Valid_Values)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_VALID_VALUES));
}
TEST(Cflash_FVT_Suite, E_MC_test_DCA_Invalid_Flags)
{
#ifdef _AIX
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_INVALID_FLAGS));
#else
    ASSERT_EQ(1,mc_test_engine(TEST_DCA_INVALID_FLAGS));
#endif
}

TEST(Cflash_FVT_Suite, E_MC_test_DCA_Twice)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_CALL_TWICE));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCA_Reuse_Context_Flag)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_REUSE_CTX_FLAG));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCA_Reuse_Context_Flag_On_New_Plun_Disk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_REUSE_CTX_FLAG_NEW_PLUN_DISK));
}


TEST(Cflash_FVT_Suite, G_MC_test_DCA_Reuse_Context_Flag_On_New_Vlun_Disk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_REUSE_CTX_NEW_VLUN_DISK));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCA_Reuse_Context_All_Capi_Disk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_REUSE_CTX_ALL_CAPI_DISK));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCA_Reuse_Context_Of_Detach_Ctx)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_REUSE_CTX_OF_DETACH_CTX));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCA_Reuse_Context_Of_Relased_Ioctl)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_REUSE_CTX_OF_RELASED_IOCTL));
}

/***  DK_CAPI_RECOVER_CTX  ***/
TEST(Cflash_FVT_Suite, G_MC_test_DCRC_No_EEH)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_NO_EEH));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCRC_Reattach_Detach_Ctx)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_DETACHED_CTX));
}

TEST(Cflash_FVT_Suite, G_TEST_MAX_VLUNS_ONE_CTX)
{
    ASSERT_EQ(0,mc_test_engine(G_TEST_MAX_VLUNS));
}

#ifdef _AIX
TEST(Cflash_FVT_Suite, E_MC_test_DCRC_Invalid_Devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_INVALID_DEVNO));
}
#endif

TEST(Cflash_FVT_Suite, E_MC_test_DCRC_Invalid_Flag)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_INVALID_FLAG));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCRC_Invalid_Reason)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_INVALID_REASON));
}

/***  DK_CAPI_USER_DIRECT ***/
#ifdef _AIX
TEST(Cflash_FVT_Suite, E_MC_test_DCUD_Invalid_Devno_Valid_Contx_ID)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_INVALID_DEVNO_VALID_CTX));
}
#endif
TEST(Cflash_FVT_Suite, E_MC_test_DCUD_Invalid_Contx_Valid_Devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_INVALID_CTX_VALID_DEVNO));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCUD_Valid_Contx_Valid_Devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_VALID_CTX_VALID_DEVNO));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCUD_Flags)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_FLAGS));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCUD_Twice_Same_Contx_Devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_TWICE_SAME_CTX_DEVNO));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCUD_With_Vlun_created_On_Same_Disk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_VLUN_ALREADY_CREATED_SAME_DISK));
}
TEST(Cflash_FVT_Suite, E_MC_test_DCUD_With_Plun_created_On_Same_Disk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_PLUN_ALREADY_CREATED_SAME_DISK));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCUD_With_Vlun_Created_destroyed_On_Same_Disk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_VLUN_CREATED_DESTROYED_SAME_DISK));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCUD_In_Loop)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_IN_LOOP));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCUD_Path_Id_Mask_Values)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_PATH_ID_MASK_VALUES));
}

#ifdef _AIX
TEST(Cflash_FVT_Suite, E_MC_test_DCUD_Invalid_Path_Id_Mask_Values)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_BAD_PATH_ID_MASK_VALUES));
}
/***  DK_CAPI_USER_VIRTUAL ***/

TEST(Cflash_FVT_Suite, E_MC_test_DCUV_Invalid_Devno_Valid_Contx)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_INVALID_DEVNO_VALID_CTX));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCUV_Invalid_Contx_Invalid_Devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_INVALID_CTX_INVALID_DEVNO));
}
#endif

TEST(Cflash_FVT_Suite, E_MC_test_DCUV_Valid_Devno_Invalid_Contx)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_VALID_DEVNO_INVALID_CTX));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCUV_Vlun_Size_Zero)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_LUN_VLUN_SIZE_ZERO));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCUV_With_Plun_Already_Created_On_Same_Disk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_PLUN_ALREADY_CREATED_SAME_DISK));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCUV_With_Vlun_Already_Created_On_Same_Disk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_VLUN_ALREADY_CREATED_SAME_DISK));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCUV_With_No_Further_Vlun_Capacity)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_NO_FURTHER_VLUN_CAPACITY));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCUV_With_Multiple_Vluns_On_Same_Capacity_Same_Disk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_MTPLE_VLUNS_SAME_CAPACITY_SAME_DISK));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCUV_With_Twice_Same_Contx_Devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_TWICE_SAME_CTX_DEVNO));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCUV_Vlun_Max_Size)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_VLUN_MAX));
}
TEST(Cflash_FVT_Suite, E_MC_test_DCUV_Vlun_size_More_Than_Disk_Size)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_VLUN_SIZE_MORE_THAN_DISK_SIZE));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCUV_With_Plun_Created_Destroyed_On_Same_Disk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_PLUN_CREATED_DESTROYED_SAME_DISK));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCUV_With_Contx_Of_PLUN)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_WITH_CTX_OF_PLUN));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCUD_With_Contx_Of_VLUN)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUD_WITH_CTX_OF_VLUN));
}

#ifdef _AIX
TEST(Cflash_FVT_Suite, G_MC_test_DCUV_Path_ID_Mask_Values)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_PATH_ID_MASK_VALUES));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCUV_Invalid_Path_ID_Mask_Values)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_INVALID_PATH_ID_MASK_VALUES));
}
#endif

TEST(Cflash_FVT_Suite, G_MC_test_DCUV_In_Loop)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCUV_IN_LOOP));
}
/***  DK_CAPI_VLUN_RESIZE ***/
#ifdef _AIX
TEST(Cflash_FVT_Suite, E_MC_test_DCVR_Invalid_devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_INVALID_DEVNO));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCVR_Invalid_devno_Ctx)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_INVALID_CTX_DEVNO));
}
#endif

TEST(Cflash_FVT_Suite, E_MC_test_DCVR_Invalid_Ctx)
{
    ASSERT_EQ(22,mc_test_engine(TEST_DCVR_INVALID_CTX));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCVR_No_Vlun)
{
    ASSERT_EQ(22,mc_test_engine(TEST_DCVR_NO_VLUN));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCVR_On_Plun)
{
    ASSERT_EQ(22,mc_test_engine(TEST_DCVR_ON_PLUN));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCVR_Gt_Disk_size)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_GT_DISK_SIZE));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCVR_Not_factor_256MB)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_NOT_FCT_256MB));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCVR_EQ_CT_Vlun_size)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_EQ_CT_VLUN_SIZE));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCVR_LT_CT_Vlun_size)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_LT_CT_VLUN_SIZE));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCVR_GT_CT_Vlun_size)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_GT_CT_VLUN_SIZE));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCVR_EG_Disk_size_None_Vlun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_EQ_DISK_SIZE_NONE_VLUN));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCVR_EQ_Disk_size_other_Vlun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_EQ_DISK_SIZE_OTHER_VLUN));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCVR_INC_256MB)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_INC_256MB));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCVR_DEC_256MB)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_DEC_256MB));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCVR_GT_CT_Vlun_LT_256MB)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_GT_CT_VLUN_LT_256MB));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCVR_LT_CT_Vlun_LT_256MB)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_LT_CT_VLUN_LT_256MB));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCVR_Inc_Dec_Loop)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCVR_INC_DEC_LOOP));
}
TEST(Cflash_FVT_Suite, G_MC_test_DCVR_zero_vlun_size)
{
    ASSERT_EQ(0,mc_test_engine(G_MC_test_DCVR_ZERO_Vlun_size));
}

/*** DK_CAPI_RELEASE ***/
#ifdef _AIX
TEST(Cflash_FVT_Suite, E_MC_test_DCR_Invalid_devno)
{
    ASSERT_EQ(22,mc_test_engine(TEST_DCR_INVALID_DEVNO));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCR_Invalid_devno_Ctx)
{
    ASSERT_EQ(22,mc_test_engine(TEST_DCR_INVALID_DEVNO_CTX));
}
#endif
TEST(Cflash_FVT_Suite, E_MC_test_DCR_Invalid_Ctx)
{
    ASSERT_EQ(22,mc_test_engine(TEST_DCR_INVALID_CTX));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCR_No_Vlun)
{
    ASSERT_EQ(22,mc_test_engine(TEST_DCR_NO_VLUN));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCR_Plun_again)
{
    ASSERT_EQ(22,mc_test_engine(TEST_DCR_PLUN_AGIAN));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCR_Vlun_again)
{
    ASSERT_EQ(22,mc_test_engine(TEST_DCR_VLUN_AGIAN));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCR_Multiple_Vlun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCR_MULTP_VLUN));
}
TEST(Cflash_FVT_Suite, E_MC_test_DCR_inv_res)
{
    ASSERT_EQ(22,mc_test_engine(TEST_DCR_VLUN_INV_REL));
}

/*** DK_CAPI_DETACH ***/
#ifdef _AIX
TEST(Cflash_FVT_Suite, E_MC_test_DCD_Invalid_Devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCD_INVALID_DEVNO));
}
#endif
TEST(Cflash_FVT_Suite, E_MC_test_DCD_Invalid_Ctx)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCD_INVALID_CTX));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCD_Twice_on_Plun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCD_TWICE_ON_PLUN));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCD_Twice_on_vlun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCD_TWICE_ON_VLUN));
}

/*** DK_CAPI_VERIFY ***/
#ifdef _AIX
TEST(Cflash_FVT_Suite, E_MC_test_DCV_Invalid_Devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_INVALID_DEVNO));
}
#endif

TEST(Cflash_FVT_Suite, E_MC_test_DCV_Invalid_flags)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_INVALID_FLAGS));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCV_Invalid_Res_handle)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_INVALID_RES_HANDLE));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCV_No_error)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_NO_ERR));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCV_Vlun_Rst_flag)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_VLUN_RST_FlAG));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCV_Vlun_Tur_flag)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_VLUN_TUR_FLAG));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCV_Vlun_Inq_flag)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_VLUN_INQ_FLAG));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCV_Vlun_Hint_Sense)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_VLUN_HINT_SENSE));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCV_Plun_Rst_flag)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_PLUN_RST_FlAG));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCV_Plun_Tur_flag)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_PLUN_TUR_FLAG));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCV_Plun_Inq_flag)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_PLUN_INQ_FLAG));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCV_Plun_Hint_Sense)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_PLUN_HINT_SENSE));
}

TEST(Cflash_FVT_Suite, G_spio_single_0_vlun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_SPIO_0_VLUN));
}
TEST(Cflash_FVT_Suite, G_spio_single_vlun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_SPIO_VLUN));
}
TEST(Cflash_FVT_Suite, G_spio_single_plun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_SPIO_A_PLUN));
}

TEST(Cflash_FVT_Suite, E_spio_no_res_afurc)
{
#ifdef _AIX
    ASSERT_EQ(0x1,mc_test_engine(TEST_SPIO_NORES_AFURC));
#else
    ASSERT_EQ(0x5,mc_test_engine(TEST_SPIO_NORES_AFURC));
#endif
}
TEST(Cflash_FVT_Suite, G_sp_io_all_pluns)
{
    ASSERT_EQ(0,mc_test_engine(TEST_SPIO_ALL_PLUN));
}

TEST(Cflash_FVT_Suite, G_sp_io_all_vluns)
{
    ASSERT_EQ(0,mc_test_engine(TEST_SPIO_ALL_VLUN));
}

TEST(Cflash_FVT_Suite, G_sp_io_vlun_plun_altr)
{
    ASSERT_EQ(0,mc_test_engine(TEST_SPIO_VLUN_PLUN));
}

TEST(Cflash_FVT_Suite, G_mc_ctx_crt_dstr_rgrs)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_REGRESS_CTX_CRT_DSTR));
}
TEST(Cflash_FVT_Suite, G_mc_ctx_crt_dstr_rgrs_io)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_REGRESS_CTX_CRT_DSTR_IO));
}

TEST(Cflash_FVT_Suite, G_mc_two_ctx_rd_wrthrd)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_TWO_CTX_RD_WRTHRD));
}

TEST(Cflash_FVT_Suite, G_mc_two_ctx_rdwr_size)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_TWO_CTX_RDWR_SIZE));
}

TEST(Cflash_FVT_Suite, G_mc_wr_thrd_rd_thrd)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_ONE_CTX_TWO_THRD));
}

TEST(Cflash_FVT_Suite, G_mc_rdwr_thrd_size_thrd)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_ONE_CTX_RD_WRSIZE));
}

TEST(Cflash_FVT_Suite, G_max_res_handl_one_ctx)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_MAX_RES_HNDL));
}

TEST(Cflash_FVT_Suite, G_one_unit_chunk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_ONE_UNIT_SIZE));
}

TEST(Cflash_FVT_Suite, G_mc_lun_cap_incremnt)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_MAX_SIZE));
}

TEST(Cflash_FVT_Suite, G_spio_vlun_attach_detach)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_SPIO_VLUN_ATCH_DTCH));
}

TEST(Cflash_FVT_Suite, G_spio_plun_attach_detach)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_SPIO_PLUN_ATCH_DTCH));
}

//----------------------

TEST(Cflash_FVT_Suite, E_Test_mc_afu_rc_invalid_opcode)
{
    ASSERT_EQ(0x21, test_mc_invalid_ioarcb(1));
}

TEST(Cflash_FVT_Suite, E_Test_mc_ioarcb_ea_null)
{
#ifdef _AIX
	ASSERT_EQ(7, test_mc_invalid_ioarcb(2));
#else
    int res = test_mc_invalid_ioarcb(2);
    ASSERT_EQ(0xff,res);
#endif
}

TEST(Cflash_FVT_Suite, E_Test_mc_ioarcb_invalid_flags)
{
    ASSERT_EQ(0x58, test_mc_invalid_ioarcb(3));
}

TEST(Cflash_FVT_Suite, E_Test_mc_ioarcb_invalid_lun_fc_port)
{
    int res = test_mc_invalid_ioarcb(4);
    if (res == 0x21 || res == 0x5 || res == 0x3)
        res =1;
    ASSERT_EQ(1, res);
}

TEST(Cflash_FVT_Suite, E_Test_mc_afu_rc_rht_invalid)
{
    ASSERT_EQ(0x5, test_mc_invalid_ioarcb(5));
}

TEST(Cflash_FVT_Suite, E_Test_mc_afu_rc_rht_out_of_bounds)
{
    int res = test_mc_invalid_ioarcb(6);
    if (res == 0x3 || res == 0x5)
		res=1;
	ASSERT_EQ(1, res);
}

TEST(Cflash_FVT_Suite, E_Test_mc_error_page_fault)
{
#ifdef _AIX
    ASSERT_EQ(7, test_mc_invalid_ioarcb(7));
#else
    ASSERT_EQ(0xff, test_mc_invalid_ioarcb(7));
#endif

}
TEST(Cflash_FVT_Suite, E_Test_mc_ioarcb_invalid_ctx_id)
{
    ASSERT_EQ(0x21, test_mc_invalid_ioarcb(8));
}

TEST(Cflash_FVT_Suite, E_Test_rc_flags_underrun)
{
    ASSERT_EQ(0x2, test_mc_invalid_ioarcb(9));
}

TEST(Cflash_FVT_Suite, E_Test_rc_flags_overrun)
{
    ASSERT_EQ(0x2, test_mc_invalid_ioarcb(10));
}

TEST(Cflash_FVT_Suite, E_Test_scsi_rc_check)
{
    ASSERT_EQ(0x2, test_mc_invalid_ioarcb(11));
}
TEST(Cflash_FVT_Suite, E_Test_ioarcb_d_len_0)
{
    ASSERT_EQ(0x2, test_mc_invalid_ioarcb(12));
}
TEST(Cflash_FVT_Suite, E_Test_ioarcb_blk_len_0)
{
    ASSERT_EQ(0x0, test_mc_invalid_ioarcb(13));
}

TEST(Cflash_FVT_Suite, E_Test_ioarcb_vlba_out_range)
{
    ASSERT_EQ(0x13, test_mc_invalid_ioarcb(14));
}

TEST(Cflash_FVT_Suite, E_Test_ioarcb_plba_out_range)
{
    ASSERT_EQ(0x2, test_mc_invalid_ioarcb(15));
}

TEST(Cflash_FVT_Suite, E_bad_ioarcb_address)
{
    ASSERT_EQ(100, test_mc_invalid_ioarcb(100));
}

TEST(Cflash_FVT_Suite, E_bad_ioasa_address)
{
#ifdef _AIX
    ASSERT_EQ(255, test_mc_invalid_ioarcb(101));
#else
    int res = test_mc_invalid_ioarcb(101);
    if ( res == 10 || res == 255 )
        res = 1; 
        
    ASSERT_EQ(1, res);
#endif
}

TEST(Cflash_FVT_Suite, E_cmd_room_violation)
{
	int rc = test_mc_invalid_ioarcb(102);
#ifdef _AIX
	if(NUM_CMDS <= 4)
		 ASSERT_EQ(255,rc);
	else
		ASSERT_EQ(102,rc);
#else
	if(NUM_CMDS <= 16)
		 ASSERT_EQ(0,rc);
	else
		ASSERT_EQ(102,rc);
#endif
}

TEST(Cflash_FVT_Suite, E_bad_hrrq_address)
{
    ASSERT_EQ(103, test_mc_invalid_ioarcb(103));
}

TEST(Cflash_FVT_Suite, E_Test_ioarcb_intr_prcs_ctx)
{
    ASSERT_EQ(0x21, test_mc_inter_prcs_ctx(1));
}
TEST(Cflash_FVT_Suite, E_Test_ioarcb_intr_prcs_ctx_rsh_clsd)
{
    ASSERT_EQ(0x21, test_mc_inter_prcs_ctx(2));
}
TEST(Cflash_FVT_Suite, G_Test_mc_ioarcb_ea_alignmnt_16)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_IOARCB_EA_ALGNMNT_16));
}

TEST(Cflash_FVT_Suite, G_Test_mc_ioarcb_ea_alignmnt_128)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_IOARCB_EA_ALGNMNT_128));
}

TEST(Cflash_FVT_Suite, G_Test_mc_ioarcb_ea_mix_alignmnt)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_IOARCB_EA_INVLD_ALGNMNT));
}
TEST(Cflash_FVT_Suite, E_Test_scsi_cmds)
{
    ASSERT_EQ(0x21,mc_test_engine(E_test_SCSI_CMDS));
}

TEST(Cflash_FVT_Suite, G_Test_mc_rwbuff_in_global)
{
    ASSERT_EQ(0,mc_test_engine(MC_TEST_RWBUFF_GLOBAL));
}

TEST(Cflash_FVT_Suite, G_Test_mc_rwbuff_in_shared_memory)
{
    ASSERT_EQ(0,mc_test_engine(MC_TEST_RWBUFF_SHM));
}

TEST(Cflash_FVT_Suite, G_Test_mc_rwbuff_in_heap)
{
    ASSERT_EQ(0,mc_test_engine(MC_TEST_RWBUFF_HEAP));
}

TEST(Cflash_FVT_Suite, G_Test_mc_rw_size_parallel_sync)
{
    ASSERT_EQ(0,mc_test_engine(MC_TEST_RW_SIZE_PARALLEL));
}
TEST(Cflash_FVT_Suite, E_spio_good_path_disk_err_path_disk)
{
    ASSERT_EQ(0,mc_test_engine(MC_TEST_GOOD_ERR_AFU_DEV));
}

TEST(Cflash_FVT_Suite, E_Test_mc_rw_close_res_hndl)
{
    int res = mc_test_engine(TEST_MC_RW_CLS_RSH);
    if (res == 0x13 || res == 0x05)
        res =1;
    ASSERT_EQ(1, res);
}
TEST(Cflash_FVT_Suite, E_Test_mc_rw_close_ctx)
{
#ifdef _AIX
	int res=mc_test_engine(TEST_MC_RW_CLOSE_CTX);
	if(res == 10 || res == 0x5 || res == 0x13 || res == 0)
		res=1;
	ASSERT_EQ(1,res);
#else
    ASSERT_NE(0,mc_test_engine(TEST_MC_RW_CLOSE_CTX));
#endif
}
#ifndef _AIX
TEST(Cflash_FVT_Suite, E_Test_mc_rw_close_disk_fd)
{
    int res=mc_test_engine(TEST_MC_RW_CLOSE_DISK_FD);
	if(res == 0x5 || res == 0x0|| res == 10||res == 0x19)
		res=1;
    ASSERT_EQ(1,res);
}
TEST(Cflash_FVT_Suite, E_Test_mc_rw_unmap_mmio)
{
    ASSERT_EQ(10,mc_test_engine(TEST_MC_RW_UNMAP_MMIO));
}
#endif
TEST(Cflash_FVT_Suite, G_Test_large_transfer_io)
{
    ASSERT_EQ(0,mc_test_engine(TEST_LARGE_TRANSFER_IO));
}

TEST(Cflash_FVT_Suite, E_Test_large_trsnfr_boundary)
{
	int res = mc_test_engine(TEST_LARGE_TRNSFR_BOUNDARY);
	if(res == 0x2 || res == 255)
		res=1;
    ASSERT_EQ(1, res);
}


TEST(Cflash_FVT_Suite, E_test_mmio_errcase1)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MMIO_ERRCASE1));
}

TEST(Cflash_FVT_Suite, E_test_mmio_errcase2)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MMIO_ERRCASE2));
}

TEST(Cflash_FVT_Suite, E_test_mmio_errcase3)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MMIO_ERRCASE3));
}


TEST(Cflash_FVT_Suite, E_test_spio_killprocess)
{
    ASSERT_EQ(0,mc_test_engine(TEST_SPIO_KILLPROCESS));
}

TEST(Cflash_FVT_Suite, E_test_spio_exit)
{
    ASSERT_EQ(0,mc_test_engine(TEST_SPIO_EXIT));
}

TEST(Cflash_FVT_Suite, E_test_ioctl_spio_errcase)
{
    ASSERT_EQ(0,mc_test_engine(TEST_IOCTL_SPIO_ERRCASE));
}
TEST(Cflash_FVT_Suite, E_test_attach_reuse_diff_proc)
{
    ASSERT_EQ(0,mc_test_engine(TEST_ATTACH_REUSE_DIFF_PROC));
}

TEST(Cflash_FVT_Suite, E_test_detach_diff_proc)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DETACH_DIFF_PROC));
}

TEST(Cflash_FVT_Suite, E_test_vlun_verify)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_VLUN_VERIFY));
}

TEST(Cflash_FVT_Suite, E_test_plun_verify)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_PLUN_VERIFY));
}


TEST(Cflash_FVT_Suite,E_test_inval_devno)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_INVAL_DEVNO));
}
TEST(Cflash_FVT_Suite,E_test_inval_ctxtkn)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_INVAL_CTXTKN));
}

TEST(Cflash_FVT_Suite,E_test_inval_rschndl)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_INVAL_RSCHNDL));
}

#ifndef _AIX
TEST(Cflash_FVT_Suite,G_dk_capi_clone)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DK_CAPI_CLONE));
}
#endif
#ifdef _AIX
TEST(Cflash_FVT_Suite, G_MC_test_MRC_MC_VLUN)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_119));
}

TEST(Cflash_FVT_Suite, E_MC_test_MRC_MC_PLUN)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_120));
}

TEST(Cflash_FVT_Suite, E_test_SPIO_RMDEV)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_180));
}
TEST(Cflash_FVT_Suite, E_test_SPIO_RMDEV_1)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_1801));
}
TEST(Cflash_FVT_Suite, E_test_SPIO_LSM)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_181));
}
TEST(Cflash_FVT_Suite, E_test_CFL_OPEN_SPIO)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_182));
}
TEST(Cflash_FVT_Suite, G_test_SPIO_CFG_UNCFG)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_187));
}
TEST(Cflash_FVT_Suite, E_test_QRY_NO_RES)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_212));
}
TEST(Cflash_FVT_Suite, E_test_QRY_PATH_DIS)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_213));
}
#endif
/*
TEST(Cflash_FVT_Suite, G_7_1_214)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_214));
}
*/
TEST(Cflash_FVT_Suite, E_test_REUSE_DTCH)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_215));
}
TEST(Cflash_FVT_Suite, E_test_REUSE_DTCH_LOOP)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_216));
}

TEST(Cflash_FVT_Suite, E_test_SPIO_RLS_DET)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_190));
}
#ifdef _AIX
TEST(Cflash_FVT_Suite, G_test_CFG_UNCFG_VLUN)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_191));
}
TEST(Cflash_FVT_Suite, G_test_ATH_DTCH_VLUN)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_192));
}
TEST(Cflash_FVT_Suite, G_test_SPIO_NCHN)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_193_1));
}

TEST(Cflash_FVT_Suite, G_test_SPIO_NCHN_2)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_193_2));
}
#endif

TEST(Cflash_FVT_Suite, G_test_MUL_PLUN_VLUN)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_196));
}

#ifndef _AIX
TEST(Cflash_FVT_Suite, E_test_CHN_CTX_ID)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_197));
}
#endif
TEST(Cflash_FVT_Suite, E_test_CHN_CTX_ID_DIF_PRC)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_198));
}

#ifdef _AIX
TEST(Cflash_FVT_Suite, E_test_VG_V_PLUN)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_209));
}
#endif
TEST(Cflash_FVT_Suite, E_test_RD_PRM_WRITE)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_210));
}

TEST(Cflash_FVT_Suite, E_test_SPIO_VLUN_PLUN_SIM)
{
    ASSERT_EQ(255,mc_test_engine(E_ioctl_7_1_211));
}
TEST(Cflash_FVT_Suite, E_test_context_reset)
{
    ASSERT_EQ(0,mc_test_engine(E_TEST_CTX_RESET));
}
TEST(Cflash_FVT_Suite, G_test_max_ctx_plun)
{
    ASSERT_EQ(0,mc_test_engine(G_TEST_MAX_CTX_PLUN));
}
TEST(Cflash_FVT_Suite, G_test_max_ctx_0_vlun)
{
    ASSERT_EQ(0,mc_test_engine(G_TEST_MAX_CTX_0_VLUN));
}

TEST(Cflash_FVT_Suite, G_test_max_ctx_only)
{
    ASSERT_EQ(0,mc_test_engine(G_TEST_MAX_CTX_ONLY));
}
TEST(Cflash_FVT_Suite, G_test_max_ctx_io_noflg)
{
    ASSERT_EQ(0,mc_test_engine(G_TEST_MAX_CTX_IO_NOFLG));
}
TEST(Cflash_FVT_Suite, G_sp_io_size_regress)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_SIZE_REGRESS));
}
TEST(Cflash_FVT_Suite, G_mc_regress_resource)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MC_REGRESS_RESOURCE));
}
TEST(Cflash_FVT_Suite, E_test_max_ctx_crss_limit)
{
    ASSERT_EQ(0,mc_test_engine(E_TEST_MAX_CTX_CRSS_LIMIT));
}
TEST(Cflash_FVT_Suite, G_max_ctx_res_unit_lun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MAX_CTX_RES_UNIT));
}

TEST(Cflash_FVT_Suite, G_max_ctx_res_lun_capacity)
{
    ASSERT_EQ(0,mc_test_engine(TEST_MAX_CTX_RES_LUN_CAP));
}
TEST(Cflash_FVT_Suite, G_7_1_203)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_203));
}
#ifdef MANUAL

TEST(Cflash_FVT_Suite, M_7_5_13_1)
{
    ASSERT_EQ(0,mc_test_engine(M_TEST_7_5_13_1));
}

TEST(Cflash_FVT_Suite, M_7_5_13_2)
{
    ASSERT_EQ(0,mc_test_engine(M_TEST_7_5_13_2));
}

TEST(Cflash_FVT_Suite, E_test_fc_reset_vlun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_FC_PR_RESET_VLUN));
}
TEST(Cflash_FVT_Suite, E_test_fc_reset_plun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_FC_PR_RESET_PLUN));
}

TEST(Cflash_FVT_Suite, E_MC_test_DCA_EEH_Flag_Set_Reuse_CTX_On_New_Disk)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_REUSE_CTX_NEW_DISK_AFTER_EEH));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCRC_EEH_of_VLUN)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_EEH_VLUN));
}
TEST(Cflash_FVT_Suite, G_MC_test_DCRC_EEH_Vlun_Resuse_Ctx)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_EEH_VLUN_RESUSE_CTX));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCRC_EEH_Plun_Resuse_Ctx)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_EEH_PLUN_RESUSE_CTX));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCRC_EEH_Vlun_Resize)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_EEH_VLUN_RESIZE));
}
TEST(Cflash_FVT_Suite, G_MC_test_DCRC_EEH_Vlun_Release)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_EEH_VLUN_RELEASE));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCRC_IO_Eeh_vlun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_IO_EEH_VLUN));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCRC_IO_Eeh_Plun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_IO_EEH_PLUN));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCV_Unexpected_error)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_UNEXPECTED_ERR));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCV_Unexpected_error_vlun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_UNEXPECTED_ERR_VLUN));
}
TEST(Cflash_FVT_Suite, G_MC_test_DCV_Plun_Rst_flag_EEH)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCV_PLUN_RST_FlAG_EEH));
}
/*** DK_CAPI_LOG_EVENT ****/

#ifdef _AIX
TEST(Cflash_FVT_Suite, G_MC_test_DCLE_Valid_values)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCLE_VALID_VALUES));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCLE_Dk_Lf_Temp)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCLE_DK_LF_TEMP));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCLE_DK_Lf_Perm)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCLE_DK_LF_PERM));
}
TEST(Cflash_FVT_Suite, G_MC_test_DCLE_Dk_Fl_Hw_Err)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCLE_DK_FL_HW_ERR));
}

TEST(Cflash_FVT_Suite, G_MC_test_DCLE_Dk_Fl_Sw_Err)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCLE_DK_FL_SW_ERR));
}

TEST(Cflash_FVT_Suite, E_test_ioctl_fcp)
{
    ASSERT_EQ(0,mc_test_engine(TEST_IOCTL_FCP));
}
#endif

TEST(Cflash_FVT_Suite, E_test_vSpio_eehRecovery)
{
    ASSERT_EQ(0,mc_test_engine(TEST_VSPIO_EEHRECOVERY));
}

TEST(Cflash_FVT_Suite, E_test_dSpio_eehRecovery)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DSPIO_EEHRECOVERY));
}

TEST(Cflash_FVT_Suite, E_test_vSpio_eehRecovery_long)
{
	int rc=get_fvt_dev_env();
	if(rc)
		ASSERT_EQ(0,rc);	
    ASSERT_EQ(0,test_vSpio_eehRecovery(2));
}
TEST(Cflash_FVT_Suite, E_test_dSpio_eehRecovery_long)
{
	int rc=get_fvt_dev_env();
	if(rc)
		ASSERT_EQ(0,rc);	
    ASSERT_EQ(0,test_dSpio_eehRecovery(2));
}
TEST(Cflash_FVT_Suite, E_test_vlun_disbale)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_VLUN_DISABLE));
}

TEST(Cflash_FVT_Suite, E_test_plun_disbale)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_PLUN_DISABLE));
}
/*
TEST(Cflash_FVT_Suite, E_test_vlun_increase)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_VLUN_INCREASE ));
}

TEST(Cflash_FVT_Suite,E_test_vlun_reduce)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_VLUN_REDUCE));
}
*/
TEST(Cflash_FVT_Suite, E_max_ctx_rcvr_except_last_one)
{
    ASSERT_EQ(0,mc_test_engine(MAX_CTX_RCVR_EXCEPT_LAST_ONE));
}

TEST(Cflash_FVT_Suite, E_max_ctx_rcvr_last_one_no_rcvr)
{
    ASSERT_EQ(0,mc_test_engine(MAX_CTX_RCVR_LAST_ONE_NO_RCVR));
}

TEST(Cflash_FVT_Suite,E_test_eeh_simulation)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_EEH_SIMULATION));
}

TEST(Cflash_FVT_Suite,E_test_disk_incrs_excp)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_DISK_INCREASE));
}

TEST(Cflash_FVT_Suite, E_test_VLUN_TDIO)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_174));
}

TEST(Cflash_FVT_Suite, E_test_TDIO_VLUN)
{
    ASSERT_EQ(0,mc_test_engine(E_ioctl_7_1_175));
}

TEST(Cflash_FVT_Suite, E_test_LUN_RESET )
{
   printf("\n================== Manual ========================\n");
   printf("Test: G_7_1_203 need to be run from one session &\n"); 
   printf("Error injection from another session using \n"); 
   printf("'sg_reset -H /dev/sg#' And 'sg_reset -d /dev/sg#'.\n");
   printf("Ex: While true; do sg_reset -H /dev/sg#; sleep 5;\n");
   printf("sg_reset -d /dev/sg#; sleep 5; done");
   printf("\n================== Manual ========================\n");
   ASSERT_EQ(1,0);
}

#ifndef _AIX
TEST(Cflash_FVT_Suite, E_CAPI_LINK_DOWN)
{
    ASSERT_EQ(0,mc_test_engine(E_CAPI_LINK_DOWN));
}
#endif

#ifdef _AIX
TEST(Cflash_FVT_Suite, G_MC_test_DCA_Diff_Devno_Multiple)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCA_CALL_DIFF_DEVNO_MULTIPLE));
}
TEST(Cflash_FVT_Suite, E_test_cfdisk_ctxs_diff_devno)
{
    ASSERT_EQ(0,mc_test_engine(TEST_CFDISK_CTXS_DIFF_DEVNO));
}
#endif
TEST(Cflash_FVT_Suite, G_MC_test_DCRC_EEH_Plun_Mutli_Vlun)
{
    ASSERT_EQ(0,mc_test_engine(TEST_DCRC_EEH_PLUN_MULTI_VLUN));
}
TEST(Cflash_FVT_Suite,E_test_vlun_uattention)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_VLUN_UATTENTION));
}

TEST(Cflash_FVT_Suite,E_test_plun_uattention)
{
    ASSERT_EQ(0,mc_test_engine(EXCP_PLUN_UATTENTION));
}
TEST(Cflash_FVT_Suite, G_test_SPIO_VLUN_CFG_UNCFG)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_188));
}

TEST(Cflash_FVT_Suite, G_test_SPIO_PLUN_CFG_UNCFG)
{
    ASSERT_EQ(0,mc_test_engine(G_ioctl_7_1_189));
}
#endif

