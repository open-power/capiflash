/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/test/fvt_block.C $                                  */
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
/******************************************************************************
 * \file
 * \brief
 * \ingroup
 ******************************************************************************/

#include <gtest/gtest.h>

extern "C"
{
#include "blk_tst.h"
int         num_opens = 0;
uint32_t    thread_count = 0;
uint64_t    block_number;
uint64_t    max_xfer = 0;
int         num_loops;
int         thread_flag;
int         num_threads;
int         virt_lun_flags=0;
int         share_cntxt_flags=0;
int         io_bufcnt = 1;
int	    num_listio = 500;
int	    test_max_cntx = 0;
chunk_id_t  chunks[MAX_OPENS+15];
void        *blk_fvt_data_buf = NULL;
void        *blk_fvt_comp_data_buf = NULL;
char        *env_filemode = getenv("BLOCK_FILEMODE_ENABLED");
char        *env_max_xfer = getenv("CFLSH_BLK_MAX_XFER");
char        *env_num_cntx = getenv("MAX_CNTX");
}

int mode = O_RDWR;

TEST(Block_FVT_Suite, BLK_API_FVT_FM_open_phys_lun)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs    = 64;
    int         er_no   = 0;
    int         open_cnt = 1;

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );

    blk_open_tst_cleanup();


}
TEST(Block_FVT_Suite, BLK_API_FVT_open_phys_lun_RDONLY_mode_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd;

    mode = O_RDONLY;

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );


    // Try to write , it should fail
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    // Try to read it should pass
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  

    blk_open_tst_cleanup();

}

TEST(Block_FVT_Suite, BLK_API_FVT_open_phys_lun_WRONLY_mode_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd;

    mode = O_WRONLY;

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );

    // Try to read , it should fail
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    // Try to write it should pass
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  

    blk_open_tst_cleanup();

}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_open_phys_lun_RDWR_mode_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd;

    mode = O_RDWR;

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );

    // Try to write , it should pass
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    // Try to read it should pass
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  

    blk_open_tst_cleanup();

}

TEST(Block_FVT_Suite, BLK_API_FVT_open_virt_lun_RDONLY_mode_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;

    mode = O_RDONLY;

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );


    temp_sz = 1;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    // Try to write , it should fail
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    // Try to read it should pass
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  

    blk_open_tst_cleanup();

}

TEST(Block_FVT_Suite, BLK_API_FVT_open_virt_lun_WRONLY_mode_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;

    mode = O_WRONLY;

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );


    temp_sz = 1;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    // Try to read , it should fail
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    // Try to write it should pass
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  

    blk_open_tst_cleanup();

}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_open_virt_lun_RDWR_mode_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;

    mode = O_RDWR;

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );


    temp_sz = 1;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    // Try to write , it should pass
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    // Try to read it should pass
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  

    blk_open_tst_cleanup();

}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_open_virt_lun)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs    = 64;
    int         er_no   = 0;
    int         open_cnt = 1;

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );

    blk_open_tst_cleanup();
}
/** Need to check max_requests no, if valid */

TEST(Block_FVT_Suite, BLK_API_FVT_FM_open_phys_lun_exceed_max_reqs)
{
    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs    = 8192;
    int         er_no   = 0;
    int         open_cnt = 1;


    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    // expect success

    EXPECT_NE(NULL_CHUNK_ID,  id );
    EXPECT_EQ(0,  er_no );

    if (id == NULL_CHUNK_ID) {
        blk_open_tst_cleanup();
        return;
    }

    max_reqs   = (8192 +1);
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    // expect failure

    EXPECT_EQ(NULL_CHUNK_ID,  id );

    EXPECT_EQ(12,  er_no );

    blk_open_tst_cleanup();

}


TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_open_virt_lun_exceed_max_reqs)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs   = 8192;
    int         er_no   = 0;
    int         open_cnt = 1;


    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    // expect success

    EXPECT_NE(NULL_CHUNK_ID,  id );
    EXPECT_EQ(0,  er_no );

    if (id == NULL_CHUNK_ID) {
        blk_open_tst_cleanup();
        return;
    }

    max_reqs   = (8192 +1);
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    // expect failure

    EXPECT_EQ(NULL_CHUNK_ID,  id );

    EXPECT_EQ(12,  er_no );

    blk_open_tst_cleanup();

}

// verify using physical lun, max_contex 508 succeeds 

TEST(Block_FVT_Suite, BLK_API_FVT_UMC_Max_context_tst)
{

    int         open_flags   = 0;
    int         max_reqs    = 64;
    int         er_no   = 0;
    int         ret   = 0;

    int         max_cntxt = 508;

#ifdef _AIX
    max_cntxt = 494; 
#endif


    ASSERT_EQ(0,blk_fvt_setup(1));

    max_context(&ret, &er_no, max_reqs, max_cntxt, open_flags,mode);

    EXPECT_EQ(0,  ret );
    EXPECT_EQ(0,  er_no );

    blk_open_tst_cleanup ();

}

// verify using physical lun, max_contex 509 (for AIX 495) exceeds max , should fail 


TEST(Block_FVT_Suite, BLK_API_FVT_UMC_Exceed_Max_context_tst)
{

    int         open_flags   = 0;
    int         max_reqs    = 64;
    int         er_no   = 0;
    int         ret   = 0;

    int         max_cntxt = 509;
#ifdef _AIX
    max_cntxt = 496; /* Should fail on 495 */
#endif


    ASSERT_EQ(0,blk_fvt_setup(1));

    if (test_max_cntx ) {
            max_cntxt = test_max_cntx;
    }

    max_context(&ret, &er_no, max_reqs, max_cntxt, open_flags,mode);

    EXPECT_NE(0,  ret );
    EXPECT_NE(0,  er_no );

    blk_open_tst_cleanup ();

}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_open_invalid_dev_path)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs    = 64;
    const char  *dev_path   = "junk";
    int         er_no   = 0;
    int         open_cnt = 1;



    ASSERT_EQ(0,blk_fvt_setup(1));

    
    blk_open_tst_inv_path(dev_path, &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    EXPECT_EQ(NULL_CHUNK_ID,  id);
    EXPECT_NE(0, er_no);

    blk_open_tst_cleanup();

}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_open_close_phys_lun)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs    = 64;
    int         er_no   = 0;
    int         open_cnt = 1;
    int         close_flag = 0;
    int         ret = 0;
    

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    blk_close_tst(id, &ret, &er_no, close_flag);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_open_close_virt_lun)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs    = 64;
    int         er_no   = 0;
    int         open_cnt = 1;
    int         close_flag = 0;
    int         ret = 0;
    

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    blk_close_tst(id, &ret, &er_no, close_flag);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
}



TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_close_unopen_phys_lun)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs    = 64;
    int         er_no   = 0;
    int         open_cnt = 1;
    int         close_flag = 0;
    int         ret = 0;
    

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    blk_close_tst(id, &ret, &er_no, close_flag);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

