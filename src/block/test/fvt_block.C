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
/*******************************************************************************
 * \file
 * \brief
 * \ingroup
 ******************************************************************************/

#include <gtest/gtest.h>

extern "C"
{
#include <pg.h>
#include <blk_tst.h>
uint64_t    block_number = 0;
int         mode = O_RDWR;
int         num_opens = 0;
uint32_t    thread_count = 0;
uint64_t    max_xfer = 0;
int         num_loops;
int         thread_flag;
int         num_threads;
int         virt_lun_flags=0;
int         share_cntxt_flags=0;
int         io_bufcnt = 1;
int         num_listio = MAX_NUM_CMDS;
int         test_max_cntx = 0;
int         host_type = 0;
int         dev_type  = 0;
int         max_cntxt = 0;
chunk_id_t  chunks[MAX_OPENS+15];
void        *blk_fvt_data_buf = NULL;
void        *blk_fvt_comp_data_buf = NULL;
char        *env_filemode = getenv("BLOCK_FILEMODE_ENABLED");
char        *env_max_xfer = getenv("CFLSH_BLK_MAX_XFER");
char        *env_num_cntx = getenv("MAX_CNTX");
extern char dev_paths[MAX_LUNS][128];
extern chunk_ext_arg_t ext;
}

/**
 *******************************************************************************
 * \brief
 *   test idx_t functions
 ******************************************************************************/
TEST(Block_FVT_Suite, SIMPLE_idx)
{
    idx_t *p_idx = idx_new(7);
    IDX    gi    = 0;
    int    i     = 0;

    for (i=0; i<7; i++)
    {
        ASSERT_EQ(0,idx_getl(p_idx,&gi));
        ASSERT_EQ(i,gi);
    }
    ASSERT_EQ(EAGAIN,idx_getl(p_idx,&gi));

    for (i=6; i>=0; i--)
    {
        ASSERT_EQ(0,idx_putl(p_idx,i));
    }
    ASSERT_EQ(ENOSPC,idx_putl(p_idx,i));

    idx_free(p_idx);
}

/**
 *******************************************************************************
 * \brief
 *   test pg_t functions
 ******************************************************************************/
TEST(Block_FVT_Suite, SIMPLE_pg)
{
    int   x    = 4;
    int   y    = 5;
    pg_t *p_pg = pg_new(x, y);
    int   T    = x*y;
    int   A    = x*2+2;
    int   B    = x*3+4;
    int   i    = 0;
    int   rc   = 0;
    int  *ret  = 0;
    int   p[T];

    // fill
    for (i=0; i<T; i++)
    {
        ASSERT_TRUE(0 <= (rc=pg_add(p_pg,p+i)));
        ASSERT_EQ(p+i,pg_get(p_pg,rc));
    }
    // verify full
    ASSERT_EQ(-1,pg_add(p_pg,p+i));

    // remove some to create a gap
    for (i=A; i<B; i++)
    {
        ASSERT_EQ(0,pg_del(p_pg,i));
    }

    // traverse beg to end, across the gap
    i=0;
    do {if ((ret=(int*)pg_getnext(p_pg,&i))) {ASSERT_EQ(p+(i-1),ret);}}
    while (ret);

    // do 1 getnext over the gap
    rc=A;
    ASSERT_TRUE(p+B == pg_getnext(p_pg,&rc));

    // add back what was deleted
    for (i=A; i<B; i++)
    {
        ASSERT_TRUE(0 <= (rc=pg_add(p_pg,p+i)));
        ASSERT_EQ(p+i,pg_get(p_pg,rc));
    }
    // verify full
    ASSERT_EQ(-1,pg_add(p_pg,p+i));

    // remove all
    for (i=T-1; i>=0; i--)
    {
        ASSERT_EQ(0,pg_del(p_pg,i));
    }

    // re-fill
    for (i=0; i<T; i++)
    {
        ASSERT_TRUE(0 <= (rc=pg_add(p_pg,p+i)));
        ASSERT_EQ(p+i,pg_get(p_pg,rc));
    }
    // verify full
    ASSERT_EQ(-1,pg_add(p_pg,p+i));

    // re-empty
    for (i=0; i<T; i++)
    {
        ASSERT_EQ(0,pg_del(p_pg,i));
    }

    // verify empty
    rc=0;
    ASSERT_EQ((void*)NULL, pg_getnext(p_pg,&rc));

    pg_free(p_pg);

    // alloc/fill/empty/free 1million element list
    x=200; y=5000;
    ASSERT_TRUE(NULL != (p_pg=pg_new(x,y)));
    for (i=0; i<x*y; i++)
    {
        ASSERT_EQ(i, pg_add(p_pg,p+i));
    }
    for (i=(x*y)-1; i>=0; i--)
    {
        ASSERT_EQ(0, pg_del(p_pg,i));
    }
    pg_free(p_pg);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_open)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_ID;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_opens)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_ID;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 16;
    int         ret      = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_multi_opens)
{
    chunk_id_t  id[4]    = {0};
    int         flags    = CBLK_GROUP_ID;
    int         max_reqs = 64;
    int         rc       = 0;
    int         i        = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));

    for (i=0; i<10; i++)
    {
        if (env_filemode && atoi(env_filemode)==1) {ext=1;} else {ext=1;}
        id[0] = cblk_cg_open(dev_paths[0], max_reqs, mode, ext, 0, flags);
        ASSERT_NE(NULL_CHUNK_ID, id[0]);
        if (env_filemode && atoi(env_filemode)==1) {ext=1;} else {ext=2;}
        id[1] = cblk_cg_open(dev_paths[0], max_reqs, mode, ext, 0, flags);
        ASSERT_NE(NULL_CHUNK_ID, id[1]);
        if (env_filemode && atoi(env_filemode)==1) {ext=1;} else {ext=8;}
        id[2] = cblk_cg_open(dev_paths[0], max_reqs, mode, ext, 0, flags);
        ASSERT_NE(NULL_CHUNK_ID, id[2]);
        if (env_filemode && atoi(env_filemode)==1) {ext=1;} else {ext=32;}
        id[3] = cblk_cg_open(dev_paths[0], max_reqs, mode, ext, 0, flags);
        ASSERT_NE(NULL_CHUNK_ID, id[3]);

        rc = cblk_close(id[2], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[0], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[3], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[1], flags);
        EXPECT_EQ(0, rc);
    }
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_plun_open_READONLY)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_ID;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    uint64_t    lba;
    int         io_flags = CBLK_GROUP_ID;
    size_t      nblks;
    int         cmd;
    int         ret = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));

    mode = O_RDONLY;
    ext  = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_EQ(1 , ret);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_plun_open_WRONLY)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_ID;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    uint64_t    lba;
    int         io_flags = CBLK_GROUP_ID;
    size_t      nblks;
    int         cmd;
    int         ret      = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));

    mode = O_WRONLY;
    ext  = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    cmd   = FV_READ;
    lba   = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    cmd   = FV_WRITE;
    lba   = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_EQ(1, ret);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_open_RDWR)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_ID;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    uint64_t    lba;
    int         io_flags = CBLK_GROUP_ID;
    size_t      nblks;
    int         cmd;
    int         ret = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    EXPECT_EQ(0, er_no);

    cmd   = FV_WRITE;
    lba   = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_EQ(1, ret);

    cmd   = FV_READ;
    lba   = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_EQ(1, ret);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/******************************************************************************
 * \brief
 ******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_open_exceed_max_reqs)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_ID;
    int         max_reqs = MAX_NUM_REQS;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    EXPECT_NE(NULL_CHUNK_ID,  id);

    if (id == NULL_CHUNK_ID)
    {
        blk_open_tst_cleanup(0,  &ret,&er_no);
        EXPECT_EQ(0, ret);
        return;
    }

    max_reqs = MAX_NUM_REQS + 1;
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, flags, mode);

    // expect failure
    EXPECT_EQ(NULL_CHUNK_ID, id);
    EXPECT_EQ(12, er_no);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/******************************************************************************
 * \brief
 ******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_plun_open_Max_context_tst)
{
    int         open_flags = CBLK_GROUP_ID;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         ret        = 0;
    int         cntxts     = 0;

    TESTCASE_SKIP_IF_FILEMODE;

    ASSERT_EQ(0,blk_fvt_setup(1));

    ext    = 4;
    cntxts = max_cntxt / ext;

    max_context(&ret, &er_no, max_reqs, cntxts, open_flags,mode);
    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/*******************************************************************************
 * \brief
 ******************************************************************************/
 TEST(Block_FVT_Suite, BLK_API_FVT_CG_plun_open_Exceed_Max_context_tst)
 {
     int         open_flags = CBLK_GROUP_ID;
     int         max_reqs   = 64;
     int         er_no      = 0;
     int         ret        = 0;
     int         cntxts     = 0;

     TESTCASE_SKIP_IF_FILEMODE;

     ASSERT_EQ(0,blk_fvt_setup(1));

     ext    = 4;
     cntxts = (max_cntxt / ext) + 1;

     max_context(&ret, &er_no, max_reqs, cntxts, open_flags, mode);

     EXPECT_NE(0, ret);

     blk_open_tst_cleanup(0,  &ret,&er_no);
     EXPECT_EQ(0, ret);
}

