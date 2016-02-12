/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/cflash/test/cflash_test_engine.c $                        */
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
#include "cflash_test.h"

int test1()
{
    return -1;
}

int mc_test_engine(mc_test_t test_name)
{
    int rc = 0;
    if (get_fvt_dev_env()) return -1;
#ifdef _AIX
    //system("ctctrl -c cflashdd -r  memtraceoff");
    system("ctctrl -c cflashdd -r  memtraceon");
    if (DEBUG)
        system("ctctrl -c cflashdd -r  -q");
#else
    if (DEBUG)
        system("echo \"module cxlflash +p\" > /sys/kernel/debug/dynamic_debug/control");
    displayBuildinfo();
#endif

    if (DEBUG)
    {
        system("echo;echo ---------------------- Test Start Timestamp ----------------------");
        system("date");
        system("echo ------------------------------------------------------------------; echo");
    }

    if (fork() == 0)
    {
        //child process
        switch (test_name)
        {
                /*** DK_CAPI_QUERY_PATH ***/
#ifdef _AIX
            case TEST_DCQP_VALID_PATH_COUNT:
                rc = test_dcqp_ioctl(1);
                break;
#endif
            case TEST_IOCTL_INVALID_VERSIONS:
                rc = test_all_ioctl_invalid_version();
                break;
#ifdef _AIX
            case TEST_DCQP_INVALID_PATH_COUNT:
                rc = test_dcqp_error_ioctl(1);
                break;
            case TEST_DCQP_DUAL_PATH_COUNT:
                rc = test_dcqp_ioctl(2);
                break;
            case TEST_DCQP_DK_CPIF_RESERVED:
                rc = test_dcqp_ioctl(3);
                break;
            case TEST_DCQP_DK_CPIF_FAILED:
                rc = test_dcqp_ioctl(4);
                break;
#endif
                /**** DK_CAPI_ATTACH ***/
            case TEST_DCA_OTHER_DEVNO: //other devno
                rc = test_dca_error_ioctl(1);
                break;
            case TEST_DCA_INVALID_DEVNO:
                rc = test_dca_error_ioctl(2);//invalid path -1
                break;
            case TEST_DCA_INVALID_INTRPT_NUM:
                rc = test_dca_error_ioctl(3);//invalid -1 intrpt number
                break;
            case TEST_DCA_VALID_VALUES:
                rc = test_dca_ioctl(1);//Valid values
                break;
                /* IBM Data Engine for NoSQL - Power Systems Edition User Library Project */
            case TEST_DCA_INVALID_FLAGS:
                rc = test_dca_error_ioctl(4);
                break;
            case TEST_DCA_CALL_TWICE:
                rc = test_dca_error_ioctl(5);
                break;
            case TEST_DCA_CALL_DIFF_DEVNO_MULTIPLE:
                rc = test_dca_ioctl(2);
                break;
            case TEST_DCA_REUSE_CTX_FLAG:
                rc = test_dca_error_ioctl(6);
                break;
            case TEST_DCA_REUSE_CTX_FLAG_NEW_PLUN_DISK:
                rc = test_dca_error_ioctl(7);
                break;
            case TEST_DCA_REUSE_CTX_NEW_VLUN_DISK:
                rc = test_dca_error_ioctl(8);
                break;
            case TEST_DCA_REUSE_CTX_ALL_CAPI_DISK:
                rc = test_dca_error_ioctl(12);
                break;
            case TEST_DCA_REUSE_CTX_OF_DETACH_CTX:
                rc = test_dca_error_ioctl(9);
                break;
            case TEST_DCA_REUSE_CTX_OF_RELASED_IOCTL:
                rc = test_dca_error_ioctl(11);
                break;
            case TEST_DCA_REUSE_CTX_NEW_DISK_AFTER_EEH:
                rc = test_dca_error_ioctl(10);
                break;
                /***  DK_CAPI_RECOVER_CTX  ***/
            case TEST_DCRC_NO_EEH:
                rc = test_dcrc_ioctl(1);
                break;
            case TEST_DCRC_DETACHED_CTX:
                rc = test_dcrc_ioctl(2);
                break;
            case TEST_DCRC_EEH_VLUN:
                rc = test_dcrc_ioctl(3);
                break;
            case TEST_DCRC_EEH_PLUN_MULTI_VLUN:
                rc = test_dcrc_ioctl(4);
                break;
            case TEST_DCRC_EEH_VLUN_RESUSE_CTX:
                rc = test_dca_error_ioctl(15);
                break;
            case TEST_DCRC_EEH_PLUN_RESUSE_CTX:
                rc = test_dca_error_ioctl(16);
                break;
            case TEST_DCRC_EEH_VLUN_RESIZE:
                rc = test_dcrc_ioctl(7);
                break;
            case TEST_DCRC_EEH_VLUN_RELEASE:
                rc = test_dcrc_ioctl(8);
                break;
            case TEST_DCRC_INVALID_DEVNO:
                rc = test_dcrc_ioctl(9);
                break;
            case TEST_DCRC_INVALID_FLAG:
                rc = test_dcrc_ioctl(10);
                break;
            case TEST_DCRC_INVALID_REASON:
                rc = test_dcrc_ioctl(11);
                break;
            case TEST_DCRC_IO_EEH_VLUN:
                rc = test_dcrc_ioctl(12);
                break;
            case TEST_DCRC_IO_EEH_PLUN:
                rc = test_dcrc_ioctl(13);
                break;
                /***  DK_CAPI_USER_DIRECT ***/
            case TEST_DCUD_INVALID_DEVNO_VALID_CTX:
                rc = test_dcud_error_ioctl(1);
                break;
            case TEST_DCUD_INVALID_CTX_VALID_DEVNO:
                rc = test_dcud_error_ioctl(2);
                break;
            case TEST_DCUD_VALID_CTX_VALID_DEVNO:
                rc = test_dcud_ioctl(1);
                break;
            case TEST_DCUD_FLAGS:
                rc = test_dcud_error_ioctl(3);
                break;
            case TEST_DCUD_TWICE_SAME_CTX_DEVNO:
                rc = test_dcud_error_ioctl(4);
                break;
            case TEST_DCUD_VLUN_ALREADY_CREATED_SAME_DISK:
                rc = test_dcud_error_ioctl(5);
                break;
            case TEST_DCUD_PLUN_ALREADY_CREATED_SAME_DISK:
                rc = test_dcud_error_ioctl(6);
                break;
            case TEST_DCUD_VLUN_CREATED_DESTROYED_SAME_DISK:
                rc = test_dcud_ioctl(2);
                break;
            case TEST_DCUD_IN_LOOP:
                rc = test_dcud_ioctl(3);
                break;
            case TEST_DCUD_PATH_ID_MASK_VALUES:
                rc = test_dcud_ioctl(4);
                break;
            case TEST_DCUD_BAD_PATH_ID_MASK_VALUES:
                rc = test_dcud_error_ioctl(7);
                break;
                /***  DK_CAPI_USER_VIRTUAL ***/
            case TEST_DCUV_INVALID_DEVNO_VALID_CTX:
                rc = test_dcuv_error_ioctl(1);
                break;
            case TEST_DCUV_INVALID_CTX_INVALID_DEVNO:
                rc = test_dcuv_error_ioctl(2);
                break;
            case TEST_DCUV_VALID_DEVNO_INVALID_CTX:
                rc = test_dcuv_error_ioctl(3);
                break;
            case TEST_DCUV_LUN_VLUN_SIZE_ZERO:
                rc = test_dcuv_ioctl(1);
                break;
            case TEST_DCUV_PLUN_ALREADY_CREATED_SAME_DISK:
                rc = test_dcuv_error_ioctl(4);
                break;
            case TEST_DCUV_VLUN_ALREADY_CREATED_SAME_DISK:
                rc = test_dcuv_ioctl(2);
                break;
            case TEST_DCUV_NO_FURTHER_VLUN_CAPACITY:
                rc = test_dcuv_error_ioctl(5);
                break;
            case TEST_DCUV_MTPLE_VLUNS_SAME_CAPACITY_SAME_DISK:
                rc = test_dcuv_ioctl(3);
                break;
            case TEST_DCUV_TWICE_SAME_CTX_DEVNO:
                rc = test_dcuv_ioctl(4);
                break;
            case TEST_DCUV_VLUN_MAX:
                rc = test_dcuv_ioctl(5);
                break;
            case TEST_DCUV_VLUN_SIZE_MORE_THAN_DISK_SIZE:
                rc = test_dcuv_error_ioctl(6);
                break;
            case TEST_DCUV_PLUN_CREATED_DESTROYED_SAME_DISK:
                rc = test_dcuv_ioctl(6);
                break;
            case TEST_DCUV_WITH_CTX_OF_PLUN:
                rc = test_dcuv_error_ioctl(7);
                break;
            case TEST_DCUD_WITH_CTX_OF_VLUN:
                rc = test_dcuv_error_ioctl(8);
                break;
            case TEST_DCUV_PATH_ID_MASK_VALUES:
                rc = test_dcuv_ioctl(7);
                break;
            case TEST_DCUV_INVALID_PATH_ID_MASK_VALUES:
                rc = test_dcuv_ioctl(8);
                break;
            case TEST_DCUV_IN_LOOP:
                rc = test_dcuv_ioctl(9);
                break;
                /*** DK_CAPI_DETACH ***/
            case TEST_DCD_INVALID_CTX_DEVNO:
                rc = test_dcd_ioctl(1);
                break;
            case TEST_DCD_INVALID_DEVNO:
                rc = test_dcd_ioctl(2);
                break;
            case TEST_DCD_INVALID_CTX:
                rc = test_dcd_ioctl(3);
                break;
            case TEST_DCD_TWICE_ON_PLUN:
                rc = test_dcd_ioctl(4);
                break;
            case TEST_DCD_TWICE_ON_VLUN:
                rc = test_dcd_ioctl(5);
                break;
                /*** DK_CAPI_VERIFY ****/
            case TEST_DCV_INVALID_DEVNO:
                rc = test_dcv_error_ioctl(1);
                break;
            case TEST_DCV_INVALID_FLAGS:
                rc = test_dcv_error_ioctl(2);
                break;
            case TEST_DCV_INVALID_RES_HANDLE:
                rc = test_dcv_error_ioctl(3);
                break;
            case TEST_DCV_UNEXPECTED_ERR:
                rc = test_dcv_ioctl(1);
                break;
            case TEST_DCV_NO_ERR:
                rc = test_dcv_ioctl(2);
                break;
            case TEST_DCV_UNEXPECTED_ERR_VLUN:
                rc = test_dcv_ioctl(3);
                break;
            case TEST_DCV_VLUN_RST_FlAG:
                rc = test_dcv_ioctl(4);
                break;
            case TEST_DCV_VLUN_TUR_FLAG:
                rc = test_dcv_ioctl(5);
                break;
            case TEST_DCV_VLUN_INQ_FLAG:
                rc = test_dcv_ioctl(6);
                break;
            case TEST_DCV_VLUN_HINT_SENSE:
                rc = test_dcv_ioctl(7);
                break;
            case TEST_DCV_PLUN_RST_FlAG:
                rc = test_dcv_ioctl(8);
                break;
            case TEST_DCV_PLUN_TUR_FLAG:
                rc = test_dcv_ioctl(9);
                break;
            case TEST_DCV_PLUN_INQ_FLAG:
                rc = test_dcv_ioctl(10);
                break;
            case TEST_DCV_PLUN_HINT_SENSE:
                rc = test_dcv_ioctl(11);
                break;
            case TEST_DCV_PLUN_RST_FlAG_EEH:
                rc = test_dcv_ioctl(12);
                break;
                /*** DK_CAPI_VLUN_RESIZE ***/
            case TEST_DCVR_INVALID_DEVNO:
                rc = test_dcvr_error_ioctl(1);
                break;
            case TEST_DCVR_INVALID_CTX_DEVNO:
                rc = test_dcvr_error_ioctl(2);
                break;
            case TEST_DCVR_INVALID_CTX:
                rc = test_dcvr_error_ioctl(3);
                break;
            case TEST_DCVR_NO_VLUN:
                rc = test_dcvr_error_ioctl(4);
                break;
            case TEST_DCVR_ON_PLUN:
                rc = test_dcvr_error_ioctl(5);
                break;
            case TEST_DCVR_GT_DISK_SIZE:
                rc = test_dcvr_error_ioctl(6);
                break;
            case TEST_DCVR_NOT_FCT_256MB:
                rc = test_dcvr_ioctl(1);
                break;
            case TEST_DCVR_EQ_CT_VLUN_SIZE:
                rc = test_dcvr_ioctl(2);
                break;
            case TEST_DCVR_LT_CT_VLUN_SIZE:
                rc = test_dcvr_ioctl(3);
                break;
            case TEST_DCVR_GT_CT_VLUN_SIZE:
                rc = test_dcvr_ioctl(4);
                break;
            case TEST_DCVR_EQ_DISK_SIZE_NONE_VLUN:
                rc = test_dcvr_ioctl(5);
                break;
            case TEST_DCVR_EQ_DISK_SIZE_OTHER_VLUN:
                rc = test_dcvr_error_ioctl(7);
                break;
            case TEST_DCVR_INC_256MB:
                rc = test_dcvr_ioctl(6);
                break;
            case TEST_DCVR_DEC_256MB:
                rc = test_dcvr_ioctl(7);
                break;
            case TEST_DCVR_GT_CT_VLUN_LT_256MB:
                rc = test_dcvr_ioctl(8);
                break;
            case TEST_DCVR_LT_CT_VLUN_LT_256MB:
                rc = test_dcvr_ioctl(9);
                break;
            case TEST_DCVR_INC_DEC_LOOP:
                rc = test_dcvr_ioctl(10);
                break;
            case G_MC_test_DCVR_ZERO_Vlun_size:
                rc = test_dcvr_ioctl(11);
                break;
                /*** DK_CAPI_RELEASE ***/
            case TEST_DCR_INVALID_DEVNO:
                rc = test_dcr_ioctl(1);
                break;
            case TEST_DCR_INVALID_DEVNO_CTX:
                rc = test_dcr_ioctl(2);
                break;
            case TEST_DCR_INVALID_CTX:
                rc = test_dcr_ioctl(3);
                break;
            case TEST_DCR_NO_VLUN:
                rc = test_dcr_ioctl(4);
                break;
            case TEST_DCR_PLUN_AGIAN:
                rc = test_dcr_ioctl(5);
                break;
            case TEST_DCR_VLUN_AGIAN:
                rc = test_dcr_ioctl(6);
                break;
            case TEST_DCR_MULTP_VLUN:
                rc = test_dcr_ioctl(7);
                break;
            case TEST_DCR_VLUN_INV_REL:
                rc = test_dcr_ioctl(8);
                break;
                /*** DK_CAPI_LOG_EVENT ***/
#ifdef _AIX
            case TEST_DCLE_VALID_VALUES:
                rc = test_dcle_ioctl(1);
                break;
            case TEST_DCLE_DK_LF_TEMP:
                rc = test_dcle_ioctl(2);
                break;
            case TEST_DCLE_DK_LF_PERM:
                rc = test_dcle_ioctl(3);
                break;
            case TEST_DCLE_DK_FL_HW_ERR:
                rc = test_dcle_ioctl(4);
                break;
            case TEST_DCLE_DK_FL_SW_ERR:
                rc = test_dcle_ioctl(5);
                break;
#endif
            case TEST_SPIO_VLUN:
                rc = test_spio_vlun(2);
                break;
            case TEST_SPIO_0_VLUN:
                rc = test_spio_vlun(1);
                break;
            case TEST_SPIO_NORES_AFURC:
                rc = test_spio_vlun(3);
                break;
            case TEST_SPIO_A_PLUN:
                rc = test_spio_plun();
                break;
            case TEST_SPIO_ALL_PLUN:
                rc = test_spio_pluns(2);
                break;
            case TEST_SPIO_ALL_VLUN:
                rc = test_spio_vluns(1);
                break;
            case TEST_SPIO_VLUN_PLUN:
                rc = test_spio_direct_virtual();
                break;
            case TEST_MC_SIZE_REGRESS:
                rc = mc_test_chunk_regress(4);
                break;
            case TEST_MC_REGRESS_CTX_CRT_DSTR:
                rc = test_mc_regress_ctx_crt_dstr(1);
                break;
            case TEST_MC_REGRESS_CTX_CRT_DSTR_IO:
                rc = test_mc_regress_ctx_crt_dstr(2);
                break;
            case TEST_MC_REGRESS_RESOURCE:
                rc = mc_test_chunk_regress_long();
                break;
            case TEST_MC_TWO_CTX_RD_WRTHRD:
                rc = test_two_ctx_two_thrd(1);
                break;

            case TEST_MC_TWO_CTX_RDWR_SIZE:
                rc = test_two_ctx_two_thrd(2);
                break;

            case TEST_MC_ONE_CTX_TWO_THRD:
                rc = test_onectx_twothrd(1);
                break;

            case TEST_MC_ONE_CTX_RD_WRSIZE:
                rc = test_onectx_twothrd(2);
                break;

            case TEST_MC_MAX_RES_HNDL:
                rc = mc_max_vdisk_thread();
                break;

            case TEST_ONE_UNIT_SIZE:
                rc = test_one_aun_size();
                break;

            case TEST_MAX_CTX_RES_UNIT:
                rc = max_ctx_max_res(1);
                break;

            case TEST_MAX_CTX_RES_LUN_CAP:
                rc = max_ctx_max_res(2);
                break;

            case TEST_MC_MAX_SIZE:
                rc = test_mc_max_size();
                break;

            case TEST_MC_SPIO_VLUN_ATCH_DTCH:
                rc = test_spio_attach_detach(1);
                break;

            case TEST_MC_SPIO_PLUN_ATCH_DTCH:
                rc = test_spio_attach_detach(2);
                break;

            case MC_TEST_RWBUFF_GLOBAL:
                rc = mc_test_rwbuff_global(1);
                break;

            case MC_TEST_RWBUFF_HEAP:
                rc = mc_test_rwbuff_global(2);
                break;

            case MC_TEST_RWBUFF_SHM:
                rc = test_mc_rwbuff_shm();
                break;

            case MC_TEST_RW_SIZE_PARALLEL:
                rc = test_mc_rw_size_parallel();
                break;

            case MC_TEST_GOOD_ERR_AFU_DEV:
                rc = test_mc_good_error_afu_dev();
                break;

            case TEST_MC_RW_CLS_RSH:
                rc = test_rw_close_hndl(1);
                break;

            case TEST_MC_RW_CLOSE_DISK_FD:
                rc = test_rw_close_hndl(2);
                break;

            case TEST_MC_RW_CLOSE_CTX:
                rc = test_rw_close_hndl(3);
                break;

            case TEST_MC_RW_UNMAP_MMIO:
                rc = test_rw_close_hndl(4);
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

            case TEST_LARGE_TRANSFER_IO:
                rc = test_large_transfer();
                break;

            case TEST_LARGE_TRNSFR_BOUNDARY:
                rc = test_large_trnsfr_boundary();
                break;

            case MAX_CTX_RCVR_EXCEPT_LAST_ONE:
                rc = max_ctx_rcvr_except_last_one();
                break;

            case MAX_CTX_RCVR_LAST_ONE_NO_RCVR:
                rc = max_ctx_rcvr_last_one_no_rcvr();
                break;

            case TEST_VSPIO_EEHRECOVERY:
                rc = test_vSpio_eehRecovery(1);
                break;

            case TEST_DSPIO_EEHRECOVERY:
                rc = test_dSpio_eehRecovery(1);
                break;

            case TEST_IOCTL_FCP:
                rc = test_ioctl_fcp();
                break;

            case TEST_MMIO_ERRCASE1:
                rc = test_mmio_errcase(TEST_MMIO_ERRCASE1);
                break;
            case TEST_MMIO_ERRCASE2:
                rc = test_mmio_errcase(TEST_MMIO_ERRCASE2);
                break;
            case TEST_MMIO_ERRCASE3:
                rc = test_mmio_errcase(TEST_MMIO_ERRCASE3);
                break;

            case TEST_SPIO_KILLPROCESS:
                rc = test_spio_killprocess();
                break;

            case TEST_SPIO_EXIT:
                rc = test_spio_exit();
                break;

            case TEST_IOCTL_SPIO_ERRCASE:
                rc = test_ioctl_spio_errcase();
                break;
            case TEST_FC_PR_RESET_VLUN:
                rc = test_fc_port_reset_vlun();
                break;
            case TEST_FC_PR_RESET_PLUN:
                rc = test_fc_port_reset_plun();
                break;
            case TEST_CFDISK_CTXS_DIFF_DEVNO:
                rc = test_cfdisk_ctxs_diff_devno();
                break;

            case TEST_ATTACH_REUSE_DIFF_PROC:
                rc = test_attach_reuse_diff_proc();
                break;

            case TEST_DETACH_DIFF_PROC:
                rc = test_detach_diff_proc();
                break;
            case EXCP_VLUN_DISABLE:
                rc = test_dcqexp_ioctl(EXCP_VLUN_DISABLE);
                break;
            case EXCP_PLUN_DISABLE:
                rc = test_dcqexp_ioctl(EXCP_PLUN_DISABLE);
                break;
            case EXCP_VLUN_VERIFY:
                rc = test_dcqexp_ioctl(EXCP_VLUN_VERIFY);
                break;
            case EXCP_DISK_INCREASE:
                rc = test_dcqexp_ioctl(EXCP_DISK_INCREASE);
                break;
            case EXCP_PLUN_VERIFY:
                rc = test_dcqexp_ioctl(EXCP_PLUN_VERIFY);
                break;
            case EXCP_VLUN_INCREASE:
                rc = test_dcqexp_ioctl(EXCP_VLUN_INCREASE);
                break;
            case EXCP_VLUN_REDUCE :
                rc = test_dcqexp_ioctl(EXCP_VLUN_REDUCE);
                break;
            case EXCP_PLUN_UATTENTION :
                rc = test_dcqexp_ioctl(EXCP_PLUN_UATTENTION);
                break;
            case EXCP_VLUN_UATTENTION :
                rc = test_dcqexp_ioctl(EXCP_VLUN_UATTENTION);
                break;
            case EXCP_EEH_SIMULATION :
                rc = test_dcqexp_ioctl(EXCP_EEH_SIMULATION);
                break;
            case EXCP_INVAL_DEVNO :
                rc = test_dcqexp_invalid(EXCP_INVAL_DEVNO);
                break;
            case EXCP_INVAL_CTXTKN :
                rc = test_dcqexp_invalid(EXCP_INVAL_CTXTKN);
                break;
            case EXCP_INVAL_RSCHNDL :
                rc = test_dcqexp_invalid(EXCP_INVAL_RSCHNDL);
                break;
            case TEST_DK_CAPI_CLONE :
                rc = test_clone_ioctl(0);
                break;
            case G_ioctl_7_1_119:
                rc=ioctl_7_1_119_120(119);
                break;
            case E_ioctl_7_1_120:
                rc=ioctl_7_1_119_120(120);
                break;
            case E_ioctl_7_1_174:
                rc=ioctl_7_1_174_175(1);
                break;
            case E_ioctl_7_1_175:
                rc=ioctl_7_1_174_175(2);
                break;
#ifdef _AIX
            case E_ioctl_7_1_180:
                rc=test_traditional_IO(180,1);
                break;
            case E_ioctl_7_1_1801:
                rc=test_traditional_IO(1801,1);
                break;
            case G_ioctl_7_1_181:
                rc=test_traditional_IO(181,1);
                break;
            case E_ioctl_7_1_182:
                rc=test_traditional_IO(182,1);
                break;
            case G_ioctl_7_1_187:
                rc=test_traditional_IO(187,1);
                break;
#endif
            case G_ioctl_7_1_188:
#ifdef _AIX
                rc=test_vSpio_eehRecovery(1);
#else
                rc=ioctl_7_1_188(188);
#endif
                break;
            case G_ioctl_7_1_189:
#ifdef _AIX
                rc = test_dSpio_eehRecovery(1);
#else
                rc=ioctl_7_1_188(189);
#endif
                break;
            case E_CAPI_LINK_DOWN :
                rc=ioctl_7_1_188(E_CAPI_LINK_DOWN);
                break;
            case E_ioctl_7_1_190:
                rc=ioctl_7_1_190();
                break;
            case G_ioctl_7_1_191:
                rc=ioctl_7_1_191(191);
                break;
            case G_ioctl_7_1_192:
                rc=ioctl_7_1_192(192);
                break;
#ifdef _AIX
            case G_ioctl_7_1_193_1:
                rc=ioctl_7_1_193_1(193);
                break;
            case G_ioctl_7_1_193_2:
                rc=ioctl_7_1_193_2(194);
                break;
#endif
            case G_ioctl_7_1_196:
                rc=ioctl_7_1_196();
                break;
            case E_ioctl_7_1_197:
                rc=ioctl_7_1_197(197);
                break;
            case E_ioctl_7_1_198:
                rc=ioctl_7_1_198(198);
                break;
            case G_ioctl_7_1_203:
                rc=ioctl_7_1_203(203);
                break;
            case E_ioctl_7_1_209:
                rc=ioctl_7_1_209(209);
                break;
            case E_ioctl_7_1_210:
                rc=ioctl_7_1_210(210);
                break;
            case E_ioctl_7_1_211:
                rc=ioctl_7_1_211(211);
                break;
#ifdef _AIX
            case E_ioctl_7_1_212:
                rc=test_traditional_IO(212,1);
                break;
            case E_ioctl_7_1_213:
                rc=test_traditional_IO(213,1);
                break;
#endif
            case E_ioctl_7_1_214:
                rc=test_traditional_IO(214,1);
                break;

            case E_ioctl_7_1_215:
                rc= test_dca_error_ioctl(13);
                break;
            case E_ioctl_7_1_216:
                rc=test_dca_error_ioctl(14);
                break;

            case E_test_SCSI_CMDS:
                rc = test_scsi_cmds();
                break;

            case E_TEST_CTX_RESET:
                rc = test_ctx_reset();
                break;

            case M_TEST_7_5_13_1:
                rc = ioctl_7_5_13(1);
                break;

            case M_TEST_7_5_13_2:
                rc = ioctl_7_5_13(2);
                break;
            case G_TEST_MAX_CTX_PLUN:
                rc = max_ctx_on_plun(3);
                break;

            case G_TEST_MAX_CTX_0_VLUN:
                rc = max_ctx_on_plun(2);
                break;

            case G_TEST_MAX_CTX_ONLY:
                rc = max_ctx_on_plun(1);
                break;

            case G_TEST_MAX_CTX_IO_NOFLG:
                rc = max_ctx_on_plun(4);
                break;

            case G_TEST_MAX_VLUNS:
                rc = max_vlun_on_a_ctx();
                break;

            case E_TEST_MAX_CTX_CRSS_LIMIT:
                rc = max_ctx_cross_limit();
                break;

            default:
                rc = -1;
                break;
        }
        exit(rc);
    }
    wait(&rc);
    if (WIFEXITED(rc))
    {
        rc = WEXITSTATUS(rc);
    }

    if (DEBUG)
    {
        system("echo;echo ---------------------- Test End Timestamp ----------------------");
        system("date");
        system("echo ----------------------------------------------------------------; echo");
    }

    return rc;
}