// Close un-opened lun
    
    blk_close_tst(id, &ret, &er_no, close_flag);

    EXPECT_EQ(EINVAL , er_no);
    EXPECT_EQ(-1 , ret);


}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_close_unopen_virt_lun)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs    = 64;
    int         er_no   = 0;
    int         open_cnt = 1;
    int         close_flag = 0;
    int         ret = 0;
    

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    blk_close_tst(id, &ret, &er_no, close_flag);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_close_tst(id, &ret, &er_no, close_flag);

    EXPECT_EQ(-1 , ret);
    EXPECT_EQ(EINVAL , er_no);

}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_get_lun_size_physical)
{
    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      phys_lun_sz = 0;
    int         get_set_size_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size
    size_t      ret_size = 0;


    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Get phys lun size 
   
    blk_fvt_get_set_lun_size(id, &phys_lun_sz, sz_flags, get_set_size_flag, &ret, &er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_NE((size_t)0, phys_lun_sz);

// get size on phys lun should report whole phys lun size
    get_set_size_flag = 1; 
    blk_fvt_get_set_lun_size(id, &ret_size, sz_flags, get_set_size_flag, &ret, &er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

// test chunk size should be  equal whole phys lun size
    EXPECT_EQ(phys_lun_sz , ret_size);

    blk_open_tst_cleanup();

}
TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_get_lun_siz_virtual)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      lun_sz = 0;
    int         get_set_size_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size
    
    // open virtual lun
    

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Get virtual  lun size , it should fail, since it is not set
   
    get_set_size_flag = 1; 

    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_size_flag, &ret, &er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ((size_t)0, lun_sz);

    blk_open_tst_cleanup();

}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_set_size_physical)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      lun_sz = 0;
    int         get_set_size_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open phys lun
    
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Try to set size on phy lun , it should fail, since it can not be change
   
    get_set_size_flag = 2; 
    lun_sz = 4096;

    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_size_flag, &ret, &er_no);

    EXPECT_EQ(-1 , ret);
    EXPECT_EQ(EINVAL , er_no);

    blk_open_tst_cleanup();

}


TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_set_size_virt_lun)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size
    size_t      chunk_sz = 0;

    size_t      temp_sz = 0;


    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual lun
    
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Try to set size on virt lun e
   
    get_set_size_flag = 2; 
    chunk_sz = 4096;

    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_size_flag, &ret, &er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    // Try to get size on virt lun and verify the set size
   
    get_set_size_flag = 1; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ(chunk_sz, temp_sz);

    blk_open_tst_cleanup();

}

// open virt, get lun size, set size > lunsize, should fail

TEST(Block_FVT_Suite, BLK_API_FVT_FM_set_invalid_chunk_size)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set chunk size

    size_t      lun_sz = 0;
    size_t      temp_sz = 0;


    ASSERT_EQ(0,blk_fvt_setup(1));
    
    // open phys lun

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_size_flag = 1;  // get phy lun size as 1 chunk

    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_size_flag, &ret, &er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cblk_close (id, 0);
    num_opens --;

    // open virtual lun
   
    open_flags   = CBLK_OPN_VIRT_LUN; 

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Set size > lun size
   
    temp_sz = lun_sz + 1;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);


    EXPECT_EQ(-1 , ret);
    EXPECT_EQ(EINVAL ,er_no);

    blk_open_tst_cleanup();

}

// TODO current block layer code donot support this functions

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_increase_decrease_chunk_size)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz = 0;
    size_t      chunk_sz = 0;


    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual lun
    
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Get virt lun size , it sould be 0
   
    get_set_size_flag = 1; 

    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_size_flag, &ret, &er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ((size_t)0, chunk_sz);

    // set chunk size (100)
   
    temp_sz = chunk_sz + 100;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    
    // Get the chunk size and Verify it set correctly
   
    get_set_size_flag = 1; 
    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_size_flag, &ret, &er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ(temp_sz, chunk_sz );

    // Increase chunk sz by 20 blocks 
    // set size ()
   
    temp_sz += 20;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    
    // Get size and Verify it is increased correctly to 220
    chunk_sz = 0; 
    get_set_size_flag = 1; 
    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_size_flag, &ret, &er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ(temp_sz, chunk_sz);

    // decrease chunk sz by 10 blocks 
    // set size ()
   
    temp_sz = (chunk_sz - 10);
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    // Get the size and Verify it is decreased correctly to 210
    chunk_sz = 0; 
    get_set_size_flag = 1; 
    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_size_flag, &ret, &er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ(temp_sz, chunk_sz);


    blk_open_tst_cleanup();

}


TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_increase_size_preserve_data)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      temp_sz,nblks;
    uint64_t    lba;
    int         cmd; // 0 = read, 1, write                                     
    int         io_flags = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    if ((env_filemode) &&  (atoi(env_filemode) == 1)) {
        /*
         * This test will not work with filemode and no MC.
         */
        return;
    }

    ASSERT_EQ(0,blk_fvt_setup(1));


    // open virtual lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    // write on block 0
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    // Increase virt lun size 1 additional block
    temp_sz = 2;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);


    // Read block 0 and see if it is protected
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);

    // Compare buffs, it should not be overwritten
    blk_fvt_cmp_buf (nblks, &ret);
    EXPECT_EQ(0,ret);

    blk_open_tst_cleanup();
 
}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_get_chunk_status)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      chunk_sz = 0; 
    int         cmd; // 0 = read, 1, write                                     
    int         get_set_size_flag = 0; 

                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

   // open Virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Try to set size on virt lun e

    get_set_size_flag = 2;
    chunk_sz = 4096;

    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_READ;

    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
    blk_get_statistics(id, open_flags, &ret, &er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();
}


// Test read succeed on phy lun, prior to set size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_read_phys_lun_wo_setsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags = 0;
    
    int         cmd; // 0 = read, 1, write                                     

                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

   // open Phys lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    cmd = FV_READ;

    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    // expect fail ret code 

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();


}

// Test read fails on virt lun, prior to set size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_read_virt_lun_wo_setsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags = 0;
                                        
    int         cmd; // 0 = read, 1, write                                     


    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virual lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    cmd = FV_READ;
    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    blk_open_tst_cleanup();

}


// Test read fails on read outside of lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_read_vir_lun_outside_lunsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd; // 0 = read, 1, write                                     


    ASSERT_EQ(0,blk_fvt_setup(1));

   // open virual lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1;
    lba = 0;
    get_set_size_flag = 2; 

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_READ;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    lba = 1;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
  
    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);

    blk_open_tst_cleanup();

}

// Test read fails on read outside of phys lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_read_phys_lun_outside_lunsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    size_t      temp_sz,nblks;
    int         io_flags = 0;
    int         cmd; // 0 = read, 1, write                                     
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
     

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 0;
    get_set_size_flag = 1; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_READ;

    lba = temp_sz+1;
    nblks = 1;

    // verify read outside lun size fails

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    blk_open_tst_cleanup();
}
// Verify read fails when tried to read more than 1 block size

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_read_greater_than_1_blk)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    size_t      temp_sz, nblks;
    int         cmd; // 0 = read, 1, write                                     
    int         io_flags = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    

    ASSERT_EQ(0,blk_fvt_setup(2));

   // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_READ;

    lba = 0;
    nblks = 2;

    // Verify read fails when block size greater than 1

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_NE(2 , ret);
    EXPECT_EQ(22 , er_no);
  
    blk_open_tst_cleanup();

}

// write data , read data and then compare

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_read_write_compare)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;



    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_WRITE;
    lba = 1;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    cmd = FV_READ;
    lba = 1;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    // compare buffers
    
    blk_fvt_cmp_buf(nblks, &ret);

    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup();
}
// Run I/O with xfersize 1M thru 16M