/*******************************************************************************
 * \brief
 ******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_vlun_open_not_allowed)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_OPN_VIRT_LUN | CBLK_GROUP_ID;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);

    ASSERT_EQ(NULL_CHUNK_ID, id);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_open_invalid_dev_path)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_ID;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret      = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    strcpy(dev_paths[0],"junk");
    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);

    EXPECT_EQ(NULL_CHUNK_ID,  id);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_close_unopen)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_ID;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         close_flag = CBLK_GROUP_ID;
    int         ret        = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID, id);

    blk_close_tst(id, &ret, &er_no, close_flag);

    EXPECT_EQ(0, ret);

    // Close un-opened lun
    blk_close_tst(id, &ret, &er_no, close_flag);

    EXPECT_EQ(EINVAL, er_no);
    EXPECT_EQ(-1,     ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_get_size)
{
    chunk_id_t  id         = 0;
    int         flags      = CBLK_GROUP_ID;
    int         sz_flags   = CBLK_GROUP_ID;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    size_t      phys_lun_sz = 0;
    int         get_set_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size
    size_t      ret_size = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, flags, mode);

    ASSERT_NE(NULL_CHUNK_ID, id);

    // Get phys lun size
    blk_fvt_get_set_lun_size(id, &phys_lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);

    EXPECT_EQ(0, ret);
    EXPECT_NE((size_t)0, phys_lun_sz);

    // get size on phys lun should report whole phys lun size
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &ret_size, sz_flags, get_set_flag,
                             &ret,&er_no);

    EXPECT_EQ(0, ret);

    // test chunk size should be  equal whole phys lun size
    EXPECT_EQ(phys_lun_sz , ret_size);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_set_size)
{
    chunk_id_t  id         = 0;
    int         flags      = CBLK_GROUP_ID;
    int         sz_flags   = CBLK_GROUP_ID;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    size_t      lun_sz     = 0;
    int         get_set_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size
    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // set size on phy lun, it should fail, since it can not be changed
    get_set_flag = 2;
    lun_sz       = 4096;

    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);

    EXPECT_EQ(-1,     ret);
    EXPECT_EQ(EINVAL, er_no);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_read_wo_setsz)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_ID;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags   = CBLK_GROUP_ID;
    int         cmd; // 0 = read, 1, write

    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID, id);

    cmd   = FV_READ;
    lba   = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    // expect good ret code
    EXPECT_EQ(1, ret);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_1M_thru_16M_xfersize)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_ID;
    int         sz_flags   = CBLK_GROUP_ID;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    uint64_t    lba        = 0;
    int         io_flags   = CBLK_GROUP_ID;
    size_t      nblks      = 4096;
    size_t      lun_sz     = 0;
    size_t      xfersz     = 0;
    int         cmd        = 0;
    size_t      i          = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                   // 1 = get chunk sz
                                   // 2 = set chunk sz
    TESTCASE_SKIP_IF_FILEMODE;

    ASSERT_EQ(0,blk_fvt_setup(nblks));
    ext = 4;

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID, id);

    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0, ret);

    if (lun_sz < nblks) {nblks = lun_sz;}

    for (i = 256; i < nblks ; i+= 256)
    {
        xfersz = i;
        cmd = FV_WRITE;
        lba = 0;

        blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags);

        EXPECT_EQ(xfersz, (size_t)ret);
        if ((int)xfersz != ret) {
           fprintf(stderr,"Write failed xfersz 0x%lx\n",i);
           blk_open_tst_cleanup(open_flags,  &ret,&er_no);
           EXPECT_EQ(0, ret);
           return;
        }

        cmd = FV_READ;
        lba = 0;
        // read what was wrote  xfersize
        blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags);

        EXPECT_EQ(xfersz, (size_t)ret);
        if ((int)xfersz != ret) {
           fprintf(stderr,"Read failed xfersz 0x%lx\n",i);
           blk_open_tst_cleanup(open_flags,  &ret,&er_no);
           EXPECT_EQ(0, ret);
           return;
        }
        // compare buffers

        blk_fvt_cmp_buf(xfersz, &ret);
        if (ret != 0) {
           fprintf(stderr,"Compare failed xfersz 0x%lx\n",i);
           blk_open_tst_cleanup(open_flags,  &ret,&er_no);
           EXPECT_EQ(0, ret);
        }
        ASSERT_EQ(0, ret);
    }

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_rd_wr_outside_lunsz)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_ID;
    int         sz_flags   = CBLK_GROUP_ID;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    uint64_t    lba;
    size_t      temp_sz,nblks;
    int         io_flags   = CBLK_GROUP_ID;
    int         cmd;                    // 0 = read, 1, write
    int         get_set_flag = 0;       // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    temp_sz      = 0;
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    cmd   = FV_READ;
    lba   = temp_sz+1;
    nblks = 1;

    // verify read outside lun size fails
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    cmd = FV_WRITE;
    // verify write outside lun size fails
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_UMC_plun_blocking_rw_tst)
{
    int open_flags = CBLK_GROUP_ID;
    int rwflags    = CBLK_GROUP_ID;
    int resflags   = CBLK_GROUP_ID | CBLK_ARESULT_BLOCKING;
    int er_no      = 0;
    int ret        = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));

    ext         = 4;
    num_loops   = 2;
    num_threads = 40;

    blk_thread_tst(&ret, &er_no, open_flags, rwflags, resflags, 0);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_UMC_plun_polling_rw_tst)
{
    int open_flags = CBLK_GROUP_ID | CBLK_OPN_NO_INTRP_THREADS;
    int rwflags    = CBLK_GROUP_ID;
    int resflags   = CBLK_GROUP_ID;
    int er_no      = 0;
    int ret        = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));

    ext         = 4;
    num_loops   = 2;
    num_threads = 40;

    blk_thread_tst(&ret, &er_no, open_flags, rwflags, resflags, 0);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_UMC_plun_rd_wr_cmp_get_stats)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_ID;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    uint64_t    lba;
    int         io_flags   = CBLK_GROUP_ID;
    size_t      nblks;
    int         cmd;
    chunk_stats_t stats;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    nblks = 1;
    for ( lba = 1; lba <= 1000; lba++)
    {
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

    ret = cblk_get_stats (id, &stats, io_flags);
    EXPECT_EQ(0 , ret);
    EXPECT_TRUE(1000 == stats.num_reads);
    EXPECT_TRUE(1000 == stats.num_writes);
    EXPECT_TRUE(1000 == stats.num_blocks_read);
    EXPECT_TRUE(1000 == stats.num_blocks_written);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_FM_plun_persistence)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_GROUP_ID;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;

    uint64_t    lba;
    int         io_flags = CBLK_GROUP_ID;
    size_t      nblks;
    int         cmd;

    ASSERT_EQ(0,blk_fvt_setup(256));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

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

    // compare data is written correctly
    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf,BLK_FVT_BUFSIZE);
    er_no = errno;
    EXPECT_EQ(0, ret);

    // close physical lun
    cblk_close (id,CBLK_GROUP_ID);
    num_opens--;

    // open physical lun again
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    // read data from known lba
    cmd = FV_READ;
    lba = 1;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_EQ(1 , ret);

    // compare data , it should have persisted after closed lun.
    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf,BLK_FVT_BUFSIZE);
    er_no = errno;
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_CG_polling_unmap_rw_tst)
{
    int         open_flags = CBLK_GROUP_ID | CBLK_OPN_NO_INTRP_THREADS;
    int         rwflags    = CBLK_GROUP_ID | TST_UNMAP;
    int         resflags   = CBLK_GROUP_ID;
    chunk_id_t  id       = 0;
    int         flags    = 0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;
    chunk_attrs_t attrs;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ext = 4;
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, 0, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    bzero(&attrs, sizeof(attrs));
    ret = cblk_get_attrs(id, &attrs, 0);
    ASSERT_EQ(ret,0);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);

    if (attrs.flags1 & CFLSH_ATTR_UNMAP)
    {
        num_loops   = 1;
        num_threads = 40;

        blk_thread_tst(&ret, &er_no, open_flags, rwflags, resflags, 0);
        EXPECT_EQ(0, ret);
    }
    else
    {
        TESTCASE_SKIP("unmap is not supported");
    }
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_open)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_RAID0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_open_READONLY)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_RAID0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    uint64_t    lba;
    int         io_flags = CBLK_GROUP_RAID0;
    size_t      nblks;
    int         cmd;
    int         ret = 0;



    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    mode = O_RDONLY;
    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Try to write , it should fail
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_NE(1 , ret);

    // Try to read it should pass
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_EQ(1 , ret);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/*******************************************************************************
 * \brief
 ******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_open_WRONLY)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_RAID0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    uint64_t    lba;
    int         io_flags = CBLK_GROUP_RAID0;
    size_t      nblks;
    int         cmd;
    int         ret = 0;



    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    mode = O_WRONLY;
    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    // Try to read , it should fail
    cmd   = FV_READ;
    lba   = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    // Try to write it should pass
    cmd   = FV_WRITE;
    lba   = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_EQ(1, ret);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/******************************************************************************
 * \brief
 ******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_open_RDWR)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_RAID0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    uint64_t    lba;
    int         io_flags = CBLK_GROUP_RAID0;
    size_t      nblks;
    int         cmd;
    int         ret = 0;



    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    //Try to write it should pass
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_EQ(1, ret);

    // Try to read , it should pass
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);
    EXPECT_EQ(1, ret);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/******************************************************************************
 * \brief
 ******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_open_exceed_max_reqs)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_RAID0;
    int         max_reqs = MAX_NUM_REQS;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;



    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    EXPECT_NE(NULL_CHUNK_ID,  id);

    if (id == NULL_CHUNK_ID)
    {
        blk_open_tst_cleanup(0,  &ret,&er_no);
        EXPECT_EQ(0, ret);
        return;
    }

    max_reqs = (MAX_NUM_REQS * cblk_cg_get_num_chunks(id,flags)) + 1;
    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);

    // expect failure
    EXPECT_EQ(NULL_CHUNK_ID, id);
    EXPECT_EQ(12, er_no);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_open_invalid_dev_path)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    strcpy(dev_paths[0],"junk");

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    EXPECT_EQ(NULL_CHUNK_ID,  id);
    EXPECT_NE(0, er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_close_unopen)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         close_flag = CBLK_GROUP_RAID0;
    int         ret        = 0;



    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    blk_close_tst(id, &ret, &er_no, close_flag);
    EXPECT_EQ(0, ret);

    // Close un-opened lun
    blk_close_tst(id, &ret, &er_no, close_flag);
    EXPECT_EQ(EINVAL, er_no);
    EXPECT_EQ(-1,     ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_get_size)
{
    chunk_id_t  id         = 0;
    int         flags      = CBLK_GROUP_RAID0;
    int         sz_flags   = CBLK_GROUP_RAID0;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    size_t      phys_lun_sz = 0;
    int         get_set_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size
    size_t      ret_size = 0;



    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    // Get phys lun size
    blk_fvt_get_set_lun_size(id, &phys_lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_NE((size_t)0, phys_lun_sz);

    // get size on phys lun should report whole phys lun size
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &ret_size, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);

    // test chunk size should be  equal whole phys lun size
    EXPECT_EQ(phys_lun_sz, ret_size);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_set_size)
{
    chunk_id_t  id         = 0;
    int         flags      = CBLK_GROUP_RAID0;
    int         sz_flags   = CBLK_GROUP_RAID0;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    size_t      lun_sz     = 0;
    int         get_set_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size


    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    // set size on phy lun, it should fail, since it can not be changed
    get_set_flag = 2;
    lun_sz       = 4096;

    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(-1,     ret);
    EXPECT_EQ(EINVAL, er_no);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_read_wo_setsz)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags   = CBLK_GROUP_RAID0;
    int         cmd; // 0 = read, 1, write

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    cmd   = FV_READ;
    lba   = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_EQ(1, ret);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_1M_thru_16M_xfersize)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0;
    int         sz_flags   = CBLK_GROUP_RAID0;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    uint64_t    lba;
    int         io_flags = CBLK_GROUP_RAID0;
    size_t      nblks    = 4096;
    size_t      lun_sz   = 0;
    size_t      xfersz   = 0;
    int         cmd;
    size_t      i        = 0;

    TESTCASE_SKIP_IF_FILEMODE;

    ASSERT_EQ(0, blk_fvt_setup(nblks));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID, id);

    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0, ret);

    if (lun_sz < nblks) {nblks = lun_sz;}

    for (i = 256; i < nblks ; i+= 256)
    {
        xfersz = i;
        cmd = FV_WRITE;
        lba = 0;

        blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags);
        EXPECT_EQ(xfersz, (size_t)ret);
        if ((int)xfersz != ret)
        {
           fprintf(stderr,"Write failed xfersz 0x%lx\n",i);
           blk_open_tst_cleanup(open_flags,  &ret,&er_no);
           EXPECT_EQ(0, ret);
           return;
        }

        cmd = FV_READ;
        lba = 0;
        // read what was wrote  xfersize
        blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags);

        EXPECT_EQ(xfersz, (size_t)ret);
        if ((int)xfersz != ret) {
           fprintf(stderr,"Read failed xfersz 0x%lx\n",i);
           blk_open_tst_cleanup(open_flags,  &ret,&er_no);
           EXPECT_EQ(0, ret);
           return;
        }
        // compare buffers
        blk_fvt_cmp_buf(xfersz, &ret);
        if (ret != 0)
        {
           fprintf(stderr,"Compare failed xfersz 0x%lx\n",i);
           blk_open_tst_cleanup(open_flags,  &ret,&er_no);
           EXPECT_EQ(0, ret);
        }
        ASSERT_EQ(0, ret);
    }

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_rd_wr_outside_lunsz)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0;
    int         sz_flags   = CBLK_GROUP_RAID0;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    uint64_t    lba;
    size_t      temp_sz,nblks;
    int         io_flags   = CBLK_GROUP_RAID0;
    int         cmd;                    // 0 = read, 1, write
    int         get_set_flag = 0;       // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz      = 0;
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0, ret);

    cmd   = FV_READ;
    lba   = temp_sz+1;
    nblks = 1;

    // verify read outside lun size fails
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    cmd = FV_WRITE;
    // verify write outside lun size fails
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_blocking_rw_tst)
{
    int open_flags = CBLK_GROUP_RAID0;
    int rwflags    = CBLK_GROUP_RAID0;
    int resflags   = CBLK_GROUP_RAID0 | CBLK_ARESULT_BLOCKING;
    int er_no      = 0;
    int ret        = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    num_loops   = 1;
    num_threads = 40;

    blk_thread_tst(&ret, &er_no, open_flags, rwflags, resflags, 0);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_polling_rw_tst)
{
    int open_flags = CBLK_GROUP_RAID0 | CBLK_OPN_NO_INTRP_THREADS;
    int rwflags    = CBLK_GROUP_RAID0;
    int resflags   = CBLK_GROUP_RAID0;
    int er_no = 0;
    int ret   = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    num_loops      = 1;
    num_threads    = 40;

    blk_thread_tst(&ret, &er_no, open_flags, rwflags, resflags, 0);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_rd_wr_cmp_get_stats)
{

    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    uint64_t    lba;
    int         io_flags   = CBLK_GROUP_RAID0;
    size_t      nblks;
    int         cmd;
    chunk_stats_t stats;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    nblks = 1;
    for ( lba = 1; lba <= 1000; lba++)
    {
        cmd = FV_WRITE;
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, io_flags);
        EXPECT_EQ(1 , ret);

        cmd = FV_READ;
        blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, io_flags);
        EXPECT_EQ(1 , ret);

        // compare buffers
        blk_fvt_cmp_buf(nblks, &ret);
        EXPECT_EQ(0, ret);
    }

    ret = cblk_get_stats (id, &stats, io_flags);
    EXPECT_EQ(0 , ret);
    EXPECT_TRUE(1000 == stats.num_reads);
    EXPECT_TRUE(1000 == stats.num_writes);
    EXPECT_TRUE(1000 == stats.num_blocks_read);
    EXPECT_TRUE(1000 == stats.num_blocks_written);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_persistence)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_GROUP_RAID0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret      = 0;
    uint64_t    lba;
    int         io_flags = CBLK_GROUP_RAID0;
    size_t      nblks;
    int         cmd;

    ASSERT_EQ(0,blk_fvt_setup(256));
    blk_fvt_RAID0_setup();

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    // clear physcial lun , write all 0
    // write  data at known lba
    cmd = FV_WRITE;
    lba = 1;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_EQ(1 , ret);

    // read data at known lba
    cmd = FV_READ;
    lba = 1;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_EQ(1 , ret);

    // compare data is written correctly
    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf,BLK_FVT_BUFSIZE);
    er_no = errno;
    EXPECT_EQ(0, ret);

    // close physical lun
    cblk_close(id,CBLK_GROUP_RAID0);
    num_opens--;

    // open physical lun again
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    // read data from known lba
    cmd = FV_READ;
    lba = 1;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_EQ(1, ret);

    // compare data , it should have persisted after closed lun.
    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf,BLK_FVT_BUFSIZE);
    er_no = errno;
    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_async_read_wo_setsz)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    uint64_t    lba        = 0;
    size_t      nblks      = 1;
    int         io_flags   = CBLK_GROUP_RAID0;
    int         cmd;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    cmd = FV_AREAD;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_EQ(1, ret);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_async_read_outside_lunsz)
{
    chunk_id_t  id           = 0;
    int         open_flags   = CBLK_GROUP_RAID0;
    int         sz_flags     = CBLK_GROUP_RAID0;
    int         max_reqs     = 64;
    int         er_no        = 0;
    int         open_cnt     = 1;
    int         ret          = 0;
    uint64_t    lba          = 0;
    size_t      lun_sz,nblks = 1;
    int         io_flags     = CBLK_GROUP_RAID0;
    int         cmd;                    // 0 = read, 1, write
    int         get_set_flag = 0;       // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    /* get lun size */
    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);

    cmd = FV_AREAD;
    lba = lun_sz+1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_EQ(EINVAL, er_no);

    blk_open_tst_cleanup(open_flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_rd_wr_greater_than_1_blk)
{
    chunk_id_t  id           = 0;
    int         open_flags   = CBLK_GROUP_RAID0;
    int         max_reqs     = 64;
    int         er_no        = 0;
    int         open_cnt     = 1;
    int         ret          = 0;
    uint64_t    lba          = 0;
    size_t      nblks        = 1;
    int         io_flags     = CBLK_GROUP_RAID0;
    int         cmd;                    // 0 = read, 1, write

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    cmd   = FV_AREAD;
    nblks = 2;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_EQ(EINVAL, er_no);

    cmd = FV_AWRITE;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_EQ(EINVAL, er_no);

    blk_open_tst_cleanup(open_flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_async_polling_perf_test)
{
    chunk_id_t  id           = 0;
    int         open_flags   = CBLK_GROUP_RAID0 | CBLK_OPN_NO_INTRP_THREADS;
    int         rwflags      = CBLK_GROUP_RAID0;
    int         resflags     = CBLK_GROUP_RAID0;
    int         max_reqs     = MAX_NUM_CMDS;
    int         er_no        = 0;
    int         open_cnt     = 1;
    int         ret          = 0;
    int         num_cmds     = MAX_NUM_CMDS;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    io_perf_tst(&ret, &er_no, rwflags, resflags, 0);
    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup(open_flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_plun_async_blocking_perf_test)
{
    chunk_id_t  id           = 0;
    int         open_flags   = CBLK_GROUP_RAID0;
    int         rwflags      = CBLK_GROUP_RAID0;
    int         resflags     = CBLK_GROUP_RAID0      | \
                               CBLK_ARESULT_BLOCKING;
    int         max_reqs     = MAX_NUM_CMDS;
    int         er_no        = 0;
    int         open_cnt     = 1;
    int         ret          = 0;
    int         num_cmds     = MAX_NUM_CMDS;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    io_perf_tst(&ret, &er_no, rwflags, resflags, 0);
    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup(open_flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_open)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         max_reqs = MAX_NUM_CMDS;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );
    EXPECT_EQ(0,  er_no );

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_open_READONLY)
{
    chunk_id_t  id           = 0;
    int         flags        = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         sz_flags     = CBLK_GROUP_RAID0;
    int         max_reqs     = MAX_NUM_CMDS;
    int         er_no        = 0;
    int         open_cnt     = 1;
    uint64_t    lba;
    int         io_flags     = CBLK_GROUP_RAID0;
    size_t      nblks;
    int         cmd;
    int         ret          = 0;
    size_t      lun_sz       = 0;
    int         get_set_flag = 0;  // 0 = get phys
                                   // 1 = get chunk
                                   // 2 = set size
    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    mode = O_RDONLY;
    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );
    EXPECT_EQ(0, er_no );

    get_set_flag = 2;
    lun_sz       = 4096;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);

    // Try to write , it should fail
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);

    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);

    // Try to read it should pass
    cmd = FV_READ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

