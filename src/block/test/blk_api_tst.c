/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/block/test/blk_api_tst.c $                                */
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
#include <blk_tst.h>
#include <cflash_test_utils.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/resource.h>

#ifndef _AIX
#include <libudev.h>
#endif

#define CHUNK_SIZE 4096
#define NBUFS      1000

#ifdef _AIX
#define GEN_VAL_MOD for (_i=7; _i>=8-_mod; _i--) {*(_t++)=_s[_i];}
#else
#define GEN_VAL_MOD for (_i=0; _i<_mod; _i++) {*(_t++)=_s[_i];}
#endif

/* generate pattern based upon _num into _val */
/* _num can be up to 64-bit value             */
#define GEN_VAL(_val,_num,_len)                                                \
  do                                                                           \
  {                                                                            \
      volatile int64_t   _i   = 0;                                             \
      volatile uint64_t  _d   = (uint64_t)(_num);                              \
      volatile uint8_t  *_s   = (uint8_t*)&_d;                                 \
      volatile uint8_t  *_t   = (uint8_t*)(_val);                              \
      volatile int64_t   _div = (_len)/8;                                      \
      volatile int64_t   _mod = (_len)%8;                                      \
      volatile uint64_t *_p   = (uint64_t*)(_t+_mod);                          \
      GEN_VAL_MOD                                                              \
      for (_i=0; _i<_div; _i++) {*(_p++)=_d;}                                  \
  } while (0)

char test_filename[256];
pthread_t blk_thread[MAX_NUM_THREADS];

pthread_mutex_t  completion_lock;

extern int mode;
extern int num_opens;
extern uint32_t thread_count;
extern uint64_t block_number;
extern int num_loops;
extern int thread_flag;
extern int  num_threads;
extern uint64_t  max_xfer;;
extern int virt_lun_flags;
extern int share_cntxt_flags;
extern int  test_max_cntx;
extern int  host_type;
extern char* env_filemode;
extern char* env_max_xfer;
extern char* env_num_cntx;
extern char* env_num_list;
extern int io_bufcnt;
extern int num_listio;

char def_capidev[50] = "/dev/cxl/afu0.0s";

char dev_paths[MAX_LUNS][128];

chunk_ext_arg_t ext=1;

char *env_blk_verbosity = NULL;

char *env_num_list = NULL;


char *parents [MAX_LUNS] ;

int  blk_verbosity = 0;
int  num_devs = 0;
int  filemode = 0;

blk_thread_data_t blk_data;

char *paths;
char tpaths[512];

void initialize_blk_tests()
{
    int rc =0;
    rc = cblk_init(NULL,0);
    if (rc){
        fprintf(stderr, "cblk_term: failed . errno =%d\n", errno);
        return;
    }
    io_bufcnt = 4;

}

void teminate_blk_tests()
{
    int rc =0;
    rc = cblk_term(NULL,0);
    if (rc){
        fprintf(stderr, "cblk_term: failed . errno =%d\n", errno);
        return;
    }
}

/**
 *******************************************************************************
 * \brief
 * if FVT_DEV_RAID0 is set, use it
 * else if FVT_DEV is set and BLOCK_FILEMODE_ENABLED==1, use FVT_DEV
 * else if cflash_devices finds GT superpipe luns, use "RAID0"
 * else FVT_DEV is used
 ******************************************************************************/
void blk_fvt_RAID0_setup(void)
{
    char  buf[1024]    = {0};
    char *env_dev      = getenv("FVT_DEV");
    char *env_r0dev    = getenv("FVT_DEV_RAID0");
    char *env_filemode = getenv("BLOCK_FILEMODE_ENABLED");

    if (env_r0dev && strlen(env_r0dev)>=5)
    {
        DEBUG_0("RAID0_setup: use FVT_DEV_RAID0\n");
        strcpy(dev_paths[0],env_r0dev);
    }
    else if (env_dev && env_filemode && atoi(env_filemode)==1)
    {
        DEBUG_0("RAID0_setup: use FVT_DEV\n");
        strcpy(dev_paths[0],env_dev);
    }
    else
    {
#ifdef _AIX
        FILE *fp = popen("cflash_devices -S 2>/dev/null", "r");
#else
        FILE *fp = popen("/usr/bin/cflash_devices -S 2>/dev/null",
                         "r");
#endif
        if (fp) {fgets(buf, 1024, fp);}
        if (strlen(buf))
        {
            DEBUG_1("buf: %s\n", buf);
            DEBUG_0("RAID0_setup: use RAID0\n");
            strcpy(dev_paths[0],"RAID0");
        }
    }
    DEBUG_2("RAID0_setup: dev_paths[0]=%s num_devs:%d\n",
            dev_paths[0], num_devs);
    return;
}

int blk_fvt_setup(int size)
{
    char *env_user = getenv("USER");
    int  fd;
    char *p;
    int i,ret;
    FILE *fp=NULL;
    char platform[1024];

    paths = getenv("FVT_DEV");

    env_num_cntx  = getenv("MAX_CNTX");
    if (env_num_cntx && (atoi(env_num_cntx) > 0))
        test_max_cntx = atoi(env_num_cntx);

    env_blk_verbosity  = getenv("FVT_BLK_VERBOSITY");

    env_num_list = getenv ("FVT_NUM_LISTIO");

    /* user specifying num list io */
    if (env_num_list) {
        num_listio = atoi(env_num_list);	
        if ((num_listio >= 500) ||
                (!num_listio) ) {
            /* Use default if 0 or greater than 500 */
            num_listio = 500;
        }
    }

    if (env_blk_verbosity)
        blk_verbosity = atoi(env_blk_verbosity);

    DEBUG_1 ("blk_verbosity = %s \n",env_blk_verbosity);
    DEBUG_1 ("num_listio = %d \n",num_listio);

    DEBUG_1("env_filemode = %s\n",env_filemode);
    DEBUG_1("env_max_xfer = %s\n",env_max_xfer);

    if (env_max_xfer) {
        max_xfer = atoi(env_max_xfer);
        DEBUG_1("max_xfer_size = 0x%lx\n",max_xfer);
    }
    if (paths == NULL) {
        if ((env_filemode) &&  (filemode=atoi(env_filemode) == 1)) {
            sprintf(test_filename, "/tmp/%s.capitestfile", env_user);

            fd = creat(test_filename, 0777);
            if (fd != -1) { 
                paths = &test_filename[0];
                num_devs = 1;
                ret = ftruncate (fd, FILESZ);
                if (ret){
                    fprintf(stderr,"blk_fvt_setup: couldn't increase  filesize\n");
                    fprintf (stderr,"\nftruncate: rc = %d, err = %d\n", ret, errno);
                    close (fd);
                }
            } else {
                fprintf(stderr,"blk_fvt_setup: couldn't create test file\n");
                fprintf(stderr,"blk_fvt_setup: fd = %d, errno = %d\n", fd, errno);
                return (-1);
            }
        } else {
            fprintf(stderr,"Environment FVT_DEV is not set\n");
            return (-1);
        }
    }

#ifndef _AIX
    fp = popen("cat /proc/cpuinfo|grep platform", "r");
    if (fp)
    {
        fgets(platform, 1024, fp);
        pclose(fp);
        if (strlen(platform)>0 && strstr(platform,"PowerNV")==NULL)
            {host_type=HOST_TYPE_VM;}
    }
#endif

    /* strcpy instead */
    strcpy (&tpaths[0], paths);


    DEBUG_2("\nsaving paths = %s to tpaths = %s\n",paths,tpaths);

    p = strtok(&tpaths[0], "," );
    if (p)
    {
        for (i=0; p != NULL; i++ ) {
            if (!filemode) {
                parents[i] = find_parent(p);
                DEBUG_2("\nparent of %s p = %s\n", p, parents[i]);
            }
            strcpy(dev_paths[i],p);
            DEBUG_2("\npath %d : %s\n", i, p);
            p = strtok((char *)NULL,",");
        }
        num_devs = i;
    }
    DEBUG_1("blk_fvt_setup: num_devs = %d\n",num_devs);

    bzero (&blk_data, sizeof(blk_data));
    DEBUG_1("blk_fvt_setup: path = %s\n", dev_paths[0]);
    DEBUG_1("blk_fvt_setup: allocating blocks %d\n", size);
    ret = blk_fvt_alloc_bufs(size);
    DEBUG_1("blk_fvt_alloc_bufs: ret = 0x%x\n", ret);

    num_opens = 0;
    ext       = 0;
    mode      = O_RDWR;

    struct rlimit limit;
    limit.rlim_cur = 5000;
    limit.rlim_max = 5000;
    setrlimit(RLIMIT_NOFILE, &limit);

    DEBUG_2("blk_fvt_setupi_exit: dev1 = %s, dev2 = %s\n", 
            dev_paths[0], dev_paths[1]);
    return (ret);
}

/* check parents of all devs, and see if the context is sharable */
int validate_share_context()
{
    int n,i=0;
    int rc = 0;
    char *p;
    n = num_devs;
    p = parents[i++];
    DEBUG_1("\nvalidate_share_context: num_devs = %d ", n);
    for (; i < n ; i++) {
        DEBUG_2("\nvalidate_share_context: %s\n and %s\n ", p, parents[i]);
        rc = strcmp (p, parents[i]);
        if (rc != 0)
            break;
    }

    DEBUG_1("\nvalidate_share_context: ret = 0x%x\n", rc);
    return(rc);

}

void blk_open_tst_inv_path (const char* path, int *id, int max_reqs, int *er_no, int opn_cnt, int flags, int mode)
{
    chunk_id_t j = NULL_CHUNK_ID;
    errno = 0;

    j = cblk_open (path, max_reqs, mode, ext, flags);

    *id = j;
    *er_no = errno; 
}

void blk_open_tst (int *id, int max_reqs, int *er_no, int opn_cnt, int flags, int mode)
{
    int             i = 0;
    chunk_id_t j = NULL_CHUNK_ID;
    errno = 0;

    DEBUG_4("blk_open_tst : path:%s cnt:%d max_reqs:%d flags:%x\n",
            dev_paths[0], opn_cnt, max_reqs, flags);
    DEBUG_3("               ext:%d num_opens:%d mode:%d\n",
            (uint32_t)ext, num_opens, mode);

    if (opn_cnt > 0)
    {
        for (i = 0; i!= opn_cnt; i++)
        {
            DEBUG_1("Opening %s\n",dev_paths[0]);
            if (flags & CBLK_GROUP_RAID0)
            {
                DEBUG_1("Opening RAID0 ext:%d\n", (uint32_t)ext);
                j = cblk_cg_open(dev_paths[0], max_reqs, mode, 1, ext, flags);
            }
            else if (flags & CBLK_GROUP_MASK)
            {
                DEBUG_1("Opening CG ext:%d\n", (uint32_t)ext);
                j = cblk_cg_open(dev_paths[0], max_reqs, mode, ext, 0, flags);
            }
            else
            {
                DEBUG_0("Opening dev\n");
                j = cblk_open(dev_paths[0], max_reqs, mode, ext, flags);
            }
            if (j != NULL_CHUNK_ID)
            {
                // handle[i] = j;
                chunks[i] = j;
                num_opens += 1;
                DEBUG_3("blk_open_tst:  OPENED %d, ext:%ld chunk_id=%d\n",
                        (num_opens), ext, j);
            }
            else
            {
                *id    = j;
                *er_no = errno;
                DEBUG_2("blk_open_tst: Failed: open i = %d, errno = %d\n",
                        i, errno);
                return;
            }
        }
        *id = j; 
    }
    DEBUG_2("blk_open_tst: id = %d, num_open = %d\n", j, num_opens);
}

void blk_close_tst(int id, int *rc, int *err, int close_flag)
{

    errno = 0;

    *rc  = cblk_close (id, close_flag);
    *err = errno;
    // it should be done in cleanup
    if ( !(*rc)  && !(*err))
        num_opens --;
    DEBUG_3("blk_close_tst: id = %d, erno = %d, rc = %d\n", id, errno, *rc);
}