TEST(Block_FVT_Suite, BLK_API_FVT_FM_1M_thru_16M_xfersize_physical)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int        ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks = 0;
    size_t      lun_sz = 0;
    size_t      xfersz = 0;
    int         cmd;
    size_t      i = 0;


    if ((env_filemode) &&  (atoi(env_filemode) == 1)) {
        if (env_max_xfer && (atoi(env_max_xfer)))
            nblks = atoi(env_max_xfer);
        else {
            /*
            * This test will not work  filemode and no CFLSH_BLK_MAX_XFER set.
            */
            TESTCASE_SKIP("env CFLSH_BLK_MAX_XFER is _not_ set");
            return;

        }
    } else {
            nblks = 4096; /* 256 = 1 M, 4096 = 16M */
    }


    ASSERT_EQ(0,blk_fvt_setup(nblks));

    // open physical  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_size_flag = 0; 
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    if (lun_sz < nblks)
        nblks = lun_sz;

    for (i = 256; i < nblks ; i+= 256) {
        xfersz = i;
        cmd = FV_WRITE;
        lba = 0;

        blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags); 
    
        EXPECT_EQ(xfersz, (size_t)ret);
        if ((int)xfersz != ret) {
           fprintf(stderr,"Write failed xfersz 0x%lx\n",i);
           blk_open_tst_cleanup();
           return;
        }

        cmd = FV_READ;
        lba = 0;
        // read what was wrote  xfersize
        blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(xfersz, (size_t)ret);
        if ((int)xfersz != ret) {
           fprintf(stderr,"Read failed xfersz 0x%lx\n",i);
           blk_open_tst_cleanup();
           return;
        }
        // compare buffers
    
        blk_fvt_cmp_buf(xfersz, &ret);
        if (ret != 0) {
           fprintf(stderr,"Compare failed xfersz 0x%lx\n",i);
           blk_open_tst_cleanup();
        }
        ASSERT_EQ(0, ret);
    }

    blk_open_tst_cleanup();
}

// Run I/O with 1M xfersize

TEST(Block_FVT_Suite, BLK_API_FVT_FM_1M_xfersize_physical)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int        ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks = 0;
    size_t      lun_sz = 0;
    size_t      xfersz = 0;
    int         cmd;
    if ((env_filemode) &&  (atoi(env_filemode) == 1)) {
        if (env_max_xfer && (atoi(env_max_xfer)))
            nblks = atoi(env_max_xfer);
        else {
            /*
            * This test will not work  filemode and no CFLSH_BLK_MAX_XFER set.
            */
            TESTCASE_SKIP("env CFLSH_BLK_MAX_XFER is _not_ set");
            return;

        }
    } else {
            nblks = 256; /* 256 = 1 M, 4096 = 16M */
    }


    ASSERT_EQ(0,blk_fvt_setup(nblks));

    // open physical  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_size_flag = 0; 
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    if (lun_sz < nblks)
        nblks = lun_sz;

    xfersz = nblks;
    cmd = FV_WRITE;
    lba = 1;

    blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags); 
    
    EXPECT_EQ(xfersz, (size_t)ret);
    if ((int)xfersz != ret) {
           fprintf(stderr,"Write failed 1M xfersz \n");
           blk_open_tst_cleanup();
           return;
    }

    cmd = FV_READ;
    lba = 1;
    // read what was wrote  xfersize
    blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(xfersz, (size_t)ret);
    if ((int)xfersz != ret) {
           fprintf(stderr,"Read failed 1M xfersz \n");
           blk_open_tst_cleanup();
           return;
    }
    // compare buffers
    
    blk_fvt_cmp_buf(xfersz, &ret);
    if (ret != 0) {
           fprintf(stderr,"Compare failed 1M xfersz \n");
           blk_open_tst_cleanup();
    }
    ASSERT_EQ(0, ret);

    blk_open_tst_cleanup();
}

// Run I/O with variable xfersize , use env_max_xfer if set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_phys_lun_large_xfer_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int        ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks = 0;
    size_t      lun_sz = 0;
    size_t      xfersz = 0;
    int         cmd;


    // Use user supplied max xfer size, if provided

    if (env_max_xfer && (atoi(env_max_xfer)))
         nblks = atoi(env_max_xfer);

    // filemode test must have set env_max_xfer variable
    if ((env_filemode) &&  (atoi(env_filemode) == 1) && !nblks) {
            /*
            * This test will not work  filemode and no CFLSH_BLK_MAX_XFER set.
            */
            TESTCASE_SKIP("env CFLSH_BLK_MAX_XFER is _not_ set");
            return;

    } else if (!nblks){
            nblks = 513; /* 2M +*/
    }



    ASSERT_EQ(0,blk_fvt_setup(nblks));

    // open physical  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_size_flag = 0; 
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    if (lun_sz < nblks)
        nblks = lun_sz;

    xfersz = nblks;
    cmd = FV_WRITE;
    lba = 1;
    
    blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags); 
    
    EXPECT_EQ(xfersz, (size_t)ret);
    if ((int)xfersz != ret) {
       fprintf(stderr,"Write failed 1M+ xfersz \n");
       blk_open_tst_cleanup();
       return;
    }
    
    cmd = FV_READ;
    lba = 1;
    // read what was wrote  xfersize
    blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags); 
    
    EXPECT_EQ(xfersz, (size_t)ret);
    if ((int)xfersz != ret) {
       fprintf(stderr,"Read failed 1M+ xfersz \n");
       blk_open_tst_cleanup();
       return;
    }
    // compare buffers
    
    blk_fvt_cmp_buf(xfersz, &ret);
    if (ret != 0) {
       fprintf(stderr,"Compare failed 1M+ xfersz \n");
       blk_open_tst_cleanup();
    }
    ASSERT_EQ(0, ret);

    blk_open_tst_cleanup();
}

// Run I/O to debug failing block xfer size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_514_blksz_physical)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int        ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks = 0;
    size_t      lun_sz = 0;
    size_t      xfersz = 0;
    int         cmd;
    if ((env_filemode) &&  (atoi(env_filemode) == 1)) {
        if (env_max_xfer && (atoi(env_max_xfer)))
            nblks = atoi(env_max_xfer);
        else {
            /*
            * This test will not work  filemode and no CFLSH_BLK_MAX_XFER set.
            */
            TESTCASE_SKIP("env CFLSH_BLK_MAX_XFER is _not_ set");
            return;

        }
    } else {
            nblks = 514; /* 2M +*/
    }


    ASSERT_EQ(0,blk_fvt_setup(nblks));

    // open physical  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_size_flag = 0; 
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    if (lun_sz < nblks)
        nblks = lun_sz;

    xfersz = nblks;
    cmd = FV_WRITE;
    lba = 1;

    blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags); 
    
    EXPECT_EQ(xfersz, (size_t)ret);
    if ((int)xfersz != ret) {
           fprintf(stderr,"Write failed 1M+ xfersz \n");
           blk_open_tst_cleanup();
           return;
    }

    cmd = FV_READ;
    lba = 1;
    // read what was wrote  xfersize
    blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags); 

    EXPECT_TRUE(xfersz == (size_t)ret);
    if ((int)xfersz != ret) {
    fprintf(stderr,"Read failed 1M+ xfersz \n");
        blk_open_tst_cleanup();
        return;
    }
    // compare buffers
    
    blk_fvt_cmp_buf(xfersz, &ret);
    if (ret != 0) {
       fprintf(stderr,"Compare failed 1M+ xfersz \n");
    }
    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup();
}


// Verify read fails on not aligned (16 byte) boundaries

TEST(Block_FVT_Suite, BLK_API_FVT_read_not_16_byte_aligned)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         sz_flags= 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags = 1;
    
    size_t      temp_sz;
    int         get_set_size_flag;
    int         cmd; // 0 = read, 1, write                                     

   

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open Phys lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    temp_sz = 1;
    get_set_size_flag = 2; 

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_READ;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();
}