/*******************************************************************************
 * \brief
 ******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_open_WRONLY)
{
    chunk_id_t  id           = 0;
    int         flags        = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         sz_flags     = CBLK_GROUP_RAID0;
    int         max_reqs     = 64;
    int         er_no        = 0;
    int         open_cnt     = 1;
    uint64_t    lba;
    int         io_flags     = CBLK_GROUP_RAID0;
    size_t      nblks;
    int         cmd;
    int         get_set_flag = 2;
    size_t      lun_sz       = 100;
    int         ret          = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    mode = O_WRONLY;
    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);
    EXPECT_EQ(0, er_no);

    get_set_flag = 2;
    lun_sz       = 4096;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);

    // Try to read , it should fail
    cmd   = FV_READ;
    lba   = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);

    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    // Try to write it should pass
    cmd   = FV_WRITE;
    lba   = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);

    EXPECT_EQ(1, ret);
    EXPECT_EQ(0, er_no);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

/*******************************************************************************
 * \brief
 ******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_open_RDWR)
{
    chunk_id_t  id           = 0;
    int         flags        = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         sz_flags     = CBLK_GROUP_RAID0;
    int         max_reqs     = 64;
    int         er_no        = 0;
    int         open_cnt     = 1;
    uint64_t    lba;
    int         io_flags     = CBLK_GROUP_RAID0;
    size_t      nblks;
    int         cmd;
    int         get_set_flag = 2;
    size_t      lun_sz       = 100;
    int         ret          = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);
    EXPECT_EQ(0, er_no);

    get_set_flag = 2;
    lun_sz       = 4096;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);

    //Try to write it should pass
    cmd = FV_WRITE;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    // Try to read , it should pass
    cmd = FV_READ ;
    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, flags);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}


/******************************************************************************
 * \brief
 ******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_open_exceed_max_reqs)
{
    chunk_id_t  id       = 0;
    int         flags    = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         max_reqs = MAX_NUM_REQS;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    EXPECT_NE(NULL_CHUNK_ID,  id);
    EXPECT_EQ(0,  er_no);

    if (id == NULL_CHUNK_ID)
    {
        blk_open_tst_cleanup(0,  &ret,&er_no);
        EXPECT_EQ(0, ret);
        EXPECT_EQ(0, er_no);
        return;
    }

    max_reqs = (MAX_NUM_REQS * cblk_cg_get_num_chunks(id,flags)) + 1;
    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);

    // expect failure
    EXPECT_EQ(NULL_CHUNK_ID, id);
    EXPECT_EQ(12, er_no);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_open_invalid_dev_path)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    strcpy(dev_paths[0],"junk");

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);

    EXPECT_EQ(NULL_CHUNK_ID,  id);
    EXPECT_NE(0, er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_close_unopen)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         close_flag = CBLK_GROUP_RAID0;
    int         ret        = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    blk_close_tst(id, &ret, &er_no, close_flag);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    // Close un-opened lun
    blk_close_tst(id, &ret, &er_no, close_flag);

    EXPECT_EQ(EINVAL, er_no);
    EXPECT_EQ(-1,     ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_set_get_size)
{
    chunk_id_t  id           = 0;
    int         flags        = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         sz_flags     = CBLK_GROUP_RAID0;
    int         max_reqs     = 64;
    int         er_no        = 0;
    int         open_cnt     = 1;
    int         ret          = 0;
    int         width        = 0;
    size_t      _4096        = 4096;
    size_t      lun_sz       = 0;
    size_t      ret_size     = 0;
    int         get_set_flag = 1;  // 0 = get phys
                                   // 1 = get chunk
                                   // 2 = set size
    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    width = cblk_cg_get_num_chunks(id, flags);

    // Get lun size
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ((size_t)0, lun_sz);

    /* set lun size to 4096 */
    get_set_flag = 2;
    lun_sz       = _4096 * width;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);

    // get size for vlun
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &ret_size, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(ret_size, _4096*width);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_read_wo_setsz)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    size_t      nblks;
    uint64_t    lba;
    int         io_flags   = CBLK_GROUP_RAID0;
    int         cmd; // 0 = read, 1, write

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    cmd   = FV_READ;
    lba   = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    // expect fail ret code
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_rd_wr_outside_chunksz)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         sz_flags   = CBLK_GROUP_RAID0;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    int         width      = 0;
    uint64_t    lba;
    size_t      lun_sz,nblks;
    int         io_flags   = CBLK_GROUP_RAID0;
    int         cmd;                    // 0 = read, 1, write
    int         get_set_flag = 0;       // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    width = cblk_cg_get_num_chunks(id, open_flags);

    /* set lun size to 4096 */
    get_set_flag = 2;
    lun_sz       = 4096*width;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);

    cmd   = FV_READ;
    lba   = lun_sz+1;
    nblks = 1;

    // verify read outside lun size fails
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    cmd = FV_WRITE;
    // verify write outside lun size fails
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_async_rd_wr_wo_setsz)
{
    chunk_id_t  id         = 0;
    int         open_flags = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    uint64_t    lba        = 0;
    size_t      nblks      = 1;
    int         io_flags   = CBLK_GROUP_RAID0;
    int         cmd;                    // 0 = read, 1, write

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    cmd = FV_AREAD;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    cmd = FV_AWRITE;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_NE(0, er_no);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_async_rd_wr_outside_lunsz)
{
    chunk_id_t  id           = 0;
    int         open_flags   = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         sz_flags     = CBLK_GROUP_RAID0;
    int         max_reqs     = 64;
    int         er_no        = 0;
    int         open_cnt     = 1;
    int         ret          = 0;
    int         width        = 0;
    uint64_t    lba          = 0;
    size_t      lun_sz,nblks = 1;
    int         io_flags     = CBLK_GROUP_RAID0;
    int         cmd;                    // 0 = read, 1, write
    int         get_set_flag = 0;       // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    width = cblk_cg_get_num_chunks(id, open_flags);

    /* set lun size to 4096 */
    get_set_flag = 2;
    lun_sz       = 4096*width;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);

    cmd = FV_AREAD;
    lba = lun_sz+1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_EQ(EINVAL, er_no);

    cmd = FV_AWRITE;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_EQ(EINVAL, er_no);

    blk_open_tst_cleanup(open_flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_rd_wr_greater_than_1_blk)
{
    chunk_id_t  id           = 0;
    int         open_flags   = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         sz_flags     = CBLK_GROUP_RAID0;
    int         max_reqs     = 64;
    int         er_no        = 0;
    int         open_cnt     = 1;
    int         ret          = 0;
    uint64_t    lba          = 0;
    size_t      lun_sz,nblks = 1;
    int         io_flags     = CBLK_GROUP_RAID0;
    int         cmd;                    // 0 = read, 1, write
    int         get_set_flag = 0;       // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    /* set lun size to 4096 */
    get_set_flag = 2;
    lun_sz       = 4096;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    EXPECT_EQ(0, ret);

    cmd   = FV_AREAD;
    nblks = 2;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_EQ(EINVAL, er_no);

    cmd = FV_AWRITE;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);
    EXPECT_NE(1, ret);
    EXPECT_EQ(EINVAL, er_no);

    blk_open_tst_cleanup(open_flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_polling_rw_tst)
{
    int open_flags = CBLK_GROUP_RAID0  | \
                     CBLK_OPN_VIRT_LUN | \
                     CBLK_OPN_NO_INTRP_THREADS;
    int rwflags    = CBLK_GROUP_RAID0;
    int resflags   = CBLK_GROUP_RAID0;
    int er_no      = 0;
    int ret        = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    num_loops      = 1;
    num_threads    = 40;
    thread_flag    = 1;

    blk_thread_tst(&ret, &er_no, open_flags, rwflags, resflags, 0);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_blocking_rw_tst)
{
    int open_flags = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int rwflags    = CBLK_GROUP_RAID0;
    int resflags   = CBLK_GROUP_RAID0 | CBLK_ARESULT_BLOCKING;
    int er_no      = 0;
    int ret        = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_fvt_RAID0_setup();

    num_loops      = 1;
    num_threads    = 40;

    blk_thread_tst(&ret, &er_no, open_flags, rwflags, resflags, 0);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_async_polling_perf_test)
{
    chunk_id_t  id           = 0;
    int         open_flags   = CBLK_GROUP_RAID0  | \
                               CBLK_OPN_VIRT_LUN | \
                               CBLK_OPN_NO_INTRP_THREADS;
    int         rwflags      = CBLK_GROUP_RAID0;
    int         resflags     = CBLK_GROUP_RAID0;
    int         sz_flags     = CBLK_GROUP_RAID0;
    int         max_reqs     = MAX_NUM_CMDS;
    int         er_no        = 0;
    int         open_cnt     = 1;
    int         ret          = 0;
    size_t      lun_sz       = 1;
    int         num_cmds     = MAX_NUM_CMDS;
    int         get_set_flag = 0;       // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(num_cmds));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    /* set lun size */
    get_set_flag = 2;
    lun_sz       = 10000;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0, ret);

    io_perf_tst(&ret, &er_no, rwflags, resflags, 0);
    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup(open_flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_vlun_async_blocking_perf_test)
{
    chunk_id_t  id           = 0;
    int         open_flags   = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         rwflags      = CBLK_GROUP_RAID0;
    int         resflags     = CBLK_GROUP_RAID0      | \
                               CBLK_ARESULT_BLOCKING;
    int         sz_flags     = CBLK_GROUP_RAID0;
    int         max_reqs     = MAX_NUM_CMDS;
    int         er_no        = 0;
    int         open_cnt     = 1;
    int         ret          = 0;
    size_t      lun_sz       = 1;
    int         num_cmds     = MAX_NUM_CMDS;
    int         get_set_flag = 0;       // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(num_cmds));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    /* set lun size */
    get_set_flag = 2;
    lun_sz       = 10000;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0, ret);

    io_perf_tst(&ret, &er_no, rwflags, resflags, 0);
    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup(open_flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_clone_chunk)
{
    int         open_flags   = CBLK_GROUP_RAID0 | CBLK_OPN_VIRT_LUN;
    int         io_flags     = CBLK_GROUP_RAID0;
    int         er_no        = 0;
    int         ret          = 0;
    int         rc           = 0;

    TESTCASE_SKIP_IF_FILEMODE;

    rc = fork_and_clone(&ret, &er_no, mode, open_flags, io_flags);
    ASSERT_NE(-1, rc);
    EXPECT_EQ(1,  ret);
    EXPECT_EQ(0,  er_no);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_multi_vlun_perf_test)
{
    int         open_flags  = CBLK_GROUP_RAID0  | \
                              CBLK_OPN_VIRT_LUN | \
                              CBLK_OPN_NO_INTRP_THREADS;
    int         sz_flags    = CBLK_GROUP_RAID0;
    int         rwflags     = CBLK_GROUP_RAID0;
    int         resflags    = CBLK_GROUP_RAID0;
    chunk_id_t  id          = 0;
    int         num_cmds    = MAX_NUM_CMDS;
    int         er_no       = 0;
    int         ret         = 0;
    int         i           = 0;
    size_t      psize       = 0;
    size_t      set_size    = 5000;
    int         MULTI_VLUNS = 100;
    int         vluns_in    = 0;
    char       *env_vluns   = getenv("FVT_MULTI_VLUNS");
    char       *env_filemode = getenv("BLOCK_FILEMODE_ENABLED");

    if (env_vluns)
    {
        vluns_in = atoi(env_vluns);
        if (vluns_in > 0 && vluns_in < 256) {MULTI_VLUNS=vluns_in;}
    }
    if (env_filemode && atoi(env_filemode)==1) {MULTI_VLUNS=32;}

    ASSERT_EQ(0,blk_fvt_setup(MULTI_VLUNS*num_cmds));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, num_cmds, &er_no, MULTI_VLUNS, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    blk_fvt_get_set_lun_size(id, &psize, sz_flags, 0, &ret, &er_no);
    ASSERT_NE(-1 , ret);
    set_size = (psize/(MULTI_VLUNS*2));

    for (i=0; i<MULTI_VLUNS; i++)
    {
        blk_fvt_get_set_lun_size(chunks[i], &set_size, sz_flags, 2, &ret, &er_no);
        ASSERT_NE(-1 , ret);
    }

    io_perf_tst(&ret, &er_no, rwflags, resflags, 0);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_RAID0_multi_vlun_unmap_perf_test)
{
    int         open_flags  = CBLK_GROUP_RAID0  | \
                              CBLK_OPN_VIRT_LUN | \
                              CBLK_OPN_NO_INTRP_THREADS;
    int         sz_flags    = CBLK_GROUP_RAID0;
    int         rwflags     = CBLK_GROUP_RAID0;
    int         resflags    = CBLK_GROUP_RAID0;
    chunk_id_t  id          = 0;
    int         num_cmds    = MAX_NUM_CMDS;
    int         er_no       = 0;
    int         ret         = 0;
    int         i           = 0;
    size_t      psize       = 0;
    size_t      set_size    = 5000;
    int         MULTI_VLUNS = 100;
    int         vluns_in    = 0;
    char       *env_vluns   = getenv("FVT_MULTI_VLUNS");
    char       *env_filemode = getenv("BLOCK_FILEMODE_ENABLED");
    chunk_attrs_t attrs;

    if (env_vluns)
    {
        vluns_in = atoi(env_vluns);
        if (vluns_in > 0 && vluns_in < 256) {MULTI_VLUNS=vluns_in;}
    }
    if (env_filemode && atoi(env_filemode)==1) {MULTI_VLUNS=32;}

    ASSERT_EQ(0,blk_fvt_setup(MULTI_VLUNS*num_cmds));
    blk_open_tst(&id, num_cmds, &er_no, 1, 0, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    bzero(&attrs, sizeof(attrs));
    ret = cblk_get_attrs(id, &attrs, 0);
    ASSERT_EQ(ret,0);

    blk_open_tst_cleanup(0, &ret,&er_no);
    EXPECT_EQ(0, ret);

    if (!(attrs.flags1 & CFLSH_ATTR_UNMAP))
    {
        TESTCASE_SKIP("unmap is not supported");
        return;
    }

    ASSERT_EQ(0,blk_fvt_setup(MULTI_VLUNS*num_cmds));
    blk_fvt_RAID0_setup();

    blk_open_tst(&id, num_cmds, &er_no, MULTI_VLUNS, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    blk_fvt_get_set_lun_size(id, &psize, sz_flags, 0, &ret, &er_no);
    ASSERT_NE(-1 , ret);
    set_size = (psize/(MULTI_VLUNS*2));

    for (i=0; i<MULTI_VLUNS; i++)
    {
        blk_fvt_get_set_lun_size(chunks[i], &set_size, sz_flags, 2, &ret, &er_no);
        ASSERT_NE(-1 , ret);
    }

    io_perf_tst(&ret, &er_no, rwflags, resflags, 0);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(open_flags,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/*****************************************************************/

TEST(Block_FVT_Suite, BLK_API_FVT_FM_open_phys_lun)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs    = 64;
    int         er_no   = 0;
    int         open_cnt = 1;
    int         ret      = 0;

    // Setup dev name and allocated test buffers
    ASSERT_EQ(0,blk_fvt_setup(1));

    ext = 1;

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_FM_plun_multi_opens)
{
    chunk_id_t  id[20]    = {0};
    int         flags    = 0;
    int         max_reqs = 64;
    int         rc       = 0;
    int         i        = 0;
    int         j        = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));

    for (i=0; i<100; i++)
    {
        for (j=0; j<20; j++)
        {
            id[j] = cblk_open(dev_paths[0], max_reqs, mode, 0, flags);
            ASSERT_NE(NULL_CHUNK_ID, id[j]);
        }
        rc = cblk_close(id[0], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[2], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[4], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[6], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[8], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[10], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[12], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[14], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[16], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[18], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[19], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[1], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[17], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[3], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[15], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[5], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[13], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[7], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[11], flags);
        EXPECT_EQ(0, rc);
        rc = cblk_close(id[9], flags);
        EXPECT_EQ(0, rc);
    }
}


TEST(Block_FVT_Suite, BLK_API_FVT_open_plun_RDONLY_mode_test)
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

    ASSERT_EQ(0,blk_fvt_setup(1));
    mode = O_RDONLY;

    // Setup dev name and allocated test buffers
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
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

TEST(Block_FVT_Suite, BLK_API_FVT_open_plun_WRONLY_mode_test)
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

    ASSERT_EQ(0,blk_fvt_setup(1));

    // Setup dev name and allocated test buffers
    mode = O_WRONLY;
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
  

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_open_plun_RDWR_mode_test)
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

    ASSERT_EQ(0,blk_fvt_setup(1));

    // Setup dev name and allocated test buffers
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
  

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_open_vlun_RDONLY_mode_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;

    ASSERT_EQ(0,blk_fvt_setup(1));

    // Setup dev name and allocated test buffers
    mode = O_RDONLY;
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );


    temp_sz = 1;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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
  

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_open_vlun_WRONLY_mode_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;

    ASSERT_EQ(0,blk_fvt_setup(1));

    // Setup dev name and allocated test buffers
    mode = O_WRONLY;
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );


    temp_sz = 1;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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
  

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_open_vlun_RDWR_mode_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      temp_sz,nblks;
    int         cmd;

    ASSERT_EQ(0,blk_fvt_setup(1));

    // Setup dev name and allocated test buffers
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );


    temp_sz = 1;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_open_virt_lun)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs    = 64;
    int         er_no   = 0;
    int         open_cnt = 1;
    int         ret      = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    EXPECT_EQ(0,  er_no );

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}
/** Need to check max_requests no, if valid */