void blk_open_tst_cleanup (int flags, int *p_rc, int *p_err)
{
    int             i  = 0;
    int             rc = 0;
    int             opns = num_opens;

    *p_rc  = 0;
    *p_err = 0;
    errno  = 0;

    DEBUG_2("blk_open_tst_cleanup: closing num_opens:%d flags:%x\n",
            num_opens, flags);
    for (i=0; i<opns; i++)
    {
        if (chunks[i] != NULL_CHUNK_ID)
        {
            rc = cblk_close(chunks[i], flags);
            if (!rc)
            {
                DEBUG_1("blk_open_tst_cleanup: closed id:%d\n", chunks[i]);
                chunks[i] = NULL_CHUNK_ID;
                --num_opens;
            }
            else
            {
                DEBUG_4("\nblk_open_tst_cleanup: Close FAILED chunk id = %d "
                        "rc = %d, errno %d num_opens:%d\n",
                        chunks[i], rc, errno, num_opens);
                *p_rc  = rc;
                *p_err = errno;
            }
        }
    }

    DEBUG_1("blk_open_tst_cleanup: ***Num Chunk CLOSED %d\n\n",i);

    // At the end of test executions, to free up allocated
    // test buffers.

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
}

void blk_fvt_get_set_lun_size(chunk_id_t id,
                              size_t    *size,
                              int        sz_flags,
                              int        get_set_size_flag,
                              int       *ret,
                              int       *err)
{
    size_t c_size;
    int rc;
    errno =0;

    if (!get_set_size_flag)
    {
        /* get physical lun size */
        rc = cblk_get_lun_size(id, &c_size, sz_flags);
        *err = errno;
        *size = c_size;
        *ret = rc;
        DEBUG_3("get_lun_size: sz = %d, ret = %d, err = %d\n",
                (int)c_size, rc, errno);
    }
    else if (get_set_size_flag == 1)
    {
        /* get chunk size */
        rc = cblk_get_size(id, &c_size, sz_flags);
        *err = errno;
        *ret = rc;
        *size = c_size;
        DEBUG_3("get_size: sz = %d, ret = %d, err = %d\n",
                (int)c_size, rc, errno);
    }
    else if (get_set_size_flag == 2)
    {
        /* set chunk size */
        c_size = *size;
        rc = cblk_set_size(id, c_size, sz_flags);
        *err = errno;
        *ret = rc;
        DEBUG_4("set_size: sz = %d, ret = %d, err = %d flags:%x\n",
                (int)c_size, rc, errno, sz_flags);
    }
}

int blk_fvt_alloc_bufs(int size)
{

    int ret = -1;
    int i,x;
    char* p;

    /*
     * Align data buffer on page boundary.
     */	
    if ( posix_memalign((void *)&blk_fvt_data_buf,4096,BLK_FVT_BUFSIZE*size)) {

        perror("posix_memalign failed for data buffer");
        return (ret);
    }

    bzero(blk_fvt_data_buf,BLK_FVT_BUFSIZE*size);

    /*
     * Align data buffer on page boundary.
     */	
    if ( posix_memalign((void *)&blk_fvt_comp_data_buf,4096,BLK_FVT_BUFSIZE*size)) {
        perror("posix_memalign failed for comp data buffer");
        free(blk_fvt_data_buf);
        blk_fvt_data_buf=NULL;
        return (ret);
    }
    bzero(blk_fvt_comp_data_buf,BLK_FVT_BUFSIZE*size);

    p = blk_fvt_comp_data_buf;
    for (i=1; i<size+1;i++) {
        /* fill each page with diff data to write */
        for (x=0; x<BLK_FVT_BUFSIZE; x++) {
            *p=i;
            p++;
        }     
    }
    return(0);
}

void blk_fvt_cmp_buf (int size, int *ret)
{
    int rc;

    rc = memcmp(blk_fvt_data_buf,blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE*size);
    if (rc) {
#ifdef _AIX
        DEBUG_1("Memcmp failed with rc = 0x%x\n",rc);
#else
        DEBUG_1("Memcmp failed with rc = 0x%x\n",rc);
#endif
        /**
          DEBUG_0("Written data:\n");
          dumppage(blk_fvt_comp_data_buf,BLK_FVT_BUFSIZE);
          DEBUG_0("******************************************************\n\n");
          DEBUG_0("read data:\n");
          dumppage(blk_fvt_data_buf,BLK_FVT_BUFSIZE);
         **/
    } else {
        DEBUG_1("Memcmp OK with rc = 0x%x\n",rc);
    }
    *ret = rc;
}

void blk_fvt_io (chunk_id_t id, int cmd, uint64_t lba_no, size_t nblocks,
                 int *ret, int *err, int io_flags, int open_flag)
{
    int rc = 0;
    errno = 0;
    int tag = 0;
    cblk_arw_status_t ard_status;
    cblk_arw_status_t awrt_status;
    uint64_t ar_status = 0;
    cflsh_cg_tag_t cgtag={-1,-1};
    int align = 0;

    DEBUG_4("blk_fvt_io: id:%d cmd:%d lba_no:%ld nblocks:%ld\n",
            id, cmd, lba_no, nblocks);
    DEBUG_2("blk_fvt_io: io_flags:%x open_flag:%x\n", io_flags, open_flag);

    switch  (cmd)
    {
        case FV_READ:
            rc = cblk_read(id,(blk_fvt_data_buf+align),lba_no,nblocks,io_flags);
            DEBUG_3("cblk_read complete for lba = 0x%lx, rc = %d, errno = %d\n",
                    lba_no,rc,errno);
            if (rc <= 0) {
                if(blk_verbosity == 9) {
                    fprintf(stderr,"cblk_read failed for  lba = 0x%lx, rc = %d,"
                                   "errno = %d\n",lba_no,rc,errno);
                    fprintf(stderr,"Returned data is ...\n");
                    hexdump(blk_fvt_data_buf,100,NULL);
                }
            }
            break;
        case FV_AREAD:

            if (io_flags & CBLK_ARW_USER_STATUS_FLAG)
            {
                if (io_flags & CBLK_GROUP_MASK)
                {
                    rc = cblk_cg_aread(id, blk_fvt_data_buf+align, lba_no,
                                       nblocks, &cgtag, &ard_status, io_flags);
                }
                else
                {
                    rc = cblk_aread(id, blk_fvt_data_buf+align, lba_no,
                                    nblocks, &tag, &ard_status, io_flags);
                }
            }
            else
            {
                if (io_flags & CBLK_GROUP_MASK)
                {
                    rc = cblk_cg_aread(id, blk_fvt_data_buf+align, lba_no,
                                       nblocks, &cgtag, NULL, io_flags);
                }
                else
                {
                    rc = cblk_aread(id, blk_fvt_data_buf+align, lba_no,
                                    nblocks, &tag, NULL, io_flags);
                }
            }

            if (rc < 0) {
                DEBUG_3("cblk_aread error  lba = 0x%lx, rc = %d, errno = %d\n",
                        lba_no, rc, errno);
                *ret = rc;
                *err = errno;
                return;
            } else {
                DEBUG_2("Async read returned rc = %d, tag =%d\n", rc, tag);
                while (TRUE) {

                    if (io_flags & CBLK_GROUP_MASK)
                    {
                        rc = cblk_cg_aresult(id,&cgtag, &ar_status,io_flags);
                    }
                    else
                    {
                        rc = cblk_aresult(id,&tag, &ar_status,io_flags);
                    }
                    if (rc > 0) {
                        if (blk_verbosity) {
                            DEBUG_0("Async read data completed ...\n");
                            if (blk_verbosity == 9) {
                                DEBUG_0("Returned data is ...\n");
                                hexdump(blk_fvt_comp_data_buf,100,NULL);
                            }
                        }
                    } else if (rc == 0) {
                        DEBUG_3("cblk_aresult completed: command still active "
                                "for tag = 0x%x, rc = %d, errno = %d\n",
                                tag,rc,errno);
                        usleep(1000);
                        continue;
                    } else {
                        DEBUG_3("cblk_aresult completed for  for tag = 0x%d, "
                                "rc = %d, errno = %d\n",tag,rc,errno);
                    }

                    break;

                } /* while */
            }
            break;

        case FV_WRITE:
            rc = cblk_write(id,blk_fvt_comp_data_buf+align,lba_no,
                                nblocks,io_flags);
            DEBUG_4("cblk_write complete at lba = 0x%lx, size = %lx, rc = %d, "
                    "errno = %d\n",lba_no,nblocks,rc,errno);
            if (rc <= 0) {
                DEBUG_3("cblk_write failed at lba = 0x%lx, rc = %d, errno=%d\n",
                        lba_no,rc,errno);
            }
            break;
        case FV_AWRITE:
            if (io_flags & CBLK_ARW_USER_STATUS_FLAG)
            {
                if (io_flags & CBLK_GROUP_MASK)
                {
                    rc = cblk_cg_awrite(id, blk_fvt_comp_data_buf+align, lba_no,
                                       nblocks, &cgtag, &awrt_status, io_flags);
                }
                else
                {
                    rc = cblk_awrite(id, blk_fvt_comp_data_buf+align, lba_no,
                                     nblocks, &tag, &awrt_status, io_flags);
                }
            }
            else
            {
                if (io_flags & CBLK_GROUP_MASK)
                {
                    rc = cblk_cg_awrite(id, blk_fvt_comp_data_buf+align, lba_no,
                                        nblocks, &cgtag, NULL, io_flags);
                }
                else
                {
                    rc = cblk_awrite(id, blk_fvt_comp_data_buf+align, lba_no,
                                     nblocks, &tag, NULL, io_flags);
                }
            }

            if (rc < 0) {
                DEBUG_3("cblk_awrite error  lba = 0x%lx, rc = %d, errno = %d\n",
                        lba_no, rc, errno);
                *ret = rc;
                *err = errno;
                return;
            }
            else
            {
                while (TRUE)
                {
                    if (io_flags & CBLK_GROUP_MASK)
                    {
                        rc = cblk_cg_aresult(id,&cgtag, &ar_status,io_flags);
                    }
                    else
                    {
                        rc = cblk_aresult(id, &tag, &ar_status, io_flags);
                    }
                    if (rc > 0)
                    {
                        if (blk_verbosity) {
                            DEBUG_0("Async write data completed ...\n");
                            if (blk_verbosity == 9) {
                                DEBUG_0("Returned data is ...\n");
                                hexdump(blk_fvt_comp_data_buf,100,NULL);
                            }
                        }
                    }
                    else if (rc == 0)
                    {
                        DEBUG_3("cblk_aresult completed: command still active "
                                "for tag = 0x%x, rc = %d, errno = %d\n",
                                tag,rc,errno);
                        usleep(1000);
                        continue;
                    }
                    else
                    {
                        DEBUG_3("cblk_aresult completed for  for tag = 0x%d, "
                                "rc = %d, errno = %d\n",tag,rc,errno);
                    }
                    break;

                } /* while */
            }
            break;

        default:
            break;
    }
    *ret = rc;
    *err = errno;

    DEBUG_2("blk_fvt_io: ret = %d, errno = %d\n", rc, errno);

}

void blk_fvt_intrp_io_tst(chunk_id_t id,
                          int testflag,
                          int open_flag,
                          int *ret,
                          int *err)
{
    int             rc = 0;
    int             err_no = 0;
    errno = 0;
    int             tag = 0;
    uint64_t        lba_no = 1;
    int             nblocks = 1;
    cblk_arw_status_t arw_status;
    int             fd;
    int             arflag = 0;
    int             io_flags = 0;

    bzero(&arw_status,sizeof(arw_status));

    /* fill compare buffer with pattern */
    fd = open("/dev/urandom", O_RDONLY);
    read(fd, blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE);
    close(fd);

    // testflag 1 - NO_INTRP      set, null status,  io_flags ARW_USER  set
    // testflag 2 - NO_INTRP _not set,      status,  io_flags ARW_USER  not set
    // testflag 3 - NO_INTRP _not set,      status,  io_flags ARW_USER  set
    // testflag 4 - NO_INTRP      set,      status,  io_flags ARW_USER  set
    // testflag 5 - NO_INTRP      set,      status,  ARW_USER | ARW_WAIT set
    // testflag 6 - NO_INTRP      set,      status,  ARW_USER | ARW_WAIT| ARW_USER_TAG set

    switch(testflag) {
        case 1:
            io_flags |= CBLK_ARW_USER_STATUS_FLAG;
            break;
        case 2:
            io_flags |= 0;
            break;
        case 3:
            io_flags |= CBLK_ARW_USER_STATUS_FLAG;
            break;
        case 4:
            io_flags |= CBLK_ARW_USER_STATUS_FLAG;
            break;
        case 5:
            io_flags |= CBLK_ARW_USER_STATUS_FLAG|CBLK_ARW_WAIT_CMD_FLAGS;
            break;
        case 6:
            io_flags |= CBLK_ARW_USER_STATUS_FLAG|CBLK_ARW_WAIT_CMD_FLAGS|CBLK_ARW_USER_TAG_FLAG;
            tag=77;
            break;

        default:
            break;

    }

    rc = cblk_awrite(id, blk_fvt_comp_data_buf, lba_no, nblocks,
            &tag,((testflag == 1) ? NULL : (&arw_status)), io_flags);

    if (rc < 0) {
        DEBUG_3("cblk_awrite error  lba = 0x%lx, rc = %d, errno = %d\n", lba_no, rc, errno);
        *ret = rc;
        *err = errno;
        return;
    }

    check_astatus(id, &tag, arflag, open_flag, io_flags, &arw_status, &rc, &err_no);

    DEBUG_2("blk_fvt_io: ret = %d, errno = %d\n", rc, err_no);
    *ret = rc;
    *err = errno;
    return;

}