// Test write succeed on phy lun, prior to set size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_write_phys_lun_wo_setsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags = 0;
    
    int         cmd; // 0 = read, 1, write                                     



    ASSERT_EQ(0,blk_fvt_setup(1));

   // open Phys lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    cmd = FV_WRITE;

    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    blk_open_tst_cleanup();


}
// Test write fails on virt lun, prior to set size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_write_virt_lun_wo_setsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags = 0;
                                        
    int         cmd; // 0 = read, 1, write                                     



    ASSERT_EQ(0,blk_fvt_setup(1));

   // open virual lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    blk_open_tst_cleanup();

}



// Test write fails on write outside of lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_write_virt_lun_outside_lunsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd; // 0 = read, 1, write                                     




    ASSERT_EQ(0,blk_fvt_setup(1));

   // open virual lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1;
    get_set_size_flag = 2; 

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_WRITE;
    nblks = 1;
    lba = 0;

    // This write should succeed
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    lba = 2;
    nblks = 1;


    // This write should fail
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
  
    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);

    blk_open_tst_cleanup();

}

// Test write fails on write outside of phys lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_write_phys_lun_outside_lunsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd; // 0 = read, 1, write                                     
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 0;
    get_set_size_flag = 1; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_WRITE;

    lba = temp_sz+1;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    blk_open_tst_cleanup();
}
// Verify write fails when tried to write more than 1 block size

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_write_greater_than_1_blk)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz, nblks;
    int         cmd; // 0 = read, 1, write                                     
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        

    ASSERT_EQ(0,blk_fvt_setup(2));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_WRITE;

    lba = 0;
    nblks = 2;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

/**
    if (!max_xfer && ((env_filemode) &&  (atoi(env_filemode) == 1))) {
        EXPECT_NE(2 , ret);
        EXPECT_NE(0 , er_no);
    } else {
        EXPECT_EQ(2 , ret);
        EXPECT_EQ(0 , er_no);
    }
**/
    EXPECT_NE(2 , ret);
    EXPECT_EQ(22 , er_no);
  
    blk_open_tst_cleanup();

}


// Verify write fails when data buffer is not 16 byte aligned

TEST(Block_FVT_Suite, BLK_API_FVT_write_not_16_byte_aligned)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         sz_flags= 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags = 1;
    
    size_t      temp_sz;
    int         get_set_size_flag;
    int         cmd; // 0 = read, 1, write                                     

   

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open Phys lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    temp_sz = 1;
    get_set_size_flag = 2; 

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_WRITE;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();
}

// Verify async read issued prior to set size succeed on physical lun

TEST(Block_FVT_Suite, BLK_API_FVT_FM_async_read_phys_lun_wo_setsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd; // 0 = read, 1, write                                     
    
                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

   // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    cmd = FV_AREAD;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();

}

// Verify async read issued prior to set size fails on virtual lun

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_read_virt_lun_wo_setsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd; // 0 = read, 1, write                                     
    

                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

   // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    cmd = FV_AREAD;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_NE(1 , ret);
    EXPECT_EQ(EINVAL , er_no);
  
    blk_open_tst_cleanup();
}

// Test async read fails on read outside of lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_read_vir_lun_outside_lunsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd; // 0 = read, 1, write                                     

                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

   // open virual lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1;
    lba = 0;
    get_set_size_flag = 2; 

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AREAD;
    nblks = 1;
    lba = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);

    blk_open_tst_cleanup();

}

// Test async read fails on read outside of phys lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_read_phys_lun_outside_lunsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd; // 0 = read, 1, write                                     
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 0;
    get_set_size_flag = 1; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AREAD;

    lba = temp_sz+1;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

// TODO shouldn't be expecting 0

    EXPECT_NE(0 , ret);
    EXPECT_EQ(22 , er_no);
  
    blk_open_tst_cleanup();
}
// Verify async read fails when tried to read more than 1 block size

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_read_virt_lun_greater_than_1_blk)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz, nblks;
    int         cmd; // 0 = read, 1, write                                     
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        
    ASSERT_EQ(0,blk_fvt_setup(2));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AREAD;

    lba = 0;
    nblks = 2;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

/**
    if (!max_xfer && ((env_filemode) &&  (atoi(env_filemode) == 1))) {
        EXPECT_NE(2 , ret);
        EXPECT_NE(0 , er_no);
    } else {
        EXPECT_EQ(2 , ret);
        EXPECT_EQ(0 , er_no);
    }
**/
    EXPECT_NE(2 , ret);
    EXPECT_EQ(22 , er_no);
  
    blk_open_tst_cleanup();

}

// Verify async write issued prior to set size succeed on physical lun

TEST(Block_FVT_Suite, BLK_API_FVT_FM__async_write_phys_lun_wo_setsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd; // 0 = read, 1, write                                     
    
                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

   // open physical  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    cmd = FV_AWRITE;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

// TODO Shuldn't be NE 1 ? expect failure

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();

}


// Verify async write issued prior to set size fails on virtual lun

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_write_virt_lun_wo_setsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd; // 0 = read, 1, write                                     
    
                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

   // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    cmd = FV_AWRITE;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_NE(1 , ret);
    EXPECT_EQ(EINVAL , er_no);
  
    blk_open_tst_cleanup();
}


// Test async write fails on write outside of lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_write_vir_lun_outside_lunsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd; // 0 = read, 1, write                                     

                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

   // open virual lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1;
    lba = 0;
    get_set_size_flag = 2; 

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AWRITE;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    lba = 1;
    nblks = 1;


    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
  
    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);

    blk_open_tst_cleanup();

}

// Test async write fails on write outside of phys lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_write_phys_lun_outside_lunsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd; // 0 = read, 1, write                                     
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 0;
    get_set_size_flag = 1; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AWRITE;

    lba = temp_sz+1;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    blk_open_tst_cleanup();
}
// Verify async write fails when tried to write more than 1 block size

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_write_greater_than_1_blk)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz, nblks;
    int         cmd; // 0 = read, 1, write                                     
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        

    ASSERT_EQ(0,blk_fvt_setup(2));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AWRITE;

    lba = 0;
    nblks = 2;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_NE(2 , ret);
    EXPECT_EQ(22 , er_no);
  
    blk_open_tst_cleanup();

}

// async write data , async read data and then compare

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_write_read_compare)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;



    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AWRITE;
    lba = 1;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    cmd = FV_AREAD;
    lba = 1;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    // compare buffers
    
    blk_fvt_cmp_buf(nblks, &ret);

    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup();
}

// Verify async write fails when data buffer is not 16 byte aligned.

TEST(Block_FVT_Suite, BLK_API_FVT_awrite_not_16_byte_aligned)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         sz_flags= 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags = 1;
    
    size_t      temp_sz;
    int         get_set_size_flag;
    int         cmd; // 0 = read, 1, write                                     

   

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open Phys lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    temp_sz = 1;
    get_set_size_flag = 2; 

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AWRITE;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();
}

// Verify async read fails when data buffer is not 16 byte aligned.

TEST(Block_FVT_Suite, BLK_API_FVT_aread_not_16_byte_aligned)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         sz_flags= 0;
    int         open_cnt= 1;
    int         ret = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags = 1;
    
    size_t      temp_sz;
    int         get_set_size_flag;
    int         cmd; // 0 = read, 1, write                                     

   

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open Phys lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    temp_sz = 1;
    get_set_size_flag = 2; 

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AREAD;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();
}


// Verify CBLK_ARESULT_BLOCKING . waits till cmd completes
TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_aresult_blocking)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz, nblks;
    int         cmd; // 0 = read, 1, write                                     
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open Virtual lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AWRITE;

    lba = 0;
    nblks = 1;
    io_flags = FV_ARESULT_BLOCKING;
    io_flags = 0;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();
}