TEST(Block_FVT_Suite, BLK_API_FVT_FM_open_plun_exceed_max_reqs)
{
    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs    = MAX_NUM_REQS;
    int         er_no   = 0;
    int         open_cnt = 1;
    int         ret      = 0;


    ASSERT_EQ(0,blk_fvt_setup(1));
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    // expect success

    EXPECT_NE(NULL_CHUNK_ID,  id );
    EXPECT_EQ(0,  er_no );

    if (id == NULL_CHUNK_ID) {
        blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        return;
    }

    max_reqs   = (MAX_NUM_REQS +1);
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    // expect failure

    EXPECT_EQ(NULL_CHUNK_ID,  id );

    EXPECT_EQ(12,  er_no );

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}


TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_open_vlun_exceed_max_reqs)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs   = MAX_NUM_REQS;
    int         er_no   = 0;
    int         open_cnt = 1;
    int         ret      = 0;


    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    // expect success

    EXPECT_NE(NULL_CHUNK_ID,  id );
    EXPECT_EQ(0,  er_no );

    if (id == NULL_CHUNK_ID) {
        blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        return;
    }

    max_reqs   = (MAX_NUM_REQS +1);
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    // expect failure

    EXPECT_EQ(NULL_CHUNK_ID,  id );

    EXPECT_EQ(12,  er_no );

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

// verify using physical lun, max_contex succeeds