void
check_astatus(chunk_id_t id, int *tag, int arflag, int open_flag, int io_flags, cblk_arw_status_t *arw_status, int *rc, int *err)
{
    uint64_t ar_status = 0;
    int ret = 0;

    while (TRUE) {
        if (io_flags & CBLK_ARW_USER_STATUS_FLAG)
        {
            switch (arw_status->status) {
                case CBLK_ARW_STATUS_SUCCESS:
                    ret = arw_status->blocks_transferred;
                    break;
                case CBLK_ARW_STATUS_PENDING:
                    ret = 0;
                    break;
                default:
                    ret = -1;
                    errno = arw_status->fail_errno;
            }

        } else {
            ret = cblk_aresult(id, tag, &ar_status, arflag);
        }
        if (ret > 0) {

            DEBUG_0("Success\n");
        } else if (ret == 0) {
            DEBUG_0("Cmd pending !\n");
            usleep(300);
            continue;
        } else {
            DEBUG_2("Cmd completed ret = %d, errno = %d\n",ret,errno);
        }

        break;

    } /* while */

    *rc = ret;
    *err = errno;

}

/**
********************************************************************************
** \brief
*******************************************************************************/
void blk_fvt_dump_stats(chunk_stats_t *stats)
{
    fprintf(stderr,"chunk_statistics:\n");
    fprintf(stderr,"**********************************\n");
    fprintf(stderr,"max_transfer_size:              0x%lx\n", stats->max_transfer_size);
    fprintf(stderr,"num_reads:                      0x%lx\n", stats->num_reads);
    fprintf(stderr,"num_writes:                     0x%lx\n", stats->num_writes);
    fprintf(stderr,"num_areads:                     0x%lx\n", stats->num_areads);
    fprintf(stderr,"num_awrites:                    0x%lx\n", stats->num_awrites);
    fprintf(stderr,"num_act_reads:                  0x%x\n", stats->num_act_reads);
    fprintf(stderr,"num_act_writes:                 0x%x\n", stats->num_act_writes);
    fprintf(stderr,"num_act_areads:                 0x%x\n", stats->num_act_areads);
    fprintf(stderr,"num_act_awrites:                0x%x\n", stats->num_act_awrites);
    fprintf(stderr,"max_num_act_writes:             0x%x\n", stats->max_num_act_writes);
    fprintf(stderr,"max_num_act_reads:              0x%x\n", stats->max_num_act_reads);
    fprintf(stderr,"max_num_act_awrites:            0x%x\n", stats->max_num_act_awrites);
    fprintf(stderr,"max_num_act_areads:             0x%x\n", stats->max_num_act_areads);
    fprintf(stderr,"num_blocks_read:                0x%lx\n", stats->num_blocks_read);
    fprintf(stderr,"num_blocks_written:             0x%lx\n", stats->num_blocks_written);
    fprintf(stderr,"num_errors:                     0x%lx\n", stats->num_errors);
    fprintf(stderr,"num_aresult_no_cmplt:           0x%lx\n", stats->num_aresult_no_cmplt);
    fprintf(stderr,"num_retries:                    0x%lx\n", stats->num_retries);
    fprintf(stderr,"num_timeouts:                   0x%lx\n", stats->num_timeouts);
    fprintf(stderr,"num_no_cmds_free:               0x%lx\n", stats->num_no_cmds_free);
    fprintf(stderr,"num_no_cmd_room:                0x%lx\n", stats->num_no_cmd_room);
    fprintf(stderr,"num_no_cmds_free_fail:          0x%lx\n", stats->num_no_cmds_free_fail);
    fprintf(stderr,"num_fc_errors:                  0x%lx\n", stats->num_fc_errors);
    fprintf(stderr,"num_port0_linkdowns:            0x%lx\n", stats->num_port0_linkdowns);
    fprintf(stderr,"num_port1_linkdowns:            0x%lx\n", stats->num_port1_linkdowns);
    fprintf(stderr,"num_port0_no_logins:            0x%lx\n", stats->num_port0_no_logins);
    fprintf(stderr,"num_port1_no_logins:            0x%lx\n", stats->num_port1_no_logins);
    fprintf(stderr,"num_port0_fc_errors:            0x%lx\n", stats->num_port0_fc_errors);
    fprintf(stderr,"num_port1_fc_errors:            0x%lx\n", stats->num_port1_fc_errors);
    fprintf(stderr,"num_cc_errors:                  0x%lx\n", stats->num_cc_errors);
    fprintf(stderr,"num_afu_errors:                 0x%lx\n", stats->num_afu_errors);
    fprintf(stderr,"num_capi_false_reads:           0x%lx\n", stats->num_capi_false_reads);
    fprintf(stderr,"num_capi_adap_resets:            0x%lx\n", stats->num_capi_adap_resets);
    fprintf(stderr,"num_capi_afu_errors:            0x%lx\n", stats->num_capi_afu_errors);
    fprintf(stderr,"num_capi_afu_intrpts:           0x%lx\n", stats->num_capi_afu_intrpts);
    fprintf(stderr,"num_capi_unexp_afu_intrpts:     0x%lx\n", stats->num_capi_unexp_afu_intrpts);
    fprintf(stderr,"num_active_threads:             0x%lx\n", stats->num_active_threads);
    fprintf(stderr,"max_num_act_threads:            0x%lx\n", stats->max_num_act_threads);
    fprintf(stderr,"num_cache_hits:                 0x%lx\n", stats->num_cache_hits);
    fprintf(stderr,"**********************************\n");


}

/**
********************************************************************************
** \brief
*******************************************************************************/
void blk_get_statistics (chunk_id_t id, int flags, int *ret, int *err)
{

    int rc;
    chunk_stats_t stats;

    rc = cblk_get_stats (id, &stats, flags);
    *ret = rc;
    *err = errno;

    if (rc) {
        DEBUG_2("blk_get_statistics: failed ret = %d, errno = %d\n", rc, errno);
    } else {
        if (blk_verbosity == 9) {
            fprintf(stderr,"cblk_get_stats completed ...\n");
            blk_fvt_dump_stats (&stats);
            hexdump(&stats,sizeof(stats),NULL);
        }
    }
}