// Verify if CBLK_ARESULT_NEXT_TAG set,call doesn't returns till
// async req completes

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_aresult_next_tag)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= 1024;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         sz_flags= 0;
    size_t      temp_sz;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    
   

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    ASSERT_NE(-1 , ret);

    blocking_io_tst ( id,  &ret, &er_no);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();

}

// Verify CBLK_ARESULT_USER_TAG
TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_aresult_user_tag)
{
    chunk_id_t id                = 0;
    int        open_flags        = CBLK_OPN_VIRT_LUN;
    int        max_reqs          = 1024;
    int        er_no             = 0;
    int        open_cnt          = 1;
    int        ret               = 0;
    int        sz_flags          = 0;
    size_t     temp_sz           = 0;
    int        get_set_size_flag = 0;  // 0 = get phys lun sz
                                       // 1 = get chunk sz
                                       // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag,
                             &ret, &er_no);
    ASSERT_NE(-1 , ret);

    user_tag_io_tst ( id,  &ret, &er_no);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    blk_open_tst_cleanup();
}

// Verify CBLK_ARESULT_USER_TAG with CBLK_OPN_NO_INTRP_THREADS;
TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_aresult_user_tag_no_intrp)
{
    chunk_id_t id                = 0;
    int        open_flags        = CBLK_OPN_VIRT_LUN;
    int        max_reqs          = 1024;
    int        er_no             = 0;
    int        open_cnt          = 1;
    int        ret               = 0;
    int        sz_flags          = 0;
    size_t     temp_sz           = 0;
    int        get_set_size_flag = 0;  // 0 = get phys lun sz
                                       // 1 = get chunk sz
                                       // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));

    open_flags |= CBLK_OPN_NO_INTRP_THREADS;

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag,
                             &ret, &er_no);
    ASSERT_NE(-1 , ret);

    user_tag_io_tst ( id,  &ret, &er_no);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    blk_open_tst_cleanup();
}

TEST(Block_FVT_Suite, BLK_API_FVT_virt_lun_perf_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= 4096;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         sz_flags= 0;
    size_t      temp_sz;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    
    int num_cmds = 4096;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds+1));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 10000;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    ASSERT_NE(-1 , ret);

    io_perf_tst (id, &ret, &er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();

}

TEST(Block_FVT_Suite, BLK_API_FVT_phy_lun_perf_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 4096;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         sz_flags= 0;
    size_t      temp_sz;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    
    int num_cmds = 4096;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds+1));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_size_flag = 0; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    ASSERT_NE(-1 , ret);

    /* Test needs atleast 10000 blksz lun */
    if (temp_sz < 10000 ) {
            TESTCASE_SKIP("Lun size less than then 10000 blks");
            blk_open_tst_cleanup();
            return;
    }

    io_perf_tst (id, &ret, &er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();

}

#ifndef _AIX

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_clone_chunk)
{

    int         er_no   = 0;
    int         ret = 0;
    int         rc = 0;

    mode = O_RDWR;

    rc = fork_and_clone (&ret, &er_no, mode);
    ASSERT_NE(-1,  rc );
    EXPECT_EQ(1, ret);
    EXPECT_EQ(0, er_no);
    blk_open_tst_cleanup();
}

TEST(BLOCK_FVT_Suite, BLK_API_FVT_fork_clone_chunk_RDONLY_mode_test)
{
    int         er_no   = 0;
    int         ret = 0;
    int         rc = 0;

    int 	pmode = O_RDONLY;
    int 	cmode = O_RDONLY;

    rc = fork_and_clone_mode_test (&ret, &er_no, pmode, cmode);
    ASSERT_NE(-1,  rc );
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
    blk_open_tst_cleanup();

}

TEST(BLOCK_FVT_Suite, BLK_API_FVT_fork_clone_chunk_RDONLY_mode_errpath_1)
{
    int         er_no   = 0;
    int         ret = 0;
    int         rc = 0;

    int		pmode = O_RDONLY;
    int		cmode = O_RDWR;

    rc = fork_and_clone_mode_test (&ret, &er_no, pmode, cmode);
    ASSERT_NE(-1,  rc );
    EXPECT_NE(0, ret);
    EXPECT_NE(0, er_no);
    blk_open_tst_cleanup();

}

TEST(BLOCK_FVT_Suite, BLK_API_FVT_fork_clone_chunk_RDONLY_mode_errpath_2)
{
    int         er_no   = 0;
    int         ret = 0;
    int         rc = 0;

    int		pmode = O_RDONLY;
    int		cmode = O_WRONLY;

    rc = fork_and_clone_mode_test (&ret, &er_no, pmode, cmode);
    ASSERT_NE(-1,  rc );
    EXPECT_NE(0, ret);
    EXPECT_NE(0, er_no);
    blk_open_tst_cleanup();

}

TEST(BLOCK_FVT_Suite, BLK_API_FVT_fork_clone_chunk_WRONLY_mode_test)
{
    int         er_no   = 0;
    int         ret = 0;
    int         rc = 0;

    int 	pmode = O_WRONLY;
    int 	cmode = O_WRONLY;

    rc = fork_and_clone_mode_test (&ret, &er_no, pmode, cmode);
    ASSERT_NE(-1,  rc );
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
    blk_open_tst_cleanup();

}

TEST(BLOCK_FVT_Suite, BLK_API_FVT_fork_clone_chunk_WRONLY_mode_errpath_1)
{
    int         er_no   = 0;
    int         ret = 0;
    int         rc = 0;

    int		pmode = O_WRONLY;
    int		cmode = O_RDWR;

    rc = fork_and_clone_mode_test (&ret, &er_no, pmode, cmode);
    ASSERT_NE(-1,  rc );
    EXPECT_NE(0, ret);
    EXPECT_NE(0, er_no);
    blk_open_tst_cleanup();

}

TEST(BLOCK_FVT_Suite, BLK_API_FVT_fork_clone_chunk_WRONLY_mode_errpath_2)
{
    int         er_no   = 0;
    int         ret = 0;
    int         rc = 0;

    int		pmode = O_WRONLY;
    int		cmode = O_RDONLY;

    rc = fork_and_clone_mode_test (&ret, &er_no, pmode, cmode);
    ASSERT_NE(-1,  rc );
    EXPECT_NE(0, ret);
    EXPECT_NE(0, er_no);
    blk_open_tst_cleanup();

}

TEST(BLOCK_FVT_Suite, BLK_API_FVT_FM_UMC_fork_clone_chunk_RDWR_mode_test)
{
    int         er_no   = 0;
    int         ret = 0;
    int         rc = 0;

    int 	pmode = O_RDWR;
    int 	cmode = O_RDWR;

    rc = fork_and_clone_mode_test (&ret, &er_no, pmode, cmode);
    ASSERT_NE(-1,  rc );
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
    blk_open_tst_cleanup();

}

TEST(BLOCK_FVT_Suite, BLK_API_FVT_FM_UMC_fork_clone_chunk_RDWR_mode_errpath_1)
{
    int         er_no   = 0;
    int         ret = 0;
    int         rc = 0;

    int		pmode = O_RDWR;
    int		cmode = O_RDONLY;

    rc = fork_and_clone_mode_test (&ret, &er_no, pmode, cmode);
    ASSERT_NE(-1,  rc );
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
    blk_open_tst_cleanup();

}