TEST(Block_FVT_Suite, BLK_API_FVT_UMC_Max_context_tst)
{

    int         open_flags   = 0;
    int         max_reqs    = 64;
    int         er_no   = 0;
    int         ret   = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));

    max_context(&ret, &er_no, max_reqs, max_cntxt, open_flags,mode);

    EXPECT_EQ(0,  ret );
    EXPECT_EQ(0,  er_no );

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

// verify using physical lun, max_contex 509 (for AIX 495) exceeds max , should fail 


TEST(Block_FVT_Suite, BLK_API_FVT_UMC_Exceed_Max_context_tst)
{

    int         open_flags = 0;
    int         max_reqs   = 64;
    int         er_no      = 0;
    int         ret        = 0;
    int         cntxts     = 0;

    TESTCASE_SKIP_IF_FILEMODE;

    ASSERT_EQ(0,blk_fvt_setup(1));

    if (test_max_cntx) {cntxts = test_max_cntx;}
    else               {cntxts = max_cntxt + 1;}

    max_context(&ret, &er_no, max_reqs, cntxts, open_flags,mode);

    EXPECT_NE(0,  ret );
    EXPECT_NE(0,  er_no );

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_open_invalid_dev_path)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs    = 64;
    const char  *dev_path   = "junk";
    int         er_no   = 0;
    int         open_cnt = 1;
    int         ret      = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    
    blk_open_tst_inv_path(dev_path, &id, max_reqs, &er_no, open_cnt, open_flags,
                         mode);

    EXPECT_EQ(NULL_CHUNK_ID,  id);
    EXPECT_NE(0, er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
    int         get_set_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size
    size_t      ret_size = 0;


    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Get phys lun size 
   
    blk_fvt_get_set_lun_size(id, &phys_lun_sz, sz_flags, get_set_flag, &ret,
                             &er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_NE((size_t)0, phys_lun_sz);

// get size on phys lun should report whole phys lun size
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &ret_size, sz_flags,get_set_flag,&ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

// test chunk size should be  equal whole phys lun size
    EXPECT_EQ(phys_lun_sz , ret_size);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


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
    int         get_set_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size
    
    // open virtual lun
    

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Get virtual  lun size , it should fail, since it is not set
   
    get_set_flag = 1;

    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag, &ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ((size_t)0, lun_sz);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


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
    int         get_set_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open phys lun
    
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Try to set size on phy lun , it should fail, since it can not be change
   
    get_set_flag = 2;
    lun_sz = 4096;

    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag, &ret,&er_no);

    EXPECT_EQ(-1 , ret);
    EXPECT_EQ(EINVAL , er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


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
    int         get_set_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set size
    size_t      chunk_sz = 0;

    size_t      temp_sz = 0;


    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual lun
    
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Try to set size on virt lun e
   
    get_set_flag = 2;
    chunk_sz = 4096;

    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_flag,&ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    // Try to get size on virt lun and verify the set size
   
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ(chunk_sz, temp_sz);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


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
    int         get_set_flag = 0;  // 0 = get phys
                                        // 1 = get chunk
                                        // 2 = set chunk size

    size_t      lun_sz = 0;
    size_t      temp_sz = 0;


    ASSERT_EQ(0,blk_fvt_setup(1));
    
    // open phys lun

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_flag = 1;  // get phy lun size as 1 chunk

    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag, &ret,&er_no);

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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);


    EXPECT_EQ(-1 , ret);
    EXPECT_EQ(EINVAL ,er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_vlun_unmap)
{
    int         open_flags = CBLK_OPN_VIRT_LUN;
    chunk_id_t  id       = 0;
    int         flags    = 0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;
    size_t      temp_sz  = 4096;
    chunk_attrs_t attrs;

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    bzero(&attrs, sizeof(attrs));
    ret = cblk_get_attrs(id, &attrs, 0);
    ASSERT_EQ(ret,0);

    blk_fvt_get_set_lun_size(id, &temp_sz, 0, 2, &ret,&er_no);
    EXPECT_EQ(0, ret);

    if (attrs.flags1 & CFLSH_ATTR_UNMAP)
    {
        char     buf[4096] = {0};
        int      rc        = 0;
        int      tag       = -1;
        uint64_t status    = 0;

        rc = cblk_write(id, buf, 0, 1, 0);
        ASSERT_EQ(1, rc);
        rc = cblk_unmap(id, buf, 0, 1, 0);
        ASSERT_EQ(1, rc);

        rc = cblk_awrite(id, buf, 0, 1, &tag, NULL, CBLK_ARW_WAIT_CMD_FLAGS);
        ASSERT_EQ(0, rc);
        rc=cblk_aresult(id, &tag, &status, CBLK_ARESULT_BLOCKING);
        ASSERT_EQ(1, rc);
        ASSERT_EQ(0, errno);

        rc = cblk_aunmap(id, buf, 0, 1, &tag, NULL, CBLK_ARW_WAIT_CMD_FLAGS);
        ASSERT_EQ(0, rc);
        rc=cblk_aresult(id, &tag, &status, CBLK_ARESULT_BLOCKING);
        ASSERT_EQ(1, rc);
        ASSERT_EQ(0, errno);
    }
    else
    {
        TESTCASE_SKIP("unmap is not supported");
    }

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_plun_unmap)
{
    int         open_flags = 0;
    chunk_id_t  id       = 0;
    int         flags    = 0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;
    chunk_attrs_t attrs;

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    bzero(&attrs, sizeof(attrs));
    ret = cblk_get_attrs(id, &attrs, 0);
    ASSERT_EQ(ret,0);

    if (attrs.flags1 & CFLSH_ATTR_UNMAP)
    {
        char     buf[4096] = {0};
        int      rc        = 0;
        int      tag       = -1;
        uint64_t status    = 0;

        rc = cblk_write(id, buf, 0, 1, 0);
        ASSERT_EQ(1, rc);
        rc = cblk_unmap(id, buf, 0, 1, 0);
        ASSERT_EQ(1, rc);

        rc = cblk_awrite(id, buf, 0, 1, &tag, NULL, CBLK_ARW_WAIT_CMD_FLAGS);
        ASSERT_EQ(0, rc);
        rc=cblk_aresult(id, &tag, &status, CBLK_ARESULT_BLOCKING);
        ASSERT_EQ(1, rc);
        ASSERT_EQ(0, errno);

        rc = cblk_aunmap(id, buf, 0, 1, &tag, NULL, CBLK_ARW_WAIT_CMD_FLAGS);
        ASSERT_EQ(0, rc);
        rc=cblk_aresult(id, &tag, &status, CBLK_ARESULT_BLOCKING);
        ASSERT_EQ(1, rc);
        ASSERT_EQ(0, errno);
    }
    else
    {
        TESTCASE_SKIP("unmap is not supported");
    }

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz = 0;
    size_t      chunk_sz = 0;


    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual lun
    
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Get virt lun size , it sould be 0
   
    get_set_flag = 1;

    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_flag,&ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ((size_t)0, chunk_sz);

    // set chunk size (100)
   
    temp_sz = chunk_sz + 100;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    
    // Get the chunk size and Verify it set correctly
   
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_flag,&ret,&er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ(temp_sz, chunk_sz );

    // Increase chunk sz by 20 blocks 
    // set size ()
   
    temp_sz += 20;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    
    // Get size and Verify it is increased correctly to 220
    chunk_sz = 0; 
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_flag,&ret,&er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ(temp_sz, chunk_sz);

    // decrease chunk sz by 10 blocks 
    // set size ()
   
    temp_sz = (chunk_sz - 10);
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    // Get the size and Verify it is decreased correctly to 210
    chunk_sz = 0; 
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_flag,&ret,&er_no);


    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
    EXPECT_EQ(temp_sz, chunk_sz);


    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

 
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
    int         get_set_flag = 0;

                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

   // open Virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    // Try to set size on virt lun e

    get_set_flag = 2;
    chunk_sz = 4096;

    blk_fvt_get_set_lun_size(id, &chunk_sz, sz_flags, get_set_flag,&ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_READ;

    lba = 0;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
    blk_get_statistics(id, open_flags, &ret,&er_no);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}


// Test read succeed on phy lun, prior to set size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_read_plun_wo_setsz)
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


    // expect good ret code

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);



}

// Test read fails on virt lun, prior to set size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_read_vlun_wo_setsz)
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
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

// Test read fails on read outside of phys lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_read_plun_outside_lunsz)
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
     

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 0;
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_READ;

    lba = temp_sz+1;
    nblks = 1;

    // verify read outside lun size fails

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    

    ASSERT_EQ(0,blk_fvt_setup(2));

   // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_READ;

    lba = 0;
    nblks = 2;

    // Verify read fails when block size greater than 1

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_NE(2 , ret);
    EXPECT_EQ(22 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks = 4096;
    size_t      lun_sz = 0;
    size_t      xfersz = 0;
    int         cmd;
    size_t      i = 0;

    TESTCASE_SKIP_IF_FILEMODE;

    ASSERT_EQ(0,blk_fvt_setup(nblks));

    // open physical  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);

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
           blk_open_tst_cleanup(0,  &ret,&er_no);
           EXPECT_EQ(0, ret);
           return;
        }

        cmd = FV_READ;
        lba = 0;
        // read what was wrote  xfersize
        blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags); 

        EXPECT_EQ(xfersz, (size_t)ret);
        if ((int)xfersz != ret) {
           fprintf(stderr,"Read failed xfersz 0x%lx\n",i);
           blk_open_tst_cleanup(0,  &ret,&er_no);
           EXPECT_EQ(0, ret);
           return;
        }
        // compare buffers
    
        blk_fvt_cmp_buf(xfersz, &ret);
        if (ret != 0) {
           fprintf(stderr,"Compare failed xfersz 0x%lx\n",i);
           blk_open_tst_cleanup(0,  &ret,&er_no);
           EXPECT_EQ(0, ret);
        }
        ASSERT_EQ(0, ret);
    }

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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

    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    if (lun_sz < nblks) {nblks = lun_sz;}

    xfersz = nblks;
    cmd = FV_WRITE;
    lba = 1;

    blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(xfersz, (size_t)ret);
    if ((int)xfersz != ret) {
       fprintf(stderr,"Write failed 1M xfersz \n");
       blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

       return;
    }

    cmd = FV_READ;
    lba = 1;
    // read what was wrote  xfersize
    blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(xfersz, (size_t)ret);
    if ((int)xfersz != ret) {
       fprintf(stderr,"Read failed 1M xfersz \n");
       blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

       return;
    }
    // compare buffers

    blk_fvt_cmp_buf(xfersz, &ret);
    if (ret != 0) {
       fprintf(stderr,"Compare failed 1M xfersz \n");
       blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    }
    ASSERT_EQ(0, ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}

// Run I/O with variable xfersize , use env_max_xfer if set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_plun_large_xfer_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int        ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
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

    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    if (lun_sz < nblks) {nblks = lun_sz;}

    xfersz = nblks;
    cmd = FV_WRITE;
    lba = 1;

    blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(xfersz, (size_t)ret);
    if ((int)xfersz != ret) {
       fprintf(stderr,"Write failed 1M+ xfersz \n");
       blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

       return;
    }

    cmd = FV_READ;
    lba = 1;
    // read what was wrote  xfersize
    blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags);

    EXPECT_EQ(xfersz, (size_t)ret);
    if ((int)xfersz != ret) {
       fprintf(stderr,"Read failed 1M+ xfersz \n");
       blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

       return;
    }
    // compare buffers

    blk_fvt_cmp_buf(xfersz, &ret);
    if (ret != 0) {
       fprintf(stderr,"Compare failed 1M+ xfersz \n");
       blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    }
    ASSERT_EQ(0, ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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

    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &lun_sz, sz_flags, get_set_flag, &ret,&er_no);
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
           blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

           return;
    }

    cmd = FV_READ;
    lba = 1;
    // read what was wrote  xfersize
    blk_fvt_io(id, cmd, lba, xfersz, &ret, &er_no, io_flags, open_flags); 

    EXPECT_TRUE(xfersz == (size_t)ret);
    if ((int)xfersz != ret) {
    fprintf(stderr,"Read failed 1M+ xfersz \n");
        blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        return;
    }
    // compare buffers
    
    blk_fvt_cmp_buf(xfersz, &ret);
    if (ret != 0) {
       fprintf(stderr,"Compare failed 1M+ xfersz \n");
    }
    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag;
    int         cmd; // 0 = read, 1, write                                     

   

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open Phys lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    temp_sz = 1;
    get_set_flag = 2;

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_READ;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}