/**
********************************************************************************
** \brief
*******************************************************************************/
void* blk_io_loop(void *data)
{
    cflsh_cg_tag_t      cgtag           = {0};
    int                 tag             = 0;
    blk_thread_data_t  *blk_data        = data;
    int                 rc              = 0;
    void               *data_buf        = NULL;
    void               *comp_data_buf   = NULL;
    uint64_t            blk_number      = 0;
    uint64_t            ar_status       = 0;
    int                 cmd_type        = 0;
    int                 x               = 0;
    int                 loop            = 0;
    int                 lba             = 0;
    int                 max_lba         = 0;
    char               *env_num_loop    = getenv("FVT_NUM_LOOPS");

    if (env_num_loop && atoi(env_num_loop)>0)
    {
        num_loops = atoi(env_num_loop) % CHUNK_SIZE;
    }

    /* Each thread is using a different block number range */
    blk_number = blk_data->tid * CHUNK_SIZE;
    max_lba    = blk_number    + CHUNK_SIZE-1;

    if (posix_memalign((void *)&data_buf,4096,BLK_FVT_BUFSIZE))
    {
        perror("posix_memalign failed for data buffer");
        blk_data->ret     = -1;
        blk_data->errcode = errno;
        goto exit;
    }
    if (posix_memalign((void*)&comp_data_buf,4096,BLK_FVT_BUFSIZE))
    {
        perror("posix_memalign failed for data buffer");
        blk_data->ret     = -1;
        blk_data->errcode = errno;
        goto exit;
    }

    errno = 0;

    if (blk_data->tid % 2 && !(blk_data->rwflags & TST_SYNC_IO_ONLY))
    {
        cmd_type=FV_RW_AWAR;
    }
    else
    {
        cmd_type = FV_RW_COMP;
    }

    for (x=0; x<blk_data->num_devs; x++)
    {
        lba  = blk_number;
        loop = 0;

        while (loop < num_loops)
        {
            GEN_VAL(comp_data_buf, lba, BLK_FVT_BUFSIZE);

            if (cmd_type == FV_RW_COMP)
            {
                if (blk_data->rwflags & CBLK_GROUP_MASK)
                {
                    rc = cblk_cg_write(blk_data->chunk_id[x], comp_data_buf,
                                        lba, 1, blk_data->rwflags);
                    DEBUG_4("tid:%d cg_write id:%d lba:%d rc:%d\n",
                            blk_data->tid, blk_data->chunk_id[x], lba, rc);
                }
                else
                {
                    rc = cblk_write(blk_data->chunk_id[x],comp_data_buf,
                                    lba,1,blk_data->rwflags);
                }
                if (rc!=1)
                {
                    DEBUG_3("write failed tid:%d rc:%d errno:%d\n",
                            blk_data->tid, rc, errno);
                    goto exit;
                }

                if (blk_data->rwflags & CBLK_GROUP_MASK)
                {
                    rc = cblk_cg_read(blk_data->chunk_id[x], data_buf,
                                        lba, 1, blk_data->rwflags);
                    DEBUG_4("tid:%d cg_read id:%d lba:%d rc:%d\n",
                            blk_data->tid, blk_data->chunk_id[x], lba, rc);
                }
                else
                {
                    rc = cblk_read(blk_data->chunk_id[x],data_buf,
                               lba,1,blk_data->rwflags);
                }
                if (rc!=1)
                {
                    DEBUG_3("read failed tid:%d rc:%d errno:%d\n",
                            blk_data->tid, rc, errno);
                    goto exit;
                }

                if ((rc=memcmp(data_buf,comp_data_buf,BLK_FVT_BUFSIZE)) != 0)
                {
                    DEBUG_2("tid:%d memcmp failed rc:%d\n", blk_data->tid,rc);
                        /*
                           fprintf(stderr,"Written data:\n");
                           dumppage(data_buf,BLK_FVT_BUFSIZE);
                           fprintf(stderr,"******************************\n\n");
                           fprintf(stderr,"read data:\n");
                           dumppage(comp_data_buf,BLK_FVT_BUFSIZE);
                           fprintf(stderr,"******************************\n\n");
                         */
                    rc = cblk_read(blk_data->chunk_id[x],data_buf,
                                   lba,1,blk_data->rwflags);
                    if (rc == 1)
                    {
                        if (blk_verbosity == 9)
                        {
                            fprintf(stderr,"Dump of re-read\n");
                            dumppage(data_buf,BLK_FVT_BUFSIZE);
                        }
                    }
                    goto exit;
                }

                if (lba%100==0 && blk_data->rwflags & TST_UNMAP)
                {
                    memset(data_buf,0,BLK_FVT_BUFSIZE);
                    if (blk_data->rwflags & CBLK_GROUP_MASK)
                    {
                        rc = cblk_cg_unmap(blk_data->chunk_id[x],
                                           data_buf,lba,1,blk_data->rwflags);
                    }
                    else
                    {
                        rc = cblk_unmap(blk_data->chunk_id[x],data_buf,lba,1,0);
                    }
                    if (rc != 1)
                    {
                        DEBUG_3("unmap failed tid:%d rc:%d errno:%d\n",
                                blk_data->tid, rc, errno);
                        goto exit;
                    }
                }
            }
            else if (cmd_type == FV_RW_AWAR)
            {
                if (blk_data->rwflags & CBLK_GROUP_MASK)
                {
                    rc = cblk_cg_awrite(blk_data->chunk_id[x], comp_data_buf,
                                        lba, 1, &cgtag, NULL,blk_data->rwflags);
                    DEBUG_3("cg_awrite %d:%d rc:%d\n", cgtag.id, cgtag.tag, rc);
                }
                else
                {
                    rc = cblk_awrite(blk_data->chunk_id[x], comp_data_buf,
                                     lba, 1, &tag, NULL,blk_data->rwflags);
                    DEBUG_1("awrite rc:%d\n", rc);
                }
                if (rc < 0)
                {
                    DEBUG_3("awrite failed tid:%d rc:%d errno:%d\n",
                            blk_data->tid, rc, errno);
                    goto exit;
                }
                while (TRUE)
                {
                    if (blk_data->resflags & CBLK_GROUP_MASK)
                    {
                        DEBUG_3("cg_aresult %d:%d rc:%d\n", cgtag.id, cgtag.tag, rc);
                        rc = cblk_cg_aresult(blk_data->chunk_id[x], &cgtag,
                                             &ar_status, blk_data->resflags);
                    }
                    else
                    {
                        rc = cblk_aresult(blk_data->chunk_id[x], &tag,
                                          &ar_status, blk_data->resflags);
                        DEBUG_1("aresult rc:%d\n", rc);
                    }
                    if (rc > 0)
                    {
                        DEBUG_3("awrite cmp tid:%3d lba:%6d rc=%d \n",
                                blk_data->tid, lba, rc);
                        break;
                    }
                    else if (rc == 0)
                    {
                        DEBUG_3("awrite wait tid:%3d lba:%6d rc=%d\n",
                                blk_data->tid, lba, rc);
                        usleep(10);
                    }
                    else
                    {
                        DEBUG_4("awrite err tid:%3d lba:%6d rc:%d errno:%d\n",
                                blk_data->tid, lba,rc,errno);
                        goto exit;
                    }
                }

                if (blk_data->rwflags & CBLK_GROUP_MASK)
                {
                    rc = cblk_cg_aread(blk_data->chunk_id[x],data_buf,lba,1,
                                       &cgtag,NULL,blk_data->rwflags);
                    DEBUG_3("cg_aread %d:%d rc:%d\n", cgtag.id, cgtag.tag, rc);
                }
                else
                {
                    rc = cblk_aread(blk_data->chunk_id[x],data_buf,lba,1,
                                    &tag,NULL,blk_data->rwflags);
                }
                if (rc < 0)
                {
                    DEBUG_2("Aread failed rc = %d, errno = %d\n",rc, errno);
                    goto exit;
                }

                while (TRUE)
                {
                    if (blk_data->resflags & CBLK_GROUP_MASK)
                    {
                        DEBUG_3("cg_aresult %d:%d rc:%d\n", cgtag.id, cgtag.tag, rc);
                        rc = cblk_cg_aresult(blk_data->chunk_id[x], &cgtag,
                                             &ar_status, blk_data->resflags);
                    }
                    else
                    {
                        rc = cblk_aresult(blk_data->chunk_id[x], &tag,
                                          &ar_status, blk_data->resflags);
                    }
                    if (rc > 0)
                    {
                        DEBUG_3("aread  cmp tid:%3d lba:%6d rc=%d \n",
                                blk_data->tid, lba, rc);
                        break;
                    }
                    else if (rc == 0)
                    {
                        DEBUG_3("aread  wait tid:%3d lba:%6d rc=%d\n",
                                blk_data->tid, lba, rc);
                        usleep(10);
                    }
                    else
                    {
                        DEBUG_4("aread  err tid:%3d lba:%6d rc:%d errno:%d\n",
                                blk_data->tid, lba,rc,errno);
                        goto exit;
                    }
                }

                if ((rc=memcmp(data_buf,comp_data_buf,BLK_FVT_BUFSIZE)) != 0)
                {
                    if (blk_verbosity==9)
                    {
                        fprintf(stderr,"Data compare failure\n");
                        /*
                           fprintf(stderr,"Written data:\n");
                           dumppage(data_buf,BLK_FVT_BUFSIZE);
                           fprintf(stderr,"******************************\n\n");
                           fprintf(stderr,"read data:\n");
                           dumppage(comp_data_buf,BLK_FVT_BUFSIZE);
                           fprintf(stderr,"******************************\n\n");
                         */
                    }
                    rc = cblk_read(blk_data->chunk_id[x],data_buf,
                                   lba,1,blk_data->rwflags);
                    if (rc == 1)
                    {
                        if (blk_verbosity==9)
                        {
                            DEBUG_0("Dump re-read OK\n");
                            /*
                               dumppage(data_buf,BLK_FVT_BUFSIZE);
                             */
                        }
                    }
                    goto exit;
                }
                if (lba%100==0 && blk_data->rwflags & TST_UNMAP)
                {
                    memset(data_buf,0,BLK_FVT_BUFSIZE);
                    if (blk_data->rwflags & CBLK_GROUP_MASK)
                    {
                        rc = cblk_cg_aunmap(blk_data->chunk_id[x],data_buf,lba,1,
                                           &cgtag,NULL,blk_data->rwflags);
                    }
                    else
                    {
                        rc = cblk_aunmap(blk_data->chunk_id[x],data_buf,lba,1,
                                        &tag,NULL,blk_data->rwflags);
                    }
                    if (rc)
                    {
                        DEBUG_3("unmap failed tid:%d rc:%d errno:%d\n",
                                blk_data->tid, rc, errno);
                        goto exit;
                    }
                    while (TRUE)
                    {
                        if (blk_data->rwflags & CBLK_GROUP_MASK)
                        {
                            rc = cblk_cg_aresult(blk_data->chunk_id[x], &cgtag,
                                                 &ar_status,blk_data->resflags);
                        }
                        else
                        {
                            rc = cblk_aresult(blk_data->chunk_id[x], &tag,
                                              &ar_status, blk_data->resflags);
                        }
                        if (rc > 0)
                        {
                            DEBUG_3("aunmap cmp tid:%3d lba:%6d rc=%d \n",
                                    blk_data->tid, lba, rc);
                            break;
                        }
                        else if (rc == 0)
                        {
                            DEBUG_3("aunmap wait tid:%3d lba:%6d rc=%d\n",
                                    blk_data->tid, lba, rc);
                            usleep(10);
                        }
                        else
                        {
                            DEBUG_4("aunmap err tid:%3d lba:%6d rc:%d errno:%d\n",
                                    blk_data->tid, lba,rc,errno);
                            goto exit;
                        }
                    }
                }
            }

            if (++lba == max_lba)
            {
                loop += 1;
                lba   = blk_number;
                DEBUG_3("tid:%d dev:%d loop:%d \n", blk_data->tid, x, loop);
            }
        } /* while num_loops */
    } /* for num_luns */
    rc=0;

exit:
DEBUG_4("tid:%d dev:%d lba:%d rc:%d\n",
            blk_data->tid, x, lba, rc);

    blk_data->ret     = rc;
    blk_data->errcode = errno;

    if (data_buf)      {free(data_buf);}
    if (comp_data_buf) {free(comp_data_buf);}

    return NULL;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
void blk_thread_tst(int *ret, int *err, int open_flags,
                    int rwflags, int resflags, int eeh)
{
    int                 rc          = 0;
    int                 ret_code    = 0;
    int                 i           = 0;
    int                 x           = 0;
    blk_thread_data_t  *tdata       = NULL;
    chunk_id_t          id[MAX_LUNS]= {0};

    DEBUG_3("num_loops:%d num_threads:%d num_devs:%d\n",
            num_loops, num_threads, num_devs);

    if (env_filemode && atoi(env_filemode)==1)
    {
        ext=1;
        if (num_threads > 10) {num_threads=10;}
    }

    if (num_devs==0)
    {
        DEBUG_0("num_devs is 0\n");
        *ret = -1;
        *err = 22;
        return;
    }

    DEBUG_3("blk_thread_tst open_flags:%x rwflags:%x eeh:%d\n",
            open_flags, rwflags, eeh);
    DEBUG_3("num_loops:%d num_threads:%d num_devs:%d\n",
            num_loops, num_threads, num_devs);

    tdata = malloc(num_threads*sizeof(blk_thread_data_t));
    memset(tdata,0,num_threads*sizeof(blk_thread_data_t));

    for (x=0; x<num_devs; x++)
    {
        if (open_flags & CBLK_GROUP_RAID0)
        {
            id[x] = cblk_cg_open(dev_paths[0], 1024, O_RDWR, 1, ext,open_flags);
            DEBUG_1("Open RAID0 id:%d\n", id[x]);
        }
        else if (open_flags & CBLK_GROUP_MASK)
        {
            id[x] = cblk_cg_open(dev_paths[0], 1024, O_RDWR, ext, 0,open_flags);
            DEBUG_2("Open CG ext:%d id:%d\n", (uint32_t)ext, id[x]);
        }
        else
        {
            DEBUG_0("Opening\n");
            id[x] = cblk_open(dev_paths[x], 1024, O_RDWR, ext, open_flags);
        }

        if (id[x] == NULL_CHUNK_ID)
        {
            DEBUG_2("Open of %s failed with errno = %d\n",dev_paths[x],errno);
            *ret = -1;
            *err = errno;
            return;
        }

        num_opens++;
        /* Skip for physical luns */
        if (open_flags & CBLK_OPN_VIRT_LUN)
        {
            /*
             * On the first pass thru this loop
             * for virtual lun, open the virtual lun
             * and set its size. Subsequent passes.
             * skip this step.
             */
            DEBUG_2("set_size:%d flags:%x\n", CHUNK_SIZE*num_threads, open_flags);
            rc = cblk_set_size(id[x], CHUNK_SIZE*num_threads, open_flags);

            if (rc)
            {
                perror("cblk_set_size failed\n");
                *ret = -1;
                *err = errno;
                for (x=0; x<num_opens; x++)
                {
                    cblk_close(id[x], open_flags);
                }
                return;
            }
        }
    }

    if (num_threads > 0)
    {
        /* Create all threads here */
        for (i=0; i<num_threads; i++)
        {
            memcpy(tdata[i].chunk_id,id,sizeof(id));
            tdata[i].num_devs = num_devs;
            tdata[i].rwflags  = rwflags;
            tdata[i].resflags = resflags;
            tdata[i].tid      = i;
            rc = pthread_create(&blk_thread[i], NULL, blk_io_loop,
                                (void*)&tdata[i]);
            if (rc)
            {
                DEBUG_3("pthread_create failed for %d rc 0x%x, errno = 0x%x\n",
                        i, rc,errno);
                *ret = -1;
                *err = errno;
            }
        }

        if (eeh)
        {
            sleep(3);
            if (inject_EEH(dev_paths[0])) {*ret=-1; return;}
            DEBUG_0("after eeh inject\n");
        }

        /* Wait for all threads to complete */
        errno = 0;

        for (i=0; i<num_threads; i++)
        {
            rc = pthread_join(blk_thread[i], NULL);

            if (tdata[i].ret || tdata[i].errcode)
            {
                *ret = tdata[i].ret;
                *err = tdata[i].errcode;
                DEBUG_3("Thread %d returned fail ret %x, errno = %d\n",
                        i, tdata[i].ret, tdata[i].errcode);
            }
        }
    }

    DEBUG_1("Calling cblk_close num_opens:%d...\n",num_opens);
    for (x=0; x<num_opens; x++)
    {
        ret_code = cblk_close(id[x], open_flags);
        if (ret_code)
        {
            DEBUG_2("Close of id:%d failed with errno = %d\n", id[x],errno);
            *ret = ret_code;
            *err = errno;
        }
    }
    if (tdata) {free(tdata);};
}

/**
********************************************************************************
** \brief
*******************************************************************************/
void blocking_io_tst (chunk_id_t id, int *ret, int *err)
{
    int rc = -1;
    errno = 0;
    int rtag = -1;
    uint64_t ar_status = 0;
    uint64_t lba =1;
    size_t nblocks=1;
    int fd, i;
    int arflg = 0;
    int t = 1;
    int num_cmds=100;

    for (i=0; i<num_cmds; i++,lba++)
    {
        /* fill compare buffer with pattern */
        fd = open ("/dev/urandom", O_RDONLY);
        read (fd, blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE);
        close (fd);

        rc = cblk_awrite(id,blk_fvt_comp_data_buf,lba,nblocks,&rtag,NULL,arflg);
        DEBUG_3("\n***** cblk_awrite rc = 0x%d, tag = %d, lba =0x%lx\n",
                rc, rtag, lba);
        if (!rc)
        {
            DEBUG_1("Async write returned tag = %d\n", rtag);
        } 
        else if (rc < 0)
        {
            DEBUG_3("awrite failed for  lba = 0x%lx, rc = %d, errno = %d\n",
                    lba,rc,errno);
        }
    }

    arflg = CBLK_ARESULT_NEXT_TAG|CBLK_ARESULT_BLOCKING;

    while (TRUE)
    {
        rc = cblk_aresult(id,&rtag, &ar_status,arflg);

        if (rc > 0)
        {
            DEBUG_2("aresult rc = %d, tag = %d\n",rc,rtag);
            t++;
            if (t>num_cmds) {break;}
        }
        else
        {
            DEBUG_1("aresult error = %d\n",errno);
            break;
        }
    }

    *ret = rc;
    *err = errno;

    return;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
void user_tag_io_tst (chunk_id_t id, int *ret, int *err, int flags)
{
    int      rc        = -1;
    uint64_t ar_status = 0;
    uint64_t lba       = 0;
    size_t   nblocks   = 1;
    int      i         = 0;
    int      iter      = 0;
    int      tag[NBUFS]= {0};
    int      arflg     = 0;
    char    *data_buf[NBUFS];

    errno = 0;
    memset(data_buf, 0, sizeof(data_buf));
    blk_fvt_comp_data_buf = NULL;

START:
    for (i=0; i<NBUFS; i++)
    {
        /* Align write buffers on page boundary */
        if (posix_memalign((void *)(data_buf+i),4096,BLK_FVT_BUFSIZE))
        {
            perror("posix_memalign failed for data buffer");
            rc=-1;
            goto exit;
        }
        memset(*(data_buf+i), i+1, BLK_FVT_BUFSIZE);
    }
    /* Align read buffer on page boundary */
    if (posix_memalign((void *)&blk_fvt_comp_data_buf,4096,BLK_FVT_BUFSIZE))
    {
        perror("posix_memalign failed for comp data buffer");
        rc=-1;
        goto exit;
    }
    bzero(blk_fvt_comp_data_buf, BLK_FVT_BUFSIZE);

    arflg = CBLK_ARW_WAIT_CMD_FLAGS | CBLK_ARW_USER_TAG_FLAG | flags;

    /* do writes */
    lba = 10;
    for (i=0; i<NBUFS; i++,lba++)
    {
        tag[i]=lba;
        rc = cblk_awrite(id, *(data_buf+i), lba, nblocks, &tag[i], NULL, arflg);
        DEBUG_3("\n***** cblk_awrite rc = 0x%d, tag = %d, lba =0x%lx\n",
                rc, tag[i], lba);
        if (rc < 0)
        {
            DEBUG_3("awrite failed for  lba = 0x%lx, rc = %d, errno = %d\n",
                    lba,rc,errno);
            goto exit;
        }
    }

    arflg = CBLK_ARESULT_USER_TAG | CBLK_ARESULT_BLOCKING | flags;

    /* do completions in the reverse order that they were sent */
    for (i=NBUFS-1; i>=0; i--)
    {
        rc  = cblk_aresult(id, &tag[i], &ar_status, arflg);
        if (rc != 1)
        {
            DEBUG_1("aresult error = %d\n",errno);
            rc=-1;
            goto exit;
        }
        DEBUG_2("\n***** cblk_aresult rc = 0x%d, tag = %d\n", rc, tag[i]);
    }

    /* do read-compares */
    lba = 10;
    for (i=0; i<NBUFS; i++,lba++)
    {
        tag[i]= NBUFS+i;
        arflg = CBLK_ARW_WAIT_CMD_FLAGS | CBLK_ARW_USER_TAG_FLAG | flags;
        rc    = cblk_aread(id, blk_fvt_comp_data_buf, lba, nblocks, &tag[i],
                        NULL, arflg);
        DEBUG_3("\n***** cblk_aread rc = 0x%d, tag = %d, lba =0x%lx\n",
                rc, tag[i], lba);
        if (rc < 0)
        {
            DEBUG_3("aread failed for  lba = 0x%lx, rc = %d, errno = %d\n",
                    lba,rc,errno);
            goto exit;
        }

        arflg = CBLK_ARESULT_USER_TAG | CBLK_ARESULT_BLOCKING | flags;

        rc = cblk_aresult(id, &tag[i], &ar_status, arflg);
        if (rc != 1)
        {
            DEBUG_1("aresult error = %d\n",errno);
            rc=-1;
            goto exit;
        }
        DEBUG_2("\n***** cblk_aresult rc = 0x%d, tag = %d\n", rc, tag[i]);
        if (memcmp(blk_fvt_comp_data_buf, *(data_buf+i), BLK_FVT_BUFSIZE) != 0)
        {
            printf("miscompare for tag: %d\n", tag[i]);
            rc=-1;
            goto exit;
        }
    }

    if (++iter < 3) goto START;

exit:
    *ret = rc;
    *err = errno;

    for (i=0; i<NBUFS; i++)
    {
        if (*(data_buf+i))
        {
            free(*(data_buf+i));
        }
    }

    return;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
void io_perf_tst (int *ret, int *err, int rwflags, int resflags, int eeh)
{
    int             rc              = -1;
    int             er              = -1;
    int            *ctag[256]       = {0};
    cflsh_cg_tag_t *cgtag[256]      = {0};
    uint64_t        ar_status       = 0;
    uint64_t        startlba        = 0;
    uint64_t        lba             = 0;
    size_t          nblocks         = 1;
    size_t          maxlba          = 0;
    int             i               = 0;
    int             idx             = 0;
    int             ret_code        = 0;
    int             t               = 0;
    int             x               = 0;
    int             y               = 0;
    char           *env_num_cmds    = getenv("FVT_NUM_CMDS");
    char           *env_num_loop    = getenv("FVT_NUM_LOOPS");
    int             bsz             = 4096;
    int             loops           = 100;
    int             num_cmds        = 1024;
    int             eeh_fail        = 0;
    char           *addr            = NULL;

    if (env_filemode && atoi(env_filemode)==1)
    {
        eeh=0; loops=2; num_cmds=64;
        if      (num_opens == 1) {loops=5; num_cmds=256;}
        else if (num_opens > 8)  {num_cmds=32;}
    }
    else if (num_opens > 8) {loops=2;}

    if (eeh)                                  {loops=2; num_cmds=128;}
    if (env_num_cmds && atoi(env_num_cmds)>0) {num_cmds = atoi(env_num_cmds) % 1024;}
    if (env_num_loop && atoi(env_num_loop)>0) {loops = atoi(env_num_loop);}

    for (idx=0; idx<num_opens; idx++)
    {
        ctag[idx]  = malloc(num_cmds*sizeof(int));
        cgtag[idx] = malloc(num_cmds*sizeof(cflsh_cg_tag_t));
    }

    blk_fvt_get_set_lun_size(chunks[0], &maxlba, rwflags, 1, &rc, &er);
    DEBUG_3("id:%d maxlba:%ld flags:%x\n", chunks[0], maxlba, rwflags);
    DEBUG_3("luns:%d loops:%d cmds:%d\n", num_opens, loops, num_cmds);

    errno = 0;
    for (x=0; x<loops; x++, y++)
    {
        printf("lba:%ld loop:%d of %d\r", startlba, x, loops); fflush(stdout);

        for (idx=0; idx<num_opens; idx++)
        {
            for (i=0; i<num_cmds; i++)
            {
                addr = (char*)(blk_fvt_comp_data_buf)+(((idx*num_cmds)+i)*bsz);
                GEN_VAL(addr, lba, nblocks*4096);
                if (rwflags & CBLK_GROUP_MASK)
                {
                    rc = cblk_cg_awrite(chunks[idx], addr, lba, nblocks,
                                        &cgtag[idx][i], NULL,
                                        rwflags);
                    DEBUG_4("id:%d cg_awrite addr:%p lba:%ld rc:%d\n",
                            chunks[idx], addr, lba, rc);
                }
                else
                {
                    rc = cblk_awrite(chunks[idx],addr, lba, nblocks,&ctag[idx][i],
                                    NULL, rwflags);
                    DEBUG_4("id:%d awrite addr:%p lba:%ld rc:%d\n",
                            chunks[idx], addr, lba, rc);
                }
                if (rc < 0)
                {
                    fprintf(stderr,"awrite failed for lba = 0x%lx, rc = %d, "
                                   "errno = %d\n",lba,rc,errno);
                    *ret = rc;
                    *err = errno;
                    return;
                }
                ++lba; lba%=maxlba;
            }
            lba = startlba;
            DEBUG_3("id:%d awrite num_cmds:%d startlba:%ld\n", chunks[idx],i,startlba);
        }

        if (x==1 && eeh)
        {
            DEBUG_1("do EEH inject to %s\n", dev_paths[0]);
            if (inject_EEH(dev_paths[0])) {eeh_fail=1;}
            DEBUG_0("after eeh inject\n");
        }

        for (idx=0; idx<num_opens; idx++)
        {
            i=0;
            while (TRUE)
            {
                errno=0;
                if (rwflags & CBLK_GROUP_MASK)
                {
                    if (cgtag[idx][i].tag == -1) {++i; continue;}
                    rc = cblk_cg_aresult(chunks[idx], &cgtag[idx][i],
                                         &ar_status, resflags);
                }
                else
                {
                    if (ctag[idx][i] == -1) {++i; continue;}
                    rc = cblk_aresult(chunks[idx], &ctag[idx][i],
                                      &ar_status, resflags);
                }
                if (rc > 0)
                {
                    cgtag[idx][i++].tag = -1;
                    if (i>=num_cmds)
                    {
                        DEBUG_4("id:%d awrite->aresult DONE rc:%d t:%d num_cmds:%d\n",
                                chunks[idx], rc, t, num_cmds);
                        break;
                    }
                }
                else if (rc == 0) {//DEBUG_0("awrite->aresult rc=0\n");
                    continue;}
                else
                {
                    fprintf(stderr,"awrite->aresult cmdno:%d errno:%d rc:%d\n",
                            t, errno,rc);
                    *ret = rc;
                    *err = errno;
                    return;
                }
            }
        }

        lba = startlba;

        for (idx=0; idx<num_opens; idx++)
        {
            /* read the buffer we wrote */
            for (i=0; i<num_cmds; i++)
            {
                addr = (char*)(blk_fvt_data_buf)+(((idx*num_cmds)+i)*bsz);
                DEBUG_3("id:%d aread addr:%p lba:%ld\n", chunks[idx], addr, lba);
                if (rwflags & CBLK_GROUP_MASK)
                {
                    rc = cblk_cg_aread(chunks[idx], addr, lba, nblocks,
                                       &cgtag[idx][i], NULL,
                                       rwflags);
                }
                else
                {
                    rc = cblk_aread(chunks[idx], addr, lba, nblocks, &ctag[idx][i],
                                    NULL,rwflags);
                }
                if (rc < 0)
                {
                    fprintf(stderr,"aread failed for lba = 0x%lx, rc = %d, "
                                   "errno = %d\n",lba,rc,errno);
                    *ret = rc;
                    *err = errno;
                    return;
                }
                ++lba; lba%=maxlba;
            }
            lba = startlba;
            DEBUG_3("id:%d aread num_cmds:%d startlba:%ld\n", chunks[idx], i,startlba);
        }

        for (idx=0; idx<num_opens; idx++)
        {
            i=0;
            while (TRUE)
            {
                if (rwflags & CBLK_GROUP_MASK)
                {
                    if (cgtag[idx][i].tag == -1) {++i; continue;}
                    rc = cblk_cg_aresult(chunks[idx], &cgtag[idx][i],
                                         &ar_status, resflags);
                }
                else
                {
                    if (ctag[idx][i] == -1) {++i; continue;}
                    rc = cblk_aresult(chunks[idx], &ctag[idx][i],
                                      &ar_status, resflags);
                }
                if (rc > 0)
                {
                    cgtag[idx][i++].tag = -1;
                    if (i>=num_cmds) {break;}
                }
                else if (rc == 0) {continue;}
                else
                {
                    fprintf(stderr,"aread->aresult error = %d\n",errno);
                    *ret = rc;
                    *err = errno;
                    return;
                }
            }
        }

        ret_code = memcmp((char*)(blk_fvt_data_buf),
                          (char*)(blk_fvt_comp_data_buf),
                          num_cmds*bsz);
        if (ret_code)
        {
            fprintf(stderr,"\n memcmp failed rc = 0x%x\n",ret_code);
            *ret = ret_code;
            *err = errno;
            return;
        }
        else
        {
            DEBUG_1("memcmp success loop:%d\n", x);
        }
        /* clear the read buf for the next pass */
        memset(blk_fvt_data_buf,0,num_cmds*bsz);


        lba = startlba;

        for (idx=0; idx<num_opens && rwflags&TST_UNMAP; idx++)
        {
            /* unmap the lbas we wrote */
            for (i=0; i<num_cmds; i++)
            {
                addr = (char*)(blk_fvt_data_buf)+(((idx*num_cmds)+i)*bsz);
                DEBUG_3("id:%d aunmap addr:%p lba:%ld\n", chunks[idx], addr, lba);
                memset(addr,0,BLK_FVT_BUFSIZE*nblocks);
                if (rwflags & CBLK_GROUP_MASK)
                {
                    rc = cblk_cg_aunmap(chunks[idx], addr, lba, nblocks,
                                       &cgtag[idx][i], NULL, rwflags);
                }
                else
                {
                    rc = cblk_aunmap(chunks[idx], addr, lba, nblocks, &ctag[idx][i],
                                    NULL,rwflags);
                }
                if (rc < 0)
                {
                    fprintf(stderr,"aunmap failed for lba = 0x%lx, rc = %d, "
                                   "errno = %d\n",lba,rc,errno);
                    *ret = rc;
                    *err = errno;
                    return;
                }
                ++lba; lba%=maxlba;
            }
            lba = startlba;
            DEBUG_3("id:%d aunmap num_cmds:%d startlba:%ld\n", chunks[idx], i,startlba);
        }

        for (idx=0; idx<num_opens && rwflags&TST_UNMAP; idx++)
        {
            i=0;
            while (TRUE)
            {
                if (rwflags & CBLK_GROUP_MASK)
                {
                    if (cgtag[idx][i].tag == -1) {++i; continue;}
                    rc = cblk_cg_aresult(chunks[idx], &cgtag[idx][i],
                                         &ar_status, resflags);
                }
                else
                {
                    if (ctag[idx][i] == -1) {++i; continue;}
                    rc = cblk_aresult(chunks[idx], &ctag[idx][i],
                                      &ar_status, resflags);
                }
                if (rc > 0)
                {
                    cgtag[idx][i++].tag = -1;
                    if (i>=num_cmds) {break;}
                }
                else if (rc == 0) {continue;}
                else
                {
                    fprintf(stderr,"aunmap>aresult error = %d\n",errno);
                    *ret = rc;
                    *err = errno;
                    return;
                }
            }
        }

        startlba+=num_cmds; startlba%=maxlba; lba=startlba;
    }
    for (idx=0; idx<num_opens; idx++)
    {
        if (ctag[idx])  {free(ctag[idx]);}
        if (cgtag[idx]) {free(cgtag[idx]);}
    }
    if (eeh_fail) {*ret=-1;}
    DEBUG_2("Perf Test existing i = %d, x = %d\n", i, x );
    return;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int max_context(int *ret, int *err, int reqs, int cntx, int flags, int mode)
{

    int i               = 0;
    int t               = 0;
    chunk_id_t j        = NULL_CHUNK_ID;
    errno               = 0;
    int status ;

    char *path          = (char*)dev_paths[0];

    pid_t	    child_pid [700];
    errno               = 0;

    if (test_max_cntx) {
        /* use user suppiled cntx value */
        cntx = test_max_cntx;
    }
    DEBUG_1("Testing max_cntx = %d\n", cntx);

    int         ok_to_close = 0;
    int         close_now = 0;
    int         pid;
    int         ret_pid;
    int         child_ret = 0;
    int         child_err = 0;
    int         ret_code,errcode = 0;
    int         fds[2*cntx];
    int         pipefds[2*cntx];

    // create pipe to be used by child  to pass back failure status
    for (i=0; i< (cntx); i++) {
        if (pipe(fds + (i*2) ) < 0 ) {
            perror ("pipe");
            fprintf(stderr, "\nIncrease numfile count with \"ulimit -n 5000\" cmd\n");
            *ret = -1;
            *err = errno;
            return(-1);
        }
    } 


    // create pipes to be used by child  to read permission to close
    for (i=0; i< (cntx); i++) {
        if (pipe(pipefds + (i*2) ) < 0 ) {
            perror ("pipe");
            fprintf(stderr, "\nIncrease numfile count with \"ulimit -n 5000\" cmd\n");
            *ret = -1;
            *err = errno;
            return(-1);
        }
    } 

    DEBUG_0("\nForking childrens to test  max contexts\n ");
    for (i = 0; i < cntx; i++) {
        DEBUG_2("Opening %s opn_cnt = %d\n", path,i);

        child_pid [i] = fork();

        if (child_pid [i] < 0 ) {
            fprintf(stderr,"\nmax_context: fork %d failed err=%d\n",i,errno);
            *ret = -1;
            *err = errno;
            return (-1);
        } else if (child_pid [i] == 0) {
            pid = getpid();
            j = cblk_open(path, reqs, mode, ext, flags);
            if (j != -1) {
                DEBUG_1("\nmax_context: Child = %d, OPENED \n", i+1 );
                close_now = 0;
                while ( !close_now) {
                    read(pipefds[i*2], &close_now, sizeof(int));
                    DEBUG_2("\nChild %d, Received %d \n",pid, close_now);
                }
                DEBUG_3("\nChild %d, Received Parent's OK =%d close id:%d\n",
                        pid, close_now, j);
                cblk_close(j, 0);
                /* exit success */
                exit(0);
            } else {
                DEBUG_3("\nmax_context: child  =%d ret = 0x%x,open error = %d\n",i+1, j, errno);
                child_ret = j;
                child_err = errno;
                /* Send errcode thru ouput side */
                write(fds[(i*2)+1],&child_ret, sizeof(int));
                write(fds[(i*2)+1],&child_err, sizeof(int));

                /* exit error */
                exit(1);
            }
        } 
        DEBUG_1("\nmax_context: loops continue..opened = %d\n",i+1);
    }

    sleep (5);        
    for (i = 0; i < cntx; i++) {
        ok_to_close = 1;
        write(pipefds[(i*2)+1], &ok_to_close, sizeof(int));
        DEBUG_1("\nparent sends ok_to_close to %d \n",i+1);
    }
    ret_code = errcode = 0;
    /* Check all childs exited */
    for (i = 0; i < cntx; i++) {
        for (t = 0; t < 5; t++) {
            ret_pid = waitpid(child_pid[i], &status, 0);
            if (ret_pid == -1) {
                /* error */
                ret_code = -1;
                fprintf(stderr,"\nChild exited with error  %d \n",i);
                break;
            } else if (ret_pid == child_pid[i]) {
                DEBUG_1("\nChild exited  %d \n",i);
                if (WIFEXITED(status)) {
                    DEBUG_2("child =%d, status 0x%x \n",i,status);
                    if (WEXITSTATUS(status)) { 
                        read(fds[(i*2)], &ret_code, sizeof(ret_code));
                        read(fds[(i*2)], &errcode, sizeof(errcode));
                        DEBUG_3("\nchild %d errcode %d, ret_code %d\n",
                                i, errcode, ret_code);
                    } else {
                        DEBUG_1("child =%d, exited normally \n",i);
                    }
                }
                break;
            } else {
                if ( t == 4) {
                    errcode =ETIMEDOUT;
                    fprintf(stderr,"\nChild %d didn't exited Timed out\n",child_pid[i]);
                    break;
                }
                DEBUG_1("\nwaitpid returned = %d, give more time \n",ret_pid);
                DEBUG_1("\nChild %d give need more time \n",i);
                /* still running */
            }
            sleep (1);
        } /* give max 5 sec  */
        /* end test on any error */
        if (ret_code || errcode) {
            DEBUG_3("\nmax_context: Child = %d, ret = 0x%x, err = %d\n",
                    i+1, ret_code,errcode);
            break;
        }
    }

    *ret = ret_code;
    *err = errcode;
    DEBUG_2("ret%d, err:%d\n", *ret, *err);

    // Close all pipes fds 

    for (i=0; i< 2*(cntx); i++) {
        close (pipefds [i]);
    }
    // Close all pipes fds 

    for (i=0; i< 2*(cntx); i++) {
        close (fds [i]);
    }

    return(0);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int child_open(int c, int max_reqs, int flags, int mode)
{
    chunk_id_t j = NULL_CHUNK_ID;
    errno = 0;

    DEBUG_1 ("\nchild_open: opening for child %d\n", c);

    char *path          = (char*)dev_paths[0];

    j = cblk_open (path, max_reqs, mode, ext, flags);
    if (j != NULL_CHUNK_ID) {
        chunks[c] = j;
        num_opens += 1;
        return (0);
    } else {
        /*
         *id = j;
         *er_no = errno;
         */
        DEBUG_2("child_open: Failed: open i = %d, errno = %d\n", c, errno);
        return (-1);
    }
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int fork_and_clone_mode_test(int *ret, int *err, int pmode, int cmode)
{
    chunk_id_t  id      = 0;
    int         flags   = CBLK_OPN_VIRT_LUN;
    int         sz_flags= 0;
    int         max_reqs= 64;
    int         open_cnt= 1;
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
    // 1 = get chunk sz
    // 2 = set chunk sz

    int	    rc;
    int	    er;

    size_t      temp_sz;
    pid_t	    child_pid;
    int	    child_status;
    pid_t	    ret_pid;

    int         child_ret;
    int         child_err;
    int         ret_code=0;
    int         t;
    int         errcode = 0;
    int         fd[2];


    // create pipe to be used by child  to pass back status
    pipe(fd);

    if (blk_fvt_setup(1) < 0)
        return (-1);

    // open virtual  lun
    blk_open_tst( &id, max_reqs, &er, open_cnt, flags, pmode);

    if (id == NULL_CHUNK_ID)
        return (-1); 
    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, sz_flags, get_set_size_flag, &rc, &er);
    if (rc | er) {
        DEBUG_0("fork_and_clone: set_size failed\n");
        *ret = rc;
        *err = er;
        return (0);
    }

    DEBUG_0("\nForking a child process ");
    child_pid = fork(); 
    if (child_pid < 0 ) {
        DEBUG_2("fork_and_clone: fork failed rc=%x,err=%d\n",rc,errno);
        *ret = -1;
        *err = errno;
        return (0);
    }
    if (child_pid == 0) {
        DEBUG_0("\nFork success,Running child process ");

        /* Close read side */
        close (fd[0]);
        // child process 
        rc = cblk_clone_after_fork(id,cmode,0);
        if (rc) {
            DEBUG_2("\nfork_and_clone: clone failed rc=%x, err=%d\n",rc, errno);
            child_ret = rc;
            child_err = errno;
            /* Send errcode thru ouput side */
            write(fd[1],&child_ret, sizeof(int));
            write(fd[1],&child_err, sizeof(int));
            DEBUG_1("Sending child_ret %d\n",child_ret);
            cblk_close(id,0);
            exit (1);
        }

        DEBUG_0("\nfork_and_clone: Exiting child process normally ");
        cblk_close(id,0);
        exit (0);

    } else {
        // parent's process
        DEBUG_0("\nfork_and_clone:Parent waiting for child proc ");
        ret_code = errcode = 0;
        for (t = 0; t < 5; t++) {
            ret_pid = waitpid(child_pid, &child_status, 0);
            if (ret_pid == -1) {
                *ret = -1;
                *err = errno;
                DEBUG_1("\nwaitpid error = %d \n",errno);
                return(0);
            } else if (ret_pid == child_pid) {
                DEBUG_0("\nChild exited Check child_status \n");
                if (WIFEXITED(child_status)) {
                    DEBUG_2("child =%d, child_status 0x%x \n",child_pid,child_status);
                    if (WEXITSTATUS(child_status)) { 
                        read(fd[0], &ret_code, sizeof(ret_code));
                        DEBUG_2("\nchild %d retcode %d\n",
                                child_pid, ret_code);
                        read(fd[0], &errcode, sizeof(errcode));
                        DEBUG_2("\nchild %d errcode %d\n",
                                child_pid, errcode);
                    } else {
                        DEBUG_1("child =%d, exited normally \n",child_pid);
                    }
                }
                break;
            } else {
                if ( t == 4) {
                    ret_code = -1;
                    errcode =ETIMEDOUT;
                    fprintf(stderr,"\nChild %d didn't exited Timed out\n",child_pid);
                    break;
                }
                fprintf(stderr,"\nwaitpid returned = %d, give more time \n",ret_pid);
                /* still running */
            }
            sleep (1);
        } /* give max 5 sec  */

        *ret = ret_code;
        *err = errcode;
    }

    return(0);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int fork_and_clone(int *ret, int *err, int mode, int open_flags, int io_flags)
{
    chunk_id_t  id      = 0;
    int         max_reqs= 64;
    int         open_cnt= 1;
    int         rc;
    int         er;
    uint64_t    lba;
    size_t      temp_sz,nblks;
    int         cmd;
    pid_t       child_pid;
    int         child_status=0;
    pid_t       w_ret;
    int         child_ret;
    int         child_err;
    int         ret_code=0;
    int         ret_err=0;
    int         fd[2];
    int         get_set_size_flag = 0;  // 0 = get phys lun sz
                                        // 1 = get chunk sz
                                        // 2 = set chunk sz

    // create pipe to be used by child  to pass back status
    pipe(fd);

    if (blk_fvt_setup(1) < 0)
        return (-1);

    // open virtual  lun
    blk_open_tst(&id, max_reqs, &er, open_cnt, open_flags, mode);

    if (id == NULL_CHUNK_ID)
        return (-1); 
    temp_sz = 64;
    get_set_size_flag = 2; 
    blk_fvt_get_set_lun_size(id, &temp_sz, io_flags, get_set_size_flag, &rc, &er);
    if (rc | er) {
        DEBUG_0("fork_and_clone: set_size failed\n");
        *ret = rc;
        *err = er;
        return (0);
    }

    cmd = FV_WRITE;
    lba = 1;
    nblks = 1;
    blk_fvt_io(id, cmd, lba, nblks, &rc, &er, io_flags, open_flags);

    if (rc != 1) {
        DEBUG_0("fork_and_clone: blk_fvt_io failed\n");
        *ret = rc;
        *err = er;
        return (0);
    }

    DEBUG_0("\nForking a child process ");
    child_pid = fork(); 
    if (child_pid < 0 ) {
        DEBUG_0("fork_and_clone: fork failed\n");
        *ret = -1;
        *err = errno;
        return (0);
    }
    if (child_pid == 0) {
        DEBUG_0("\nFork success,Running child process ");

        /* Close read side */
        close(fd[0]);

        // child process 
        rc = cblk_clone_after_fork(id,O_RDONLY,open_flags);
        if (rc) {

            DEBUG_2("\nfork_and_clone: clone failed rc=%x, err=%d\n",rc, errno);
            child_ret = rc;
            child_err = errno;
            /* Send errcode thru ouput side */
            write(fd[1],&child_ret, sizeof(int));
            write(fd[1],&child_err, sizeof(int));
            DEBUG_2("Sending child ret=%d, err=%d\n",child_ret, child_err);
            cblk_close(id,0);
            exit (1);
        }

        cmd = FV_READ;
        lba = 1;
        nblks = 1;
        blk_fvt_io(id, cmd, lba, nblks, &rc, &er, io_flags, open_flags);
        if (rc != 1) {
            // error
            DEBUG_0("fork_and_clone: child I/O failed\n");
            child_ret = rc;
            child_err = errno;
            /* Send errcode thru ouput side */
            write(fd[1],&child_ret, sizeof(int));
            write(fd[1],&child_err, sizeof(int));
            DEBUG_2("Sending child ret=%d, err=%d\n",child_ret, child_err);
            cblk_close(id,0);
            exit (1);
        }
        blk_fvt_cmp_buf(nblks, &rc);
        if (rc) {
            // error
            DEBUG_0("fork_and_clone: child I/O compare failed\n");
            child_ret = rc;
            child_err = errno;
            /* Send errcode thru ouput side */
            write(fd[1],&child_ret, sizeof(int));
            write(fd[1],&child_err, sizeof(int));
            DEBUG_2("Sending child ret=%d, err=%d\n",child_ret, child_err);
            cblk_close(id,0);
            exit (1);
        }
        DEBUG_1("fork_and_clone: Child buf compare ret rc = %d\n",rc);
        DEBUG_0("\nfork_and_clone: Exiting child process normally ");
        cblk_close(id,0);
        exit (0);

    } else {
        // parent's process

        DEBUG_0("\nfork_and_clone:Parent waiting for child proc ");
        w_ret = waitpid(child_pid, &child_status, 0);

        if (w_ret == -1) {
            DEBUG_1("fork_and_clone: wait failed %d\n",errno);
            *ret = -1;
            *err = errno;
            return (0);
        } else {

            DEBUG_0("\nfork_and_clone: Child process returned ");
            if (WIFEXITED(child_status)) {
                DEBUG_1("\nfork_and_clone:1 child exit status = 0x%x\n",child_status);
                if (WEXITSTATUS(child_status)) {
                    DEBUG_1("\nfork_and_clone: Error child exit status = 0x%x\n",child_status);
                    /* Close output side */
                    close(fd[1]);
                    rc = read(fd[0], &ret_code, sizeof(ret_code));
                    DEBUG_1("Received child status %d\n",ret_code);
                    rc = read(fd[0], &ret_err, sizeof(ret_err));
                    DEBUG_1("Received child errcode=%d \n",ret_err);
                    *ret = ret_code;
                    *err = ret_err;
                } else {
                    DEBUG_1("\nfork_and_clone: Successfullchild exit status = 0x%x\n",child_status);
                    *ret = rc;
                    *err = errno;
                }
            }
        }
    }

    return(0);
}

/**
********************************************************************************
** \brief
*******************************************************************************/
void blk_list_io_arg_test(chunk_id_t id, int arg_tst, int *err, int *ret)
{
    int 	rc = 0;
    int		i;
    int		num_complt;
    uint64_t	timeout = 0;
    uint64_t	lba = 1;
    int		uflags = 0;
    size_t	size = 1;

    cblk_io_t	cblk_issue_io[1];
    cblk_io_t	*cblk_issue_list[1];
    cblk_io_t	cblk_complete_io[1];
    cblk_io_t	*cblk_complete_list[1];

    cblk_io_t	cblk_wait_io[1];
    cblk_io_t	*cblk_wait_list[1];

    cblk_io_t	cblk_pending_io[1];

    num_complt = 1;

    // allocate buffer for 1 IO

    for (i=0; i<1; i++) {  
        bzero(&cblk_issue_io[i],sizeof(cblk_io_t));
        bzero(&cblk_complete_io[i],sizeof(cblk_io_t)); 
        bzero(&cblk_wait_io[i],sizeof(cblk_io_t));
        bzero(&cblk_pending_io[i],sizeof(cblk_io_t));
    }
    i = 0;
    lba = 1;
    if (arg_tst == 9)
        uflags = CBLK_IO_USER_STATUS;
    else
        uflags = 0;
    size = 1;
    cblk_issue_io[i].request_type = CBLK_IO_TYPE_WRITE;
    cblk_issue_io[i].buf = (void *)(blk_fvt_comp_data_buf );
    cblk_issue_io[i].lba = lba;
    cblk_issue_io[i].flags = uflags;
    cblk_issue_io[i].nblocks = size;
    cblk_issue_list[i] = &cblk_issue_io[i];

    cblk_complete_list[i] = &cblk_complete_io[i];

    cblk_wait_io[i].request_type = CBLK_IO_TYPE_WRITE;
    cblk_wait_io[i].buf = (void *)(blk_fvt_comp_data_buf);
    cblk_wait_io[i].lba = lba;
    cblk_wait_io[i].flags = uflags;
    cblk_wait_io[i].nblocks = size;
    cblk_wait_list[i] = &cblk_wait_io[i];

    cblk_pending_io[i].request_type = CBLK_IO_TYPE_WRITE;
    cblk_pending_io[i].buf = (void *)(blk_fvt_comp_data_buf);
    cblk_pending_io[i].lba = lba;
    cblk_pending_io[i].flags = uflags;
    cblk_pending_io[i].nblocks = size;


    errno = 0;
    switch (arg_tst) {
        case 1:
            /* ALL list NULL */
            rc = cblk_listio(id, 
                    NULL,0, 
                    NULL,0, 
                    NULL,0, 
                    NULL,&num_complt, 
                    timeout,0);
            break;
        case 2:
            rc = cblk_listio(id, 
                    cblk_issue_list, 0, 
                    NULL,0, 
                    NULL,0, 
                    NULL,&num_complt, 
                    timeout,0);
            break;
        case 3: 
            rc = cblk_listio(id, 
                    cblk_issue_list, 1, 
                    NULL,0, 
                    NULL,0, 
                    cblk_complete_list,0, 
                    timeout,0);
            break;
        case 4: 
            rc = cblk_listio(id, 
                    cblk_issue_list, 1, 
                    NULL,0, 
                    NULL,0, 
                    NULL,&num_complt, 
                    timeout,0);
            break;
        case 5: 
            rc = cblk_listio(id, 
                    NULL,0, 
                    NULL,0, 
                    NULL,0, 
                    cblk_complete_list,&num_complt, 
                    timeout,0);
            break;
        case 6: 
            /* Test null buffer report EINVAL error */
            cblk_issue_io[i].buf = (void *)NULL;
            rc = cblk_listio(id, 
                    cblk_issue_list, 1, 
                    NULL,0, 
                    cblk_wait_list,1, 
                    cblk_complete_list,&num_complt, 
                    timeout,0);
            break;
        case 7: 
            /* Test null num_complt report EINVAL */
            num_complt = 0;
            rc = cblk_listio(id, 

                    cblk_issue_list, 1, 
                    NULL,0, 
                    NULL,1, 
                    cblk_complete_list,&num_complt, 
                    timeout,0);
            break;
        case 8: 
            /* Test -1 num_complt report EINVAL */
            num_complt = -1;
            rc = cblk_listio(id, 
                    cblk_issue_list, 1, 
                    NULL,0, 
                    NULL,1, 
                    cblk_complete_list,&num_complt, 
                    timeout,0);
            break;
        case 9: 
            /* Test T/O with CBLK_USER_STATUS  report EINVAL */
            timeout = 1;
            rc = cblk_listio(id, 
                    cblk_issue_list, 1, 
                    NULL,0, 
                    NULL,1, 
                    cblk_complete_list,&num_complt, 
                    timeout,0);
            break;
        default:
            break;
    }

    DEBUG_1("Invalid arg test %d Complete\n",arg_tst);
    *ret = rc;
    *err = errno;
    return;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
void blk_list_io_test(chunk_id_t id, int cmd, int t_type, int uflags, uint64_t timeout, int *err, int *ret, int num_listio)
{
    int rc = 0;
    int i;
    uint64_t lba;
    size_t  size = 1;
    int ret_code = 0;
    int num_complt = num_listio;
    int cmplt_cnt = 0;

    cblk_io_t cblk_issue_io[num_listio];
    cblk_io_t *cblk_issue_list[num_listio];
    cblk_io_t cblk_complete_io[num_listio];
    cblk_io_t *cblk_complete_list[num_listio];

    cblk_io_t cblk_wait_io[num_listio];
    cblk_io_t *cblk_wait_list[num_listio];

    cblk_io_t cblk_pending_io[num_listio];
    cblk_io_t *cblk_pending_list[num_listio];


    for (i=0; i<num_listio; i++) {  
        bzero(&cblk_issue_io[i],sizeof(cblk_io_t));
        bzero(&cblk_complete_io[i],sizeof(cblk_io_t)); 
        bzero(&cblk_wait_io[i],sizeof(cblk_io_t));
        bzero(&cblk_pending_io[i],sizeof(cblk_io_t));
    }

    // create list of cmd I/O

    for (i=0,lba=1; i<num_listio; i++,lba++) {
        if (cmd == FV_WRITE) {
            cblk_issue_io[i].request_type = CBLK_IO_TYPE_WRITE;
            cblk_issue_io[i].buf = (char *)(blk_fvt_comp_data_buf + (i*BLK_FVT_BUFSIZE));
        } else {
            cblk_issue_io[i].request_type = CBLK_IO_TYPE_READ;
            cblk_issue_io[i].buf = (void *)(blk_fvt_data_buf + (i*BLK_FVT_BUFSIZE));
        }
        cblk_issue_io[i].lba = lba;
        cblk_issue_io[i].flags = uflags;
        cblk_issue_io[i].nblocks = size;

        cblk_issue_list[i] = &cblk_issue_io[i];
        cblk_complete_list[i] = &cblk_complete_io[i];
    }

    // create list of wait I/O
    for (i=0,lba=1; i<num_listio; i++,lba++) {
        if (cmd == FV_WRITE) {
            cblk_wait_io[i].request_type = CBLK_IO_TYPE_WRITE;
            cblk_wait_io[i].buf = (void *)(blk_fvt_comp_data_buf + (i*BLK_FVT_BUFSIZE));
        } else {
            cblk_wait_io[i].request_type = CBLK_IO_TYPE_READ;
            cblk_wait_io[i].buf = (void *)(blk_fvt_data_buf + (i*BLK_FVT_BUFSIZE));
        }
        cblk_wait_io[i].lba = lba;
        cblk_wait_io[i].flags = uflags;
        cblk_wait_io[i].nblocks = size;

        cblk_wait_list[i] = &cblk_wait_io[i];
    }

    // create list of pending I/O
    for (i=0,lba=1; i<num_listio; i++,lba++) {
        if (cmd == FV_WRITE) {
            cblk_pending_io[i].request_type = CBLK_IO_TYPE_WRITE;
            cblk_pending_io[i].buf = (void *)(blk_fvt_comp_data_buf + (i*BLK_FVT_BUFSIZE));
        } else {
            cblk_pending_io[i].request_type = CBLK_IO_TYPE_READ;
            cblk_pending_io[i].buf = (void *)(blk_fvt_data_buf + (i*BLK_FVT_BUFSIZE));
        }
        cblk_pending_io[i].lba = lba;
        cblk_pending_io[i].flags = uflags;
        cblk_pending_io[i].nblocks = size;

        cblk_pending_list[i] = &cblk_pending_io[i];
    }


    DEBUG_1("Issue listio cmd = %d\n",cmd);

    if ((uflags & CBLK_IO_USER_STATUS) && !timeout) {
        /* issue listio and poll issue list for completions */
        rc = cblk_listio(id, 
                cblk_issue_list, num_listio, 
                NULL,0, 
                NULL,0, 
                cblk_complete_list,&num_complt, 
                timeout,0);
    } else if (t_type == 2) {
        /* USER_STATUS _not_ set */
        /* issue listio and poll waitlist for completions */
        rc = cblk_listio(id, 
                cblk_issue_list, num_listio, 
                NULL,0, 
                cblk_wait_list, num_listio, 
                cblk_complete_list,&num_complt, 
                timeout,0);
    } else if (t_type == 3){
        /* USER_STATUS _not_ set */
        /* Issue listio with issue and compl list, followed by pending list 
         * and completion with num_complt
         */
        rc = cblk_listio(id, 
                cblk_issue_list, num_listio, 
                NULL,0, 
                NULL, 0, 
                cblk_complete_list,&num_complt, 
                timeout,0);
        if (!rc) {
            num_complt = num_listio;

            bcopy (&cblk_issue_io[0], &cblk_pending_io[0], ((sizeof(cblk_io_t))*num_listio));
            while (TRUE) {
                /* Issue listio  with pending and completion list , and then
                 * check for completion list for cmds being complete,
                 * Re-issue listio with pending lists
                 */
                rc = cblk_listio(id, 
                        NULL,0, 
                        cblk_pending_list, num_listio, 
                        NULL, 0, 
                        cblk_complete_list,&num_complt, 
                        timeout,0);

                if (!rc) {
                    /* returns num compltions */
                    ret_code = check_completions(&cblk_complete_io[0],num_listio);
                    /* returns num complts. or -1 for error*/
                    /* if all complete or any failed , exit */
                    if (ret_code == -1) {
                        rc = ret_code;
                        fprintf(stderr, "List io failed cmplt_cnt =%d\n",cmplt_cnt);
                        break;
                    } else {
                        cmplt_cnt += ret_code;
                        if (cmplt_cnt == num_listio) {
                            rc = 0;
                            DEBUG_1("\ncmplt_cnt = %d\n",cmplt_cnt);
                            break;
                        }
                    }
                    /* re-issue with pending count */
                    num_complt = num_listio;
                } else {
                    fprintf(stderr, "Type 3 listio test failed, errno =0x%x\n",errno);
                    break;
                }
            }
        } else {
            fprintf(stderr,"cblk_listio cmd = %d failed , compl_items =%d, rc = %d, errno = %d\n",cmd, cmplt_cnt, rc, errno);
            *ret = rc;
            *err = errno;
            return ;
        }
    }

    if (!rc) {
        if (uflags & CBLK_IO_USER_STATUS) {
            DEBUG_1("\nPolling issue io for cmd type = %d \n",cmd);
            /* if caller is providing a status location */
            rc = poll_arw_stat(&cblk_issue_io[0],num_listio); 
        } else if (t_type == 2) {
            DEBUG_1("\nPolling wait io for cmd type = %d \n",cmd);
            rc = poll_arw_stat(&cblk_wait_io[0],num_listio); 

        }
    }
    if (rc == -1) {
        fprintf(stderr,"listio cmd = %d failed rc = %x, err = %x\n", cmd, rc, errno);
        if ((errno == ETIMEDOUT) && (t_type == 2)) {
            /* This force 1 micro sec t/o test, so wait for cmd completion */
            poll_arw_stat(&cblk_wait_io[0],num_listio); 
        }
    }
    DEBUG_1("\nlistio cmd type = %d complete\n",cmd);

    *ret = rc;
    *err = errno; 
    return;
}

/**
********************************************************************************
** \brief
*******************************************************************************/
int poll_arw_stat(cblk_io_t *io, int num_listio)
{

    int i=0;
    int cmds_fail,cmds_invalid,cmds_cmplt;
    cmds_fail=cmds_invalid=cmds_cmplt=0;
    int ret = -1;
    int sleep_cnt = 0;

    DEBUG_0("Poll completions ...\n");
    DEBUG_2("Enetering poll_arw_stat: , slept = %d, all cmds %d done\n",sleep_cnt, cmds_cmplt);

    while (TRUE) {
        for (i=0; i<num_listio; i++) {
            switch (io[i].stat.status) {
                case CBLK_ARW_STATUS_FAIL:
                    fprintf(stderr," %d req failed, err %d, xfersize =0x%lx\n",
                        i,io[i].stat.fail_errno, io[i].stat.blocks_transferred);
                    cmds_fail ++;
                    /* mark it counted */
                    io[i].stat.status = -1;
                    errno = io[i].stat.fail_errno ;
                    break;
                case CBLK_ARW_STATUS_PENDING:
                    DEBUG_3(" %d req pending, err %d, xfersize =0x%lx\n",
                        i,io[i].stat.fail_errno, io[i].stat.blocks_transferred);
                    break;
                case CBLK_ARW_STATUS_SUCCESS:
                    DEBUG_3("\n %d req success, err %d, xfersize =%lx\n\n",
                        i,io[i].stat.fail_errno, io[i].stat.blocks_transferred);
                    cmds_cmplt ++;
                    /* mark it counted */
                    io[i].stat.status = -1;
                    break;
                case CBLK_ARW_STATUS_INVALID:
                    fprintf(stderr," %d req invalid, err %d, xfersize =%lx\n",
                        i,io[i].stat.fail_errno, io[i].stat.blocks_transferred);
                    cmds_invalid ++;
                    break;
                default:
                    /* if this is marked counted */
                    if ((io[i].stat.status) == -1)
                        break;
                    else
                        fprintf(stderr,"Invalid status %x\n",
                                            (io[i].stat.status)); 
                    break;
            }

        }
        if (cmds_fail ) {
            if ((cmds_fail + cmds_cmplt) == num_listio) {
                fprintf(stderr, "Exiting poll_arw_stat with erreno\n");
                return (ret);
            }
        }
        if ( cmds_cmplt == num_listio) {
            DEBUG_2("Exiting poll_arw_stat: , slept = %d, all cmds %d done\n",
                            sleep_cnt, cmds_cmplt);
            ret = 0;
            return(ret);
        }
        if (sleep_cnt > 50) {
            fprintf(stderr, "Waited 50 sec, fail = %d, cmplt =%d\n",
                            cmds_fail,cmds_cmplt);
            break;
        }
        sleep_cnt ++;
        sleep(1);
    }

    return (ret);

}

/**
********************************************************************************
** \brief
*******************************************************************************/
int check_completions(cblk_io_t *io, int num_listio )
{
    int i;
    int cmds_fail,cmds_invalid,cmds_cmplt,cmds_pend;
    cmds_fail=cmds_invalid=cmds_cmplt=cmds_pend=0;


    for (i=0; i<num_listio; i++) {
        switch (io[i].stat.status) {
            case CBLK_ARW_STATUS_FAIL:
                fprintf(stderr," %d req failed, err %d, xfersize =%lx\n",
                    i,io[i].stat.fail_errno, io[i].stat.blocks_transferred);
                cmds_fail ++;
                // mark it counted
                io[i].stat.status = -1;
                errno = io[i].stat.fail_errno ;
                break;
            case CBLK_ARW_STATUS_PENDING:
                DEBUG_3(" %d req pending, err %d, xfersize =0x%lx\n",
                    i,io[i].stat.fail_errno, io[i].stat.blocks_transferred);
                cmds_pend ++;
                break;
            case CBLK_ARW_STATUS_SUCCESS:
                DEBUG_3(" %d req success, err %d, xfersize =%lx\n",
                    i,io[i].stat.fail_errno, io[i].stat.blocks_transferred);
                cmds_cmplt ++;
                // mark it counted
                io[i].stat.status = -1;
                break;
            case CBLK_ARW_STATUS_INVALID:
                fprintf(stderr," %d req invalid, err %d, xfersize =%lx\n",
                       i,io[i].stat.fail_errno, io[i].stat.blocks_transferred);
                cmds_invalid ++;
                break;
            default:
                /*
                   fprintf(stderr, "Invalid status\n"); 
                 */
                break;
        }

    }
    if (cmds_fail) {
        fprintf(stderr, "pend = %d, fail = %d, cmplt =%d\n",
                    cmds_pend, cmds_fail,cmds_cmplt);
        return (-1);
    } 
    if (cmds_cmplt ) {
        DEBUG_3("pend = %d, fail = %d, cmplt =%d\n",
                        cmds_pend, cmds_fail,cmds_cmplt);
        return(cmds_cmplt); 
    }

    return (0);

}

/**
********************************************************************************
** \brief
*******************************************************************************/
#ifdef _AIX
char *find_parent(char *device_name)
{

    return NULL;
}
char *getunids (char *device_name)
{

    return NULL;
}
#else

char *find_parent(char *device_name)
{
    char *parent_name = NULL;
    char *child_part = NULL;
    const char *device_mode = NULL;
    char *subsystem_name = NULL;
    char *devname = NULL;


    struct udev *udev_lib;
    struct udev_device *device, *parent;


    udev_lib = udev_new();

    if (udev_lib == NULL) {
        DEBUG_1("udev_new failed with errno = %d\n",errno);
        fprintf(stderr,"udev_open failed\n");
        return parent_name;
    }

    /*
     * Extract filename with absolute path removed
     */
    devname = rindex(device_name,'/');
    if (devname == NULL) {
        devname = device_name;
    } else {
        devname++;
        if (devname == NULL) {
            DEBUG_1("\ninvalid device name = %s",devname);
            return parent_name;
        }
    }

    if (!(strncmp(devname,"sd",2))) {
        subsystem_name = "block";
    } else if (!(strncmp(devname,"sg",2))) {
        subsystem_name = "scsi_generic";
    } else {
        DEBUG_1("\ninvalid device name = %s",devname);
        return parent_name;
    }


    device = udev_device_new_from_subsystem_sysname(udev_lib,subsystem_name,devname);
    if (device == NULL) {
        DEBUG_1("\n udev_device_new_from_subsystem_sysname failed with errno = %d",errno);
        return parent_name;
    }

    device_mode = udev_device_get_sysattr_value(device,"mode");
    if (device_mode == NULL) {

        DEBUG_0("\nno mode for this device ");
        // return parent_name;
    }
    parent =  udev_device_get_parent(device);
    if (parent == NULL) {
        DEBUG_1("\n udev_device_get_parent failed with errno = %d",errno);
        return parent_name;
    }

    parent_name = (char *)udev_device_get_devpath(parent);


    /*
     * This parent name string will actually have sysfs directories
     * associated with the connection information of the child. We
     * need to get the base name of parent device that is not associated with
     * child device.  Find child portion of parent name string
     * and insert null terminator there to remove it.
     */

    child_part = strstr(parent_name,"/host");
    if (child_part) {
        child_part[0] = '\0';
    }
    return parent_name;
}

#endif /* !AIX */