TEST(BLOCK_FVT_Suite, BLK_API_FVT_FM_UMC_fork_clone_chunk_RDWR_mode_errpath_2)
{
    int         er_no   = 0;
    int         ret = 0;
    int         rc = 0;

    int		pmode = O_RDWR;
    int		cmode = O_WRONLY;

    rc = fork_and_clone_mode_test (&ret, &er_no, pmode, cmode);
    ASSERT_NE(-1,  rc );
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
    blk_open_tst_cleanup();

}

#endif

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_aresult_explicit_ret_code_tst)
{


    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         tag = max_reqs + 1;
    uint64_t    status;
    size_t      temp_sz = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz


    
    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    ASSERT_NE(-1 , ret);

    ret = cblk_aresult(id,&tag,&status,0);

    EXPECT_EQ(-1 , ret);
    EXPECT_EQ(EINVAL, errno);
  
    blk_open_tst_cleanup();

}

/*** new **/
TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_virt_multi_luns_single_thread_rw_tst)
{

    int         er_no   = 0;
    int         ret = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    num_loops = 10;
    num_threads = 1;
    num_opens = 0;
    thread_flag = 1;
    virt_lun_flags = TRUE;
    blk_thread_tst(&ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

}
TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_phys_multi_luns_single_thread_rw_tst)
{

    int         er_no   = 0;
    int         ret = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    num_loops = 10;
    num_threads = 1;
    num_opens = 0;
    thread_flag = 1;
    virt_lun_flags = FALSE;
    blk_thread_tst(&ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

}

/*** new **/

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_virt_multi_luns_multi_thread_rw_tst)
{

    int         er_no   = 0;
    int         ret = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    num_loops = 10;
    num_threads = 10;
    num_opens = 0;
    thread_flag = 1;
    virt_lun_flags = TRUE;
    blk_thread_tst(&ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

}


TEST(Block_FVT_Suite, BLK_API_FVT_FM_phy_lun_multi_thread_rw_tst)
{

    int         er_no   = 0;
    int         ret = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    num_loops = 10;
    num_threads = 10;
    num_opens = 0;
    thread_flag = 1;
    virt_lun_flags = FALSE;
    blk_thread_tst(&ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

}

TEST(Block_FVT_Suite, BLK_API_FVT_virt_lun_share_cntxt_rw_tst)
{

    int         er_no   = 0;
    int         ret = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ret = validate_share_context();
    if (ret) {
           TESTCASE_SKIP("context _not_ sharable");
           if (blk_fvt_data_buf != NULL)
               free(blk_fvt_data_buf);
           if (blk_fvt_comp_data_buf != NULL)
               free(blk_fvt_comp_data_buf);
           return;
    }
    num_loops = 10;
    num_threads = 10;
    num_opens = 0;
    thread_flag = 1;
    virt_lun_flags = TRUE;
    share_cntxt_flags = TRUE;
    blk_thread_tst(&ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

}

TEST(Block_FVT_Suite, BLK_API_FVT_phys_lun_share_cntxt_rw_tst)
{

    int         er_no   = 0;
    int         ret = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ret = validate_share_context();
    if (ret) {
           TESTCASE_SKIP("context _not_ sharable");
           if (blk_fvt_data_buf != NULL)
               free(blk_fvt_data_buf);
           if (blk_fvt_comp_data_buf != NULL)
               free(blk_fvt_comp_data_buf);
           return;
    }
    num_loops = 10;
    num_threads = 10;
    num_opens = 0;
    thread_flag = 1;
    virt_lun_flags = FALSE;
    share_cntxt_flags = TRUE;
    blk_thread_tst(&ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_read_write_compare_loop_1000)
{


    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;



    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    nblks = 1;
    for ( lba = 1; lba <= 1000; lba++ ) {
        cmd = FV_WRITE;
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(1 , ret);

        cmd = FV_READ;
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(1 , ret);

        // compare buffers
    
        blk_fvt_cmp_buf(nblks, &ret);

        EXPECT_EQ(0, ret);
    }
    // get statistics
    blk_get_statistics(id,open_flags,&ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_open_tst_cleanup();
}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_get_chunk_status_num_commands)
{
  
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;
    chunk_stats_t stats;



    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    ASSERT_EQ(0 , ret);
    ASSERT_EQ(0 , er_no);

    nblks = 1;
    for ( lba = 1; lba <= 1000; lba++ ) {
        cmd = FV_WRITE;
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(1 , ret);

        cmd = FV_READ;
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(1 , ret);

        // compare buffers
    
        blk_fvt_cmp_buf(nblks, &ret);

        EXPECT_EQ(0, ret);
    }
    // get statistics

    ret = cblk_get_stats (id, &stats, 0);

    EXPECT_EQ(0 , ret);
    EXPECT_TRUE(1000 == stats.num_reads);
    EXPECT_TRUE(1000 == stats.num_writes);
    EXPECT_TRUE(1000 == stats.num_blocks_read);
    EXPECT_TRUE(1000 == stats.num_blocks_written);


    blk_open_tst_cleanup();
    
}


TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_aread_awrite_compare_loop_1000)
{


    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;



    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    ASSERT_EQ(0 , ret);
    ASSERT_EQ(0 , er_no);

    nblks = 1;
    for ( lba = 1; lba <= 1000; lba++ ) {
        cmd = FV_AWRITE;
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(1 , ret);

        cmd = FV_AREAD;
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(1 , ret);

        // compare buffers
    
        blk_fvt_cmp_buf(nblks, &ret);

        EXPECT_EQ(0, ret);
    }

    blk_open_tst_cleanup();
}

// Test NO_INTRP async io tests

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_virt_lun_NO_INTRP_THREAD_SET_async_io)
{


    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;



    ASSERT_EQ(0,blk_fvt_setup(1));

    open_flags |= CBLK_OPN_NO_INTRP_THREADS;

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    nblks = 1;
    lba = 1;
    cmd = FV_AWRITE;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    // If errors no reason to continue 
    if (ret != 1) {
        blk_open_tst_cleanup();
        return;
    }
    cmd = FV_AREAD;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    // If errors no reason to continue 
    if (ret != 1) {
        blk_open_tst_cleanup();
        return;
    }
    // compare buffers
    
    blk_fvt_cmp_buf(nblks, &ret);

    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup();
}

// Test NO_INTRP synchronous io tests

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_virt_lun_NO_INTRP_THREAD_SET_sync_io)
{


    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;



    ASSERT_EQ(0,blk_fvt_setup(1));

    open_flags |= CBLK_OPN_NO_INTRP_THREADS;

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    nblks = 1;
    lba = 1;
    cmd = FV_WRITE;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    // If errors no reason to continue 
    if (ret != 1) {
        blk_open_tst_cleanup();
        return;
    }
    cmd = FV_READ;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    // If errors no reason to continue 
    if (ret != 1) {
        blk_open_tst_cleanup();
        return;
    }
    // compare buffers
    
    blk_fvt_cmp_buf(nblks, &ret);

    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup();
}

// testflag 1 - NO_INTRP set, null status, io_flags ARW_USER  set
// verify it ret -1 and errno set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_NO_INTRP_THREAD_ARG_TEST_1)
{


    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    int         testflag = 1;
    size_t      temp_sz;



    ASSERT_EQ(0,blk_fvt_setup(1));

    testflag = 1;
    open_flags |= CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret, &er_no);

    EXPECT_EQ(-1 , ret);
    EXPECT_NE(0 , er_no);

    blk_open_tst_cleanup();
}