// Test write succeed on phy lun, prior to set size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_write_plun_wo_setsz)
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);



}
// Test write fails on virt lun, prior to set size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_write_vlun_wo_setsz)
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
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}



// Test write fails on write outside of lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_write_vlun_outside_lunsz)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

// Test write fails on write outside of phys lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_write_plun_outside_lunsz)
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 0;
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_WRITE;

    lba = temp_sz+1;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        

    ASSERT_EQ(0,blk_fvt_setup(2));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


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
    int         get_set_flag;
    int         cmd; // 0 = read, 1, write                                     

   

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open Phys lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    temp_sz = 1;
    get_set_flag = 2;

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_WRITE;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}

// Verify async read issued prior to set size succeed on physical lun

TEST(Block_FVT_Suite, BLK_API_FVT_FM_async_read_plun_wo_setsz)
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
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

// Verify async read issued prior to set size fails on virtual lun

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_read_vlun_wo_setsz)
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
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AREAD;
    nblks = 1;
    lba = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

// Test async read fails on read outside of phys lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_read_plun_outside_lunsz)
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 0;
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AREAD;

    lba = temp_sz+1;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

// TODO shouldn't be expecting 0

    EXPECT_NE(0 , ret);
    EXPECT_EQ(22 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}
// Verify async read fails when tried to read more than 1 block size

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_read_vlun_greater_than_1_blk)
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        
    ASSERT_EQ(0,blk_fvt_setup(2));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

// Verify async write issued prior to set size succeed on physical lun

TEST(Block_FVT_Suite, BLK_API_FVT_FM__async_write_plun_wo_setsz)
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
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}


// Verify async write issued prior to set size fails on virtual lun

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_write_vlun_wo_setsz)
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
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

// Test async write fails on write outside of phys lun size 

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_async_write_plun_outside_lunsz)
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 0;
    get_set_flag = 1;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AWRITE;

    lba = temp_sz+1;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);

    EXPECT_NE(1 , ret);
    EXPECT_NE(0 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
                                        

    ASSERT_EQ(0,blk_fvt_setup(2));

    // open physocal lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 64;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AWRITE;

    lba = 0;
    nblks = 2;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_NE(2 , ret);
    EXPECT_EQ(22 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag;
    int         cmd; // 0 = read, 1, write                                     

   

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open Phys lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    temp_sz = 1;
    get_set_flag = 2;

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AWRITE;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag;
    int         cmd; // 0 = read, 1, write                                     

   

    ASSERT_EQ(0,blk_fvt_setup(1));

    // open Phys lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    temp_sz = 1;
    get_set_flag = 2;

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    cmd = FV_AREAD;

    lba = 0;
    nblks = 1;

    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags);


    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}

// Verify if CBLK_ARESULT_NEXT_TAG set,call doesn't returns till
// async req completes

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_aresult_blocking_next_tag)
{

    chunk_id_t  id           = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs     = MAX_NUM_CMDS;
    int         er_no        = 0;
    int         open_cnt     = 1;
    int         ret          = 0;
    int         sz_flags     = 0;
    size_t      temp_sz      = 1024;
    int         get_set_flag = 2;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    ASSERT_NE(-1, ret);

    blocking_io_tst(id, &ret,&er_no);
    EXPECT_EQ(1, ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

// Verify CBLK_ARESULT_USER_TAG
TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_aresult_user_tag)
{
    chunk_id_t id                = 0;
    int        open_flags        = CBLK_OPN_VIRT_LUN;
    int        max_reqs          = MAX_NUM_CMDS;
    int        er_no             = 0;
    int        open_cnt          = 1;
    int        ret               = 0;
    int        sz_flags          = 0;
    size_t     temp_sz           = 0;
    int        get_set_flag = 0;  // 0 = get phys lun sz
                                       // 1 = get chunk sz
                                       // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    ASSERT_NE(-1 , ret);

    user_tag_io_tst ( id,  &ret, &er_no, 0);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}

// Verify CBLK_ARESULT_USER_TAG with CBLK_OPN_NO_INTRP_THREADS;
TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_aresult_user_tag_no_intrp)
{
    chunk_id_t id                = 0;
    int        open_flags        = CBLK_OPN_VIRT_LUN;
    int        max_reqs          = MAX_NUM_CMDS;
    int        er_no             = 0;
    int        open_cnt          = 1;
    int        ret               = 0;
    int        sz_flags          = 0;
    size_t     temp_sz           = 0;
    int        get_set_flag = 0;  // 0 = get phys lun sz
                                       // 1 = get chunk sz
                                       // 2 = set chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));

    open_flags |= CBLK_OPN_NO_INTRP_THREADS;

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag,
                             &ret,&er_no);
    ASSERT_NE(-1 , ret);

    user_tag_io_tst ( id,  &ret, &er_no, 0);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_vlun_perf_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         sz_flags= 0;
    size_t      temp_sz;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    
    int num_cmds = MAX_NUM_CMDS;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 10000;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    ASSERT_NE(-1 , ret);

    io_perf_tst(&ret, &er_no, 0, 0, 0);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_vlun_unmap_perf_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         sz_flags= 0;
    size_t      temp_sz;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    int num_cmds = MAX_NUM_CMDS;
    chunk_attrs_t attrs;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    bzero(&attrs, sizeof(attrs));
    ret = cblk_get_attrs(id, &attrs, 0);
    ASSERT_EQ(ret,0);

    if (attrs.flags1 & CFLSH_ATTR_UNMAP)
    {
        temp_sz = 10000;
        get_set_flag = 2;
        blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
        ASSERT_NE(-1 , ret);

        io_perf_tst(&ret, &er_no, TST_UNMAP, 0, 0);
        EXPECT_EQ(0 , ret);
    }
    else
    {
        TESTCASE_SKIP("unmap is not supported");
    }

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_plun_unmap_perf_test)
{
    chunk_id_t  id         = 0;
    int         open_flags = 0;
    int         max_reqs   = MAX_NUM_CMDS;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    int         num_cmds   = MAX_NUM_CMDS;
    chunk_attrs_t attrs;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds));

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    bzero(&attrs, sizeof(attrs));
    ret = cblk_get_attrs(id, &attrs, 0);
    ASSERT_EQ(ret,0);

    if (attrs.flags1 & CFLSH_ATTR_UNMAP)
    {
        io_perf_tst(&ret, &er_no, TST_UNMAP, 0, 0);
        EXPECT_EQ(0 , ret);
    }
    else
    {
        TESTCASE_SKIP("unmap is not supported");
    }

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_vlun_polling_unmap_rw_tst)
{
    int         open_flags = CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;
    chunk_id_t  id       = 0;
    int         flags    = 0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;
    chunk_attrs_t attrs;

    ASSERT_EQ(0,blk_fvt_setup(1));
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    bzero(&attrs, sizeof(attrs));
    ret = cblk_get_attrs(id, &attrs, 0);
    ASSERT_EQ(ret,0);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    ASSERT_EQ(0, ret);

    if (attrs.flags1 & CFLSH_ATTR_UNMAP)
    {
        num_loops   = 1;
        num_threads = 40;

        blk_thread_tst(&ret, &er_no, open_flags, TST_UNMAP, 0, 0);
        EXPECT_EQ(0, errno);
        EXPECT_EQ(0, ret);
    }
    else
    {
        TESTCASE_SKIP("unmap is not supported");
    }
}

TEST(Block_FVT_Suite, BLK_API_FVT_multi_vlun_blocking_perf_test)
{
    chunk_id_t  id          = 0;
    int         open_flags  = CBLK_OPN_VIRT_LUN;
    int         resflags    = CBLK_ARESULT_BLOCKING;
    int         num_cmds    = MAX_NUM_CMDS;
    int         er_no       = 0;
    int         ret         = 0;
    int         i           = 0;
    int         sz_flags    = 0;
    size_t      psize       = 0;
    size_t      set_size    = 5000;
    int         MULTI_VLUNS = 128;
    int         vluns_in    = 0;
    char       *env_vluns   = getenv("FVT_MULTI_VLUNS");
    char       *env_filemode = getenv("BLOCK_FILEMODE_ENABLED");

    if (env_vluns)
    {
        vluns_in = atoi(env_vluns);
        if (vluns_in > 0 && vluns_in < 256) {MULTI_VLUNS=vluns_in;}
    }
    if (env_filemode && atoi(env_filemode)==1) {set_size=1024; MULTI_VLUNS=32;}

    ASSERT_EQ(0,blk_fvt_setup(MULTI_VLUNS*num_cmds));

    blk_open_tst(&id, num_cmds, &er_no, MULTI_VLUNS, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    blk_fvt_get_set_lun_size(id, &psize, 0, 0, &ret, &er_no);
    ASSERT_NE(-1 , ret);
    set_size = (psize/(MULTI_VLUNS*2));

    for (i=0; i<MULTI_VLUNS; i++)
    {
        blk_fvt_get_set_lun_size(chunks[i], &set_size, sz_flags, 2, &ret, &er_no);
        ASSERT_NE(-1 , ret);
    }

    io_perf_tst(&ret, &er_no, 0, resflags, 0);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

TEST(Block_FVT_Suite, BLK_API_FVT_multi_vlun_polling_perf_test)
{
    chunk_id_t  id          = 0;
    int         open_flags  = CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;
    int         num_cmds    = MAX_NUM_CMDS;
    int         er_no       = 0;
    int         ret         = 0;
    int         i           = 0;
    int         sz_flags    = 0;
    size_t      psize       = 0;
    size_t      set_size    = 5000;
    int         MULTI_VLUNS = 128;
    int         vluns_in    = 0;
    char       *env_vluns   = getenv("FVT_MULTI_VLUNS");
    char       *env_filemode = getenv("BLOCK_FILEMODE_ENABLED");

    if (env_vluns)
    {
        vluns_in = atoi(env_vluns);
        if (vluns_in > 0 && vluns_in < 256) {MULTI_VLUNS=vluns_in;}
    }
    if (env_filemode && atoi(env_filemode)==1) {set_size=1024; MULTI_VLUNS=32;}

    ASSERT_EQ(0,blk_fvt_setup(MULTI_VLUNS*num_cmds));

    blk_open_tst(&id, num_cmds, &er_no, MULTI_VLUNS, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    blk_fvt_get_set_lun_size(id, &psize, 0, 0, &ret, &er_no);
    ASSERT_NE(-1 , ret);
    set_size = (psize/(MULTI_VLUNS*2));

    for (i=0; i<MULTI_VLUNS; i++)
    {
        blk_fvt_get_set_lun_size(chunks[i], &set_size, sz_flags, 2, &ret, &er_no);
        ASSERT_NE(-1 , ret);
    }

    io_perf_tst(&ret, &er_no, 0, 0, 0);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_plun_polling_unmap_rw_tst)
{
    int         open_flags = CBLK_OPN_NO_INTRP_THREADS;
    chunk_id_t  id       = 0;
    int         flags    = 0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;
    chunk_attrs_t attrs;

    ASSERT_EQ(0,blk_fvt_setup(1));
    if (env_filemode && atoi(env_filemode)==1) {ext=1;}

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    bzero(&attrs, sizeof(attrs));
    ret = cblk_get_attrs(id, &attrs, 0);
    ASSERT_EQ(ret,0);

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);

    if (attrs.flags1 & CFLSH_ATTR_UNMAP)
    {
        num_loops   = 1;
        num_threads = 40;

        blk_thread_tst(&ret, &er_no, open_flags, TST_UNMAP, 0, 0);
        EXPECT_EQ(0, ret);
    }
    else
    {
        TESTCASE_SKIP("unmap is not supported");
    }
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_plun_unmap_BIG_BLOCKS)
{
    int         open_flags=0;
    chunk_id_t  id       = 0;
    int         flags    = 0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;
    size_t      temp_sz  = 64*1024;  // 16 unmap cmds, 256mb
    int         blks     = 4096;
    chunk_attrs_t attrs;

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    bzero(&attrs, sizeof(attrs));
    ret = cblk_get_attrs(id, &attrs, 0);
    ASSERT_EQ(ret,0);

    if (attrs.flags1 & CFLSH_ATTR_UNMAP)
    {
        char    *buf       = NULL;
        int      rc        = 0;
        int      tag       = -1;
        uint64_t status    = 0;

        if ((rc=posix_memalign((void**)&buf, 128, 4096*temp_sz)))
        {
            EXPECT_EQ(rc,0);
            goto exit;
        }

        rc = cblk_write(id, buf, 0, blks, 0);
        ASSERT_EQ(blks, rc);
        rc = cblk_unmap(id, buf, 0, blks, 0);
        ASSERT_EQ(blks, rc);

        rc = cblk_awrite(id, buf, 0, blks, &tag, NULL, CBLK_ARW_WAIT_CMD_FLAGS);
        ASSERT_EQ(0, rc);
        rc=cblk_aresult(id, &tag, &status, CBLK_ARESULT_BLOCKING);
        ASSERT_EQ(blks, rc);
        ASSERT_EQ(0, errno);

        rc = cblk_aunmap(id, buf, 0, blks, &tag, NULL, CBLK_ARW_WAIT_CMD_FLAGS);
        ASSERT_EQ(0, rc);
        rc=cblk_aresult(id, &tag, &status, CBLK_ARESULT_BLOCKING);
        ASSERT_EQ(blks, rc);
        ASSERT_EQ(0, errno);
        free(buf);
    }
    else
    {
        TESTCASE_SKIP("unmap is not supported");
    }

exit:
    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_vlun_unmap_scrub_small)
{
    int         open_flags = CBLK_OPN_VIRT_LUN | CBLK_OPN_SCRUB_DATA;
    chunk_id_t  id       = 0;
    int         flags    = 0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;
    size_t      temp_sz  = 5000;
    chunk_attrs_t attrs;

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    bzero(&attrs, sizeof(attrs));
    ret = cblk_get_attrs(id, &attrs, 0);
    ASSERT_EQ(ret,0);

    blk_fvt_get_set_lun_size(id, &temp_sz, 0, 2, &ret,&er_no);
    EXPECT_EQ(0, ret);

    if (attrs.flags1 & CFLSH_ATTR_UNMAP)
    {
        char     buf[4096] = {0};
        int      rc        = 0;
        int      tag       = -1;
        uint64_t status    = 0;

        rc = cblk_write(id, buf, 0, 1, 0);
        ASSERT_EQ(1, rc);
        rc = cblk_unmap(id, buf, 0, 1, 0);
        ASSERT_EQ(1, rc);

        rc = cblk_awrite(id, buf, 0, 1, &tag, NULL, CBLK_ARW_WAIT_CMD_FLAGS);
        ASSERT_EQ(0, rc);
        rc=cblk_aresult(id, &tag, &status, CBLK_ARESULT_BLOCKING);
        ASSERT_EQ(1, rc);
        ASSERT_EQ(0, errno);

        rc = cblk_aunmap(id, buf, 0, 1, &tag, NULL, CBLK_ARW_WAIT_CMD_FLAGS);
        ASSERT_EQ(0, rc);
        rc=cblk_aresult(id, &tag, &status, CBLK_ARESULT_BLOCKING);
        ASSERT_EQ(1, rc);
        ASSERT_EQ(0, errno);
    }
    else
    {
        TESTCASE_SKIP("unmap is not supported");
    }

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_vlun_unmap_scrub_big)
{
    int         open_flags = CBLK_OPN_VIRT_LUN | CBLK_OPN_SCRUB_DATA;
    chunk_id_t  id       = 0;
    int         flags    = 0;
    int         max_reqs = 64;
    int         er_no    = 0;
    int         open_cnt = 1;
    int         ret      = 0;
    size_t      temp_sz  = 200000;
    chunk_attrs_t attrs;

    ASSERT_EQ(0,blk_fvt_setup(1));

    blk_open_tst(&id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID, id);

    bzero(&attrs, sizeof(attrs));
    ret = cblk_get_attrs(id, &attrs, 0);
    ASSERT_EQ(ret,0);

    blk_fvt_get_set_lun_size(id, &temp_sz, 0, 2, &ret,&er_no);
    EXPECT_EQ(0, ret);

    if (attrs.flags1 & CFLSH_ATTR_UNMAP)
    {
        char     buf[4096] = {0};
        int      rc        = 0;
        int      tag       = -1;
        uint64_t status    = 0;

        rc = cblk_write(id, buf, 0, 1, 0);
        ASSERT_EQ(1, rc);
        rc = cblk_unmap(id, buf, 0, 1, 0);
        ASSERT_EQ(1, rc);

        rc = cblk_awrite(id, buf, 0, 1, &tag, NULL, CBLK_ARW_WAIT_CMD_FLAGS);
        ASSERT_EQ(0, rc);
        rc=cblk_aresult(id, &tag, &status, CBLK_ARESULT_BLOCKING);
        ASSERT_EQ(1, rc);
        ASSERT_EQ(0, errno);

        rc = cblk_aunmap(id, buf, 0, 1, &tag, NULL, CBLK_ARW_WAIT_CMD_FLAGS);
        ASSERT_EQ(0, rc);
        rc=cblk_aresult(id, &tag, &status, CBLK_ARESULT_BLOCKING);
        ASSERT_EQ(1, rc);
        ASSERT_EQ(0, errno);
    }
    else
    {
        TESTCASE_SKIP("unmap is not supported");
    }

    blk_open_tst_cleanup(flags, &ret,&er_no);
    EXPECT_EQ(0, ret);
}

TEST(Block_FVT_Suite, BLK_API_FVT_plun_blocking_perf_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         sz_flags= 0;
    size_t      temp_sz;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    int num_cmds = MAX_NUM_CMDS;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    ASSERT_NE(-1 , ret);

    /* Test needs atleast 10000 blksz lun */
    if (temp_sz < 10000 )
    {
        TESTCASE_SKIP("Lun size less than then 10000 blks");
        blk_open_tst_cleanup(0,  &ret,&er_no);
        EXPECT_EQ(0, ret);
        return;
    }

    io_perf_tst(&ret, &er_no, 0, CBLK_ARESULT_BLOCKING, 0);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

TEST(Block_FVT_Suite, BLK_API_FVT_plun_polling_perf_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_NO_INTRP_THREADS;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         sz_flags= 0;
    size_t      temp_sz;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    int num_cmds = MAX_NUM_CMDS;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    ASSERT_NE(-1 , ret);

    /* Test needs atleast 10000 blksz lun */
    if (temp_sz < 10000 )
    {
        TESTCASE_SKIP("Lun size less than then 10000 blks");
        blk_open_tst_cleanup(0,  &ret,&er_no);
        EXPECT_EQ(0, ret);
        return;
    }

    io_perf_tst(&ret, &er_no, 0, 0, 0);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

#ifndef _AIX

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_clone_chunk)
{
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         io_flags     = 0;
    int         er_no   = 0;
    int         ret = 0;
    int         rc = 0;

    rc = fork_and_clone (&ret, &er_no, mode, open_flags, io_flags);
    ASSERT_NE(-1,  rc );
    EXPECT_EQ(1, ret);
    EXPECT_EQ(0, er_no);
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}

TEST(Block_FVT_Suite, BLK_API_FVT_fork_clone_chunk_RDONLY_mode_test)
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
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_fork_clone_chunk_RDONLY_mode_errpath_1)
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
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_fork_clone_chunk_RDONLY_mode_errpath_2)
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
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_fork_clone_chunk_WRONLY_mode_test)
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
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_fork_clone_chunk_WRONLY_mode_errpath_1)
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
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_fork_clone_chunk_WRONLY_mode_errpath_2)
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
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_fork_clone_chunk_RDWR_mode_test)
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
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_fork_clone_chunk_RDWR_mode_errpath_1)
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
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_fork_clone_chunk_RDWR_mode_errpath_2)
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
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);


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
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    ASSERT_NE(-1 , ret);

    ret = cblk_aresult(id,&tag,&status,0);

    EXPECT_EQ(-1 , ret);
    EXPECT_EQ(EINVAL, errno);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}

TEST(Block_FVT_Suite, BLK_API_FVT_vlun_share_cntxt_rw_tst)
{
    int         flags = CBLK_OPN_VIRT_LUN | CBLK_OPN_SHARE_CTXT;
    int         er_no = 0;
    int         ret   = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ret = validate_share_context();
    if (ret) {
           TESTCASE_SKIP("context _not_ sharable");
           if (blk_fvt_data_buf != NULL)
           {
               free(blk_fvt_data_buf);
               blk_fvt_data_buf = NULL;
           }
           if (blk_fvt_comp_data_buf != NULL)
           {
               free(blk_fvt_comp_data_buf);
               blk_fvt_comp_data_buf = NULL;
           }
           return;
    }
    num_loops   = 2;
    num_threads = 10;
    thread_flag = 1;

    blk_thread_tst(&ret, &er_no, flags, 0, 0, 0);
    EXPECT_EQ(0, ret);
}

TEST(Block_FVT_Suite, BLK_API_FVT_plun_share_cntxt_rw_tst)
{
    int         flags = CBLK_OPN_SHARE_CTXT;
    int         er_no = 0;
    int         ret   = 0;

    ASSERT_EQ(0,blk_fvt_setup(1));
    ret = validate_share_context();
    if (ret) {
           TESTCASE_SKIP("context _not_ sharable");
           if (blk_fvt_data_buf != NULL)
           {
               free(blk_fvt_data_buf);
               blk_fvt_data_buf = NULL;
           }
           if (blk_fvt_comp_data_buf != NULL)
           {
               free(blk_fvt_comp_data_buf);
               blk_fvt_comp_data_buf = NULL;
           }
           return;
    }
    num_loops   = 2;
    num_threads = 10;
    thread_flag = 1;
    blk_thread_tst(&ret, &er_no, flags, 0, 0, 0);
    EXPECT_EQ(0, ret);

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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);

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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

}

// Test NO_INTRP async io tests

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_vlun_NO_INTRP_THREAD_SET_async_io)
{


    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    nblks = 1;
    lba = 1;
    cmd = FV_AWRITE;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    // If errors no reason to continue 
    if (ret != 1) {
        blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        return;
    }
    cmd = FV_AREAD;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    // If errors no reason to continue 
    if (ret != 1) {
        blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        return;
    }
    // compare buffers
    
    blk_fvt_cmp_buf(nblks, &ret);

    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

// Test NO_INTRP synchronous io tests

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_vlun_NO_INTRP_THREAD_SET_sync_io)
{


    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    nblks = 1;
    lba = 1;
    cmd = FV_WRITE;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    // If errors no reason to continue 
    if (ret != 1) {
        blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        return;
    }
    cmd = FV_READ;
    blk_fvt_io(id, cmd, lba, nblks, &ret, &er_no, io_flags, open_flags); 

    EXPECT_EQ(1 , ret);

    // If errors no reason to continue 
    if (ret != 1) {
        blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        return;
    }
    // compare buffers
    
    blk_fvt_cmp_buf(nblks, &ret);

    EXPECT_EQ(0, ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret,&er_no);

    EXPECT_EQ(-1 , ret);
    EXPECT_NE(0 , er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret,&er_no);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0  , er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret,&er_no);

    EXPECT_EQ(1 , ret);
    EXPECT_EQ(0 , er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret,&er_no);

    EXPECT_EQ(-1 , ret);
    EXPECT_NE( 0 , er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret,&er_no);

    EXPECT_EQ(-1 , ret);
    EXPECT_NE( 0, er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    blk_fvt_intrp_io_tst(id, testflag, open_flags, &ret,&er_no);

    EXPECT_EQ(-1 , ret);
    EXPECT_NE( 0, er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_list_io_args_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         i;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int		arg_tst = 0;


    ASSERT_EQ(0,blk_fvt_setup(1));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 16;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

// Test listio on virt lun, without Timout, and with CBLK_IO_USER_STATUS flag set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_vlun_list_io_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 1;
    uint64_t    timeout = 0;
    int		uflags = CBLK_IO_USER_STATUS;
    int         cmd ;
    int         i=0;

    /* Number of 4K bufs */
    io_bufcnt = MAX_NUM_CMDS;


    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }


    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret,num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }


    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

// Test listio on virt lun, without Timout, and with CBLK_IO_USER_STATUS flag set and NO_INTRP flag set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_vlun_list_io_test_no_intrp)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;
    int         sz_flags= 0;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    io_bufcnt = MAX_NUM_CMDS;


    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }


    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret,num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }


    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