// testflag 2 - NO_INTRP _not set,  status, io_flags ARW_USER not set
// expect success , verify arw_status is not updated.

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_NO_INTRP_THREAD_ARG_TEST_2)
{


    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    int         testflag = 0;
    size_t      temp_sz;



    ASSERT_EQ(0,blk_fvt_setup(1));

    testflag = 2;
    open_flags |= CBLK_OPN_VIRT_LUN ;

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret, &er_no);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0  , er_no);

    blk_open_tst_cleanup();
}

// testflag 3 - NO_INTRP flag _not set, status, io_flags ARW_USER_STATUS  set
// Write should complete ,pass test, no errno set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_NO_INTRP_THREAD_ARG_TEST_3)
{


    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    int         testflag = 0;
    size_t      temp_sz;



    ASSERT_EQ(0,blk_fvt_setup(1));

    testflag = 3;
    open_flags |= CBLK_OPN_VIRT_LUN ;

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret, &er_no);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    blk_open_tst_cleanup();
}

// testflag 4 - NO_INTRP flag set, status, io_flags ARW_USER  set
// expect failure ret code -1 and errno set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_NO_INTRP_THREAD_ARG_TEST_4)
{


    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    int         testflag = 0;
    size_t      temp_sz;



    ASSERT_EQ(0,blk_fvt_setup(1));

    testflag = 4;
    open_flags |= CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret, &er_no);

    EXPECT_EQ(-1 , ret);
    EXPECT_NE( 0 , er_no);

    blk_open_tst_cleanup();
}

// testflag 5 - NO_INTRP flag set, status,  ARW_USER | ARW_WAIT set
// expect failure ret code -1 and errno set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_NO_INTRP_THREAD_ARG_TEST_5)
{


    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    int         testflag = 0;
    size_t      temp_sz;



    ASSERT_EQ(0,blk_fvt_setup(1));

    testflag = 5;
    open_flags |= CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret, &er_no);

    EXPECT_EQ(-1 , ret);
    EXPECT_NE( 0, er_no);

    blk_open_tst_cleanup();
}

// testflag 6 - NO_INTRP flag set, status, ARW_USER | ARW_WAIT| ARW_USER_TAG set
// expect failure ret = -1 and errno set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_NO_INTRP_THREAD_ARG_TEST_6)
{


    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    int         testflag = 0;
    size_t      temp_sz;



    ASSERT_EQ(0,blk_fvt_setup(1));

    testflag = 6;
    open_flags |= CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret, &er_no);

    EXPECT_EQ(-1 , ret);
    EXPECT_NE( 0, er_no);

    blk_open_tst_cleanup();
}

// Mix read/write compare with async I/O and sync I/O
TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_mix_i_o_loop_1000)
{


    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;



    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    nblks = 1;
    for ( lba = 1; lba <= 1000; lba++ ) {
        (lba & 0x1) ? (cmd = FV_WRITE):(cmd = FV_AWRITE);
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(1 , ret);

        // If errors no reason to continue 
        if (ret != 1) {
            break;
        }

        (lba & 0x1) ? (cmd = FV_READ):(cmd = FV_AREAD);
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(1 , ret);

        // If errors no reason to continue 
        if (ret != 1) {
            break;
        }

        // compare buffers
    
        blk_fvt_cmp_buf(nblks, &ret);

        EXPECT_EQ(0, ret);
    }

    blk_open_tst_cleanup();
}

// Mix read/write compare with async I/O and sync I/O with NO_INTRP
TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_mix_i_o__no_intrp_loop_1000)
{


    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;



    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    nblks = 1;
    for ( lba = 1; lba <= 1000; lba++ ) {
        (lba & 0x1) ? (cmd = FV_WRITE):(cmd = FV_AWRITE);
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(1 , ret);

        // If errors no reason to continue 
        if (ret != 1) {
            break;
        }

        (lba & 0x1) ? (cmd = FV_READ):(cmd = FV_AREAD);
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(1 , ret);

        // If errors no reason to continue 
        if (ret != 1) {
            break;
        }

        // compare buffers
    
        blk_fvt_cmp_buf(nblks, &ret);

        EXPECT_EQ(0, ret);
    }

    blk_open_tst_cleanup();
}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_list_io_args_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 1000;  // will be using list with 100 i/o requests
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         i;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int		arg_tst = 0;


    mode = O_RDWR;

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 16;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    for (i=1 ; i<10; i++) {
        arg_tst +=1 ;
        er_no=ret=0;
        blk_list_io_arg_test(id, arg_tst, &er_no, &ret);

       	EXPECT_EQ(-1, ret);
       	EXPECT_EQ(EINVAL, er_no);

        // If errors no reason to continue 
        if ((ret != -1) || (er_no != EINVAL)) {
            break;
        }

    }

    blk_open_tst_cleanup();


}

// Test listio on virt lun, without Timout, and with CBLK_IO_USER_STATUS flag set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_virt_lun_list_io_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 1000;  // will be using list with 500 i/o requests
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 1;
    uint64_t    timeout = 0;
    int		uflags = CBLK_IO_USER_STATUS;
    int         cmd ;
    int         i=0;

    /* Number of 4K bufs */
    io_bufcnt = 500;


    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }


    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret,num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }


    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }


    /* compare 1blk at a time */
    for (i=0; i<num_listio; i++) {
        ret = memcmp( (void *)((char *)blk_fvt_data_buf + ((BLK_FVT_BUFSIZE)*i)),
                 (void *)((char *)blk_fvt_comp_data_buf + ((BLK_FVT_BUFSIZE)*i)), 
                BLK_FVT_BUFSIZE);
        er_no = errno;
        EXPECT_EQ(0 , ret);
        EXPECT_EQ(0,  er_no );
        if (ret) {
            fprintf(stderr, "list_io miscomare at block %d\n",i);
            fprintf(stderr, "Read:\n");
            hexdump(((char*)blk_fvt_data_buf + ((BLK_FVT_BUFSIZE)*i)),100,NULL);
            fprintf(stderr, "Wrote:\n");
            hexdump(((char*)blk_fvt_comp_data_buf + ((BLK_FVT_BUFSIZE)*i)),100,NULL);
            break;
        } 
    }
    blk_open_tst_cleanup();
}

// Test listio on virt lun, without Timout, and with CBLK_IO_USER_STATUS flag set and NO_INTRP flag set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_virt_lun_list_io_test_no_intrp)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;
    int         sz_flags= 0;
    int         max_reqs= 1000;  // will be using list with 500 i/o requests
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 2;
    uint64_t    timeout = 0;
/*
    int		uflags = CBLK_IO_USER_STATUS;
*/
    int		uflags = 0;
    int         cmd ;
    int         i=0;

    /* Number of 4K bufs */
    io_bufcnt = 500;


    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }


    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret,num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }


    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }


    /* compare 1blk at a time */
    for (i=0; i<num_listio; i++) {
        ret = memcmp( (void *)((char *)blk_fvt_data_buf + ((BLK_FVT_BUFSIZE)*i)),
                 (void *)((char *)blk_fvt_comp_data_buf + ((BLK_FVT_BUFSIZE)*i)), 
                BLK_FVT_BUFSIZE);
        er_no = errno;
        EXPECT_EQ(0 , ret);
        EXPECT_EQ(0,  er_no );
        if (ret) {
            fprintf(stderr, "list_io miscomare at block %d\n",i);
            fprintf(stderr, "Read:\n");
            hexdump(((char*)blk_fvt_data_buf + ((BLK_FVT_BUFSIZE)*i)),100,NULL);
            fprintf(stderr, "Wrote:\n");
            hexdump(((char*)blk_fvt_comp_data_buf + ((BLK_FVT_BUFSIZE)*i)),100,NULL);
            break;
        } 
    }
    blk_open_tst_cleanup();
}

// Test listio on physical lun, without Timout, and with CBLK_IO_USER_STATUS flag set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_phys_lun_list_io_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 1000;  // will be using list with 500 i/o requests
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 1;
    uint64_t    timeout = 0;
    int		uflags = CBLK_IO_USER_STATUS;
    int         cmd ;

    /* Number of 4K bufs */
    io_bufcnt = 500;

    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_size_flag = 0; 
    // get phys lun size
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }
    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }

    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }

    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE*num_listio);
    er_no = errno;

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    blk_open_tst_cleanup();
}

// Test listio on physical lun, without Timout, and with CBLK_IO_USER_STATUS flag set , NO_INTRP set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_phys_lun_list_io_test_no_intrp)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_NO_INTRP_THREADS;
    int         sz_flags= 0;
    int         max_reqs= 1000;  // will be using list with 500 i/o requests
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 2;
    uint64_t    timeout = 0;
/*
    int		uflags = CBLK_IO_USER_STATUS;
*/
    int		uflags = 0;
    int         cmd ;

    /* Number of 4K bufs */
    io_bufcnt = 500;

    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_size_flag = 0; 
    // get phys lun size
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }
    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }

    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }

    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE*num_listio);
    er_no = errno;

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    blk_open_tst_cleanup();
}


// Test listio on virtual lun without user status flag , no timeout set 
// Passes issue_list, wait_list and complete_list and then polls wait list for 
// completions

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_virt_lun_list_io_test_1)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 1000;  // will be using list with 500 i/o requests
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 2;
    uint64_t    timeout = 0;
    int		uflags = 0;
    int         cmd ;


    /* Number of 4K bufs */
    io_bufcnt = 500;

    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }

    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }


    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup();
            return;
        }

    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE*num_listio);
    er_no = errno;

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    blk_open_tst_cleanup();
}

// Test listio on virt lun, without timeout and without CBLK_USER_IO_STATUS
// First listio issued with issue_list and complete_list
// then completions verfied by keep issuing listio with updated pending_list and
// completion_list , till all commands comeback with either success or failure.

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_virt_lun_list_io_test_2)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 1000;  // will be using list with 500 i/o requests
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 3;
    uint64_t    timeout = 0;
    int		uflags = 0;
    int         cmd ;

    /* Number of 4K bufs */
    io_bufcnt = 500;

    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE*num_listio);
    er_no = errno;

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
    blk_open_tst_cleanup();
}

// Test listio on physical lun without user status flag , no timeout set 
// Passes issue_list, wait_list and complete_list and then polls wait list for 
// completions

TEST(Block_FVT_Suite, BLK_API_FVT_FM_phys_lun_list_io_test_1)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 1000;  // will be using list with 500 i/o requests
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 2;
    uint64_t    timeout = 0;
    int		uflags = 0;
    int         cmd ;
    /* Number of 4K bufs */
    io_bufcnt = 500;

    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );


    temp_sz = 0;
    get_set_size_flag = 0; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    if ((temp_sz < 1024) || ret || er_no ){
        blk_open_tst_cleanup();
        return;
    }

    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE*num_listio);
    er_no = errno;

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    blk_open_tst_cleanup();
}

// Test listio on Physical lun, without timeout and without CBLK_USER_IO_STATUS
// First listio issued with issue_list and complete_list
// then completions verfied by keep issuing listio with updated pending_list and
// completion_list , till all commands comeback with either success or failure.

TEST(Block_FVT_Suite, BLK_API_FVT_FM_phys_lun_list_io_test_2)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 1000;  // will be using list with 500 i/o requests
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 3;
    uint64_t    timeout = 0;
    int		uflags = 0;
    int         cmd ;


    /* Number of 4K bufs */
    io_bufcnt = 500;
    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 0;
    get_set_size_flag = 0; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    if (temp_sz < 1024) {
        blk_open_tst_cleanup();
        return;
    }
    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE*num_listio);
    er_no = errno;

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
    blk_open_tst_cleanup();
}

// Verifies data perisists across closes on physical luns

TEST(Block_FVT_Suite, BLK_API_FVT_FM_Persistence_phys_lun_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd;

    mode = O_RDWR;

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(256));

    // open physical lun

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );

    // clear physcial lun , write all 0

    // write  data at known lba 
    cmd = FV_WRITE;
    lba = 1;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0,  er_no );
    
    
    // read data at known lba
    cmd = FV_READ;
    lba = 1;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0,  er_no );

    // compare data is written correctly
    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf,BLK_FVT_BUFSIZE);
    er_no = errno;
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0,  er_no );

    // close physical lun
    cblk_close (id,0);
    num_opens --;


    // open physical lun again

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );
    EXPECT_EQ(0,  er_no );

    // read data from known lba
    cmd = FV_READ;
    lba = 1;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0,  er_no );

    // compare data , it should have persisted after closed lun.
    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf,BLK_FVT_BUFSIZE);
    er_no = errno;
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0,  er_no );


    blk_open_tst_cleanup();

}

#ifdef _AIX

// Open phys lun with reserve 
// have one host  Open lun with reserve  set
// do I/O 

TEST(Block_FVT_Suite, BLK_API_FVT_scsi_reserve_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd;

    mode = O_RDWR;

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(1));

    open_flags |= CBLK_OPN_RESERVE;

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );

    // Try to write , it should pass
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    // Try to read it should pass
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  

    blk_open_tst_cleanup();

}

// Open phys lun with scsi reserve and mpio_fo mode
// expect open faiulure with EINVAL

TEST(Block_FVT_Suite, BLK_API_FVT_scsi_reserve_err_path_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd;

    mode = O_RDWR;

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(1));

    open_flags |= (CBLK_OPN_RESERVE|CBLK_OPN_MPIO_FO);

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    EXPECT_EQ (NULL_CHUNK_ID,  id );
    EXPECT_NE (0 , er_no);

    blk_open_tst_cleanup();

}

// validate  CBLK_OPN_RESERVE and CBLK_OPN_FORCED_RESERVE flags

TEST(Block_FVT_Suite, BLK_API_FVT_scsi_force_reserve_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd;

    mode = O_RDWR;

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(1));

    open_flags |= CBLK_OPN_RESERVE;

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Try to write , it should pass
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    if ((ret !=1 ) || (errno)) {
	blk_open_tst_cleanup();
	return;
    }

    // close physical lun
    cblk_close (id,0);
    num_opens --;

    // re-open with forced reserve 

    open_flags = CBLK_OPN_FORCED_RESERVE;
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );
  
    // Try to Read , it should pass
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    blk_open_tst_cleanup();

}

// export env variable FVT_NUM_LOOPS to 2000
// Configure system with 2 paths with single reserve policy
// Open phys lun with mpio_fo
// do I/O , check for active path
// disable active path
// verify I/O continues on failover path
// read data and verify 

TEST(Block_FVT_Suite, BLK_API_FVT_phy_mpio_cabl_pull_io)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= 4096;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         sz_flags= 0;
    size_t      temp_sz;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    
    int num_cmds = 4096;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds+1));

    open_flags |= CBLK_OPN_MPIO_FO;


    // open physical lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_size_flag = 0; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &ret, &er_no);
    ASSERT_NE(-1 , ret);

    /* Test needs atleast 10000 blksz lun */
    if (temp_sz < 10000 ) {
            TESTCASE_SKIP("Lun size less than then 10000 blks");
            blk_open_tst_cleanup();
            return;
    }

    io_perf_tst (id, &ret, &er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup();

}

#endif /* _AIX */