// Test listio on physical lun, without Timout, and with CBLK_IO_USER_STATUS flag set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_plun_list_io_test)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 1;
    uint64_t    timeout = 0;
    int		uflags = CBLK_IO_USER_STATUS;
    int         cmd ;

    /* Number of 4K bufs */
    io_bufcnt = MAX_NUM_CMDS;

    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_flag = 0;
    // get phys lun size
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }
    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }

    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }

    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE*num_listio);
    er_no = errno;

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

// Test listio on physical lun, without Timout, and with CBLK_IO_USER_STATUS flag set , NO_INTRP set

TEST(Block_FVT_Suite, BLK_API_FVT_FM_plun_list_io_test_no_intrp)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_NO_INTRP_THREADS;
    int         sz_flags= 0;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
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
    io_bufcnt = MAX_NUM_CMDS;

    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_flag = 0;
    // get phys lun size
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }
    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }

    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }

    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE*num_listio);
    er_no = errno;

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}


// Test listio on virtual lun without user status flag , no timeout set 
// Passes issue_list, wait_list and complete_list and then polls wait list for 
// completions

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_vlun_list_io_test_1)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 2;
    uint64_t    timeout = 0;
    int		uflags = 0;
    int         cmd ;


    /* Number of 4K bufs */
    io_bufcnt = MAX_NUM_CMDS;

    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }

    cmd = FV_WRITE;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }


    cmd = FV_READ;
    blk_list_io_test(id, cmd, t_type, uflags, timeout, &er_no, &ret, num_listio);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

        // If errors no reason to continue 
        if ((ret != 0) || (er_no != 0)) {
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
        }

    ret = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE*num_listio);
    er_no = errno;

    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

// Test listio on virt lun, without timeout and without CBLK_USER_IO_STATUS
// First listio issued with issue_list and complete_list
// then completions verfied by keep issuing listio with updated pending_list and
// completion_list , till all commands comeback with either success or failure.

TEST(Block_FVT_Suite, BLK_API_FVT_FM_UMC_vlun_list_io_test_2)
{

    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 3;
    uint64_t    timeout = 0;
    int		uflags = 0;
    int         cmd ;

    /* Number of 4K bufs */
    io_bufcnt = MAX_NUM_CMDS;

    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 1024;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

// Test listio on physical lun without user status flag , no timeout set 
// Passes issue_list, wait_list and complete_list and then polls wait list for 
// completions

TEST(Block_FVT_Suite, BLK_API_FVT_FM_plun_list_io_test_1)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 2;
    uint64_t    timeout = 0;
    int		uflags = 0;
    int         cmd ;
    /* Number of 4K bufs */
    io_bufcnt = MAX_NUM_CMDS;

    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );


    temp_sz = 0;
    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    if ((temp_sz < 1024) || ret || er_no ){
        blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

// Test listio on Physical lun, without timeout and without CBLK_USER_IO_STATUS
// First listio issued with issue_list and complete_list
// then completions verfied by keep issuing listio with updated pending_list and
// completion_list , till all commands comeback with either success or failure.

TEST(Block_FVT_Suite, BLK_API_FVT_FM_plun_list_io_test_2)
{

    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         sz_flags= 0;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    size_t      temp_sz;
    int         t_type = 3;
    uint64_t    timeout = 0;
    int		uflags = 0;
    int         cmd ;


    /* Number of 4K bufs */
    io_bufcnt = MAX_NUM_CMDS;
    ASSERT_EQ(0,blk_fvt_setup(num_listio));

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
   
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 0;
    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);

    if (temp_sz < 1024) {
        blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

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

        blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

// Verifies data perisists across closes on physical luns

TEST(Block_FVT_Suite, BLK_API_FVT_FM_Persistence_plun_test)
{
    chunk_id_t  id      = 0;
    int         open_flags   = 0;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;

    uint64_t    lba;
    int         io_flags = 0;
    size_t      nblks;
    int         cmd;

    // Setup dev name and allocated test buffers
    ASSERT_EQ(0,blk_fvt_setup(MAX_NUM_CMDS));

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


    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
  

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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

    // Setup dev name and allocated test buffers
    
    ASSERT_EQ(0,blk_fvt_setup(1));

    open_flags |= (CBLK_OPN_RESERVE|CBLK_OPN_MPIO_FO);

    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);

    EXPECT_EQ (NULL_CHUNK_ID,  id );
    EXPECT_NE (0 , er_no);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
        blk_open_tst_cleanup(0,  &ret,&er_no);
        EXPECT_EQ(0, ret);
        EXPECT_EQ(0, er_no);
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

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
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
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         sz_flags= 0;
    size_t      temp_sz;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    
    int num_cmds = MAX_NUM_CMDS;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds+1));

    open_flags |= CBLK_OPN_MPIO_FO;


    // open physical lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    get_set_flag = 0;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    ASSERT_NE(-1 , ret);

    /* Test needs atleast 10000 blksz lun */
    if (temp_sz < 10000 ) {
            TESTCASE_SKIP("Lun size less than then 10000 blks");
            blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);

            return;
    }

    io_perf_tst(&ret, &er_no, 0, 0, 0);

    EXPECT_EQ(0 , ret);
    EXPECT_EQ(0 , er_no);
  
    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(0, er_no);
}

#else

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_FM_vlun_async_blocking_EEH)
{
    chunk_id_t id           = 0;
    int        open_flags   = CBLK_OPN_VIRT_LUN;
    int        max_reqs     = MAX_NUM_CMDS;
    int        er_no        = 0;
    int        open_cnt     = 1;
    int        ret          = 0;
    int        sz_flags     = 0;
    size_t     temp_sz      = 0;
    int        get_set_flag = 0;  // 0 = get phys lun sz
                                  // 1 = get chunk sz
                                  // 2 = set chunk sz
    int num_cmds = MAX_NUM_CMDS;

    TESTCASE_SKIP_IF_NO_EEH;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 10000;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    ASSERT_NE(-1 , ret);

    io_perf_tst(&ret, &er_no, 0, CBLK_ARESULT_BLOCKING, 1);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_FM_plun_async_blocking_EEH)
{
    chunk_id_t  id         = 0;
    int         open_flags = 0;
    int         max_reqs   = MAX_NUM_CMDS;
    int         er_no      = 0;
    int         open_cnt   = 1;
    int         ret        = 0;
    int         num_cmds   = MAX_NUM_CMDS;

    TESTCASE_SKIP_IF_NO_EEH;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    io_perf_tst(&ret, &er_no, 0, CBLK_ARESULT_BLOCKING, 1);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_FM_vlun_sync_EEH)
{
    int         flags = CBLK_OPN_VIRT_LUN;
    int         er_no = 0;
    int         ret   = 0;

    TESTCASE_SKIP_IF_NO_EEH;

    ASSERT_EQ(0,blk_fvt_setup(1));
    num_loops   = 2;
    num_threads = 40;
    blk_thread_tst(&ret, &er_no, flags, TST_SYNC_IO_ONLY, 0, 1);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_FM_plun_sync_EEH)
{
    int         flags = 0;
    int         er_no = 0;
    int         ret   = 0;

    TESTCASE_SKIP_IF_NO_EEH;

    ASSERT_EQ(0,blk_fvt_setup(1));
    num_loops   = 2;
    num_threads = 40;
    blk_thread_tst(&ret, &er_no, flags, TST_SYNC_IO_ONLY, 0, 1);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_multi_vlun_async_blocking_EEH)
{
    chunk_id_t  id          = 0;
    int         open_flags  = CBLK_OPN_VIRT_LUN;
    int         num_cmds    = MAX_NUM_CMDS;
    int         er_no       = 0;
    int         ret         = 0;
    int         i           = 0;
    int         sz_flags    = 0;
    size_t      set_size    = 5000;
    int         MULTI_VLUNS = 128;
    int         vluns_in    = 0;
    char       *env_vluns   = getenv("FVT_MULTI_VLUNS");

    TESTCASE_SKIP_IF_NO_EEH;

    if (env_vluns)
    {
        vluns_in = atoi(env_vluns);
        if (vluns_in > 0 && vluns_in < 128) {MULTI_VLUNS=vluns_in;}
    }

    ASSERT_EQ(0,blk_fvt_setup(num_cmds*MULTI_VLUNS));

    blk_open_tst(&id, num_cmds, &er_no, MULTI_VLUNS, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    for (i=0; i<MULTI_VLUNS; i++)
    {
        blk_fvt_get_set_lun_size(chunks[i], &set_size, sz_flags, 2, &ret,&er_no);
        ASSERT_NE(-1 , ret);
    }

    io_perf_tst(&ret, &er_no, 0, CBLK_ARESULT_BLOCKING, 1);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}


/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_FM_vlun_async_polling_EEH)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int         sz_flags= 0;
    size_t      temp_sz;
    int         get_set_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz
    int num_cmds = MAX_NUM_CMDS;

    TESTCASE_SKIP_IF_NO_EEH;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    temp_sz = 10000;
    get_set_flag = 2;
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_flag, &ret,&er_no);
    ASSERT_NE(-1 , ret);

    io_perf_tst(&ret, &er_no, 0, 0, 1);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_multi_vlun_async_polling_EEH)
{
    chunk_id_t  id          = 0;
    int         open_flags  = CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;
    int         num_cmds    = MAX_NUM_CMDS;
    int         er_no       = 0;
    int         ret         = 0;
    int         i           = 0;
    int         sz_flags    = 0;
    size_t      set_size    = 5000;
    int         MULTI_VLUNS = 128;
    int         vluns_in    = 0;
    char       *env_vluns   = getenv("FVT_MULTI_VLUNS");

    TESTCASE_SKIP_IF_NO_EEH;

    if (env_vluns)
    {
        vluns_in = atoi(env_vluns);
        if (vluns_in > 0 && vluns_in < 128) {MULTI_VLUNS=vluns_in;}
    }

    ASSERT_EQ(0,blk_fvt_setup(num_cmds*MULTI_VLUNS));

    blk_open_tst(&id, num_cmds, &er_no, MULTI_VLUNS, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    for (i=0; i<MULTI_VLUNS; i++)
    {
        blk_fvt_get_set_lun_size(chunks[i], &set_size, sz_flags, 2, &ret,&er_no);
        ASSERT_NE(-1 , ret);
    }

    io_perf_tst(&ret, &er_no, 0, 0, 1);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_FM_plun_async_polling_EEH)
{
    chunk_id_t  id      = 0;
    int         open_flags   = CBLK_OPN_NO_INTRP_THREADS;
    int         max_reqs= MAX_NUM_CMDS;
    int         er_no   = 0;
    int         open_cnt= 1;
    int         ret = 0;
    int num_cmds = MAX_NUM_CMDS;

    TESTCASE_SKIP_IF_NO_EEH;

    ASSERT_EQ(0,blk_fvt_setup(num_cmds));

    // open virt lun
    blk_open_tst( &id, max_reqs, &er_no, open_cnt, open_flags, mode);
    ASSERT_NE(NULL_CHUNK_ID,  id );

    io_perf_tst(&ret, &er_no, 0, 0, 1);
    EXPECT_EQ(0 , ret);

    blk_open_tst_cleanup(0,  &ret,&er_no);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_FM_vlun_sync_polling_EEH)
{
    int         flags = CBLK_OPN_VIRT_LUN | CBLK_OPN_NO_INTRP_THREADS;
    int         er_no = 0;
    int         ret   = 0;

    TESTCASE_SKIP_IF_NO_EEH;

    ASSERT_EQ(0,blk_fvt_setup(1));
    num_loops   = 2;
    num_threads = 40;
    thread_flag = 1;
    blk_thread_tst(&ret, &er_no, flags, TST_SYNC_IO_ONLY, 0, 1);
    EXPECT_EQ(0, ret);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
TEST(Block_FVT_Suite, BLK_API_FVT_FM_plun_sync_polling_EEH)
{
    int         flags = CBLK_OPN_NO_INTRP_THREADS;
    int         er_no = 0;
    int         ret   = 0;

    TESTCASE_SKIP_IF_NO_EEH;

    ASSERT_EQ(0,blk_fvt_setup(1));
    num_loops   = 2;
    num_threads = 40;
    thread_flag = 1;
    blk_thread_tst(&ret, &er_no, flags, TST_SYNC_IO_ONLY, 0, 1);
    EXPECT_EQ(0, ret);
}

#endif /* _AIX */
