/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/ea.c $                                                 */
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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include "bl.h"
#include "ea.h"
#include "am.h"

#include <kv_inject.h>
#include <arkdb_trace.h>
#include <errno.h>

static int cflsh_blk_lib_init = 0;

#ifndef _AIX

// fetch_and_or is not available for linux user space.
// It appears that __sync_fetch_and_or is doing the
// required functionality.


#define fetch_and_or(ptr,val) (__sync_fetch_and_or(ptr,val))


#endif /* !_AIX */


EA *ea_new(const char *path, uint64_t bsize, int basyncs,
           uint64_t *size, uint64_t *bcount, uint64_t vlun)
{
    int             rc    = 0;
    size_t          plen  = 0;
    uint8_t        *store = NULL;
    EA             *ea    = NULL;
    chunk_id_t      chkid = NULL_CHUNK_ID;

    if (!(fetch_and_or(&cflsh_blk_lib_init,1)))
    {
        // We need to call cblk_init once before
        // we use any other cblk_ interfaces
        rc = cblk_init(NULL,0);
        if (rc)
        {
            KV_TRC_FFDC(pAT, "cblk_init failed path %s bsize %"PRIu64" "
                             "size %"PRIu64" bcount %"PRIu64", errno = %d",
                             path, bsize, *size, *bcount, errno);
            goto error_exit;
        }
    }

    ea = am_malloc(sizeof(EA));
    if (NULL == ea)
    {
        KV_TRC_FFDC(pAT, "Out of memory path %s bsize %"PRIu64" size %"PRIu64" "
                         "bcount %"PRIu64", errno = %d",
                         path, bsize, *size, *bcount, errno);
        goto error_exit;
    }

    // We need to check the path parameter to see if
    // we are going to use memory or a file/capi
    // device (to be determined by the block layer)
    if ( (NULL == path) || (strlen(path) == 0) )
    {
        KV_TRC(pAT, "EA_STORE_TYPE_MEMORY");
        // Using memory for store
        ea->st_type = EA_STORE_TYPE_MEMORY;

        store = malloc(*size);
        if (NULL == store)
        {
            errno = ENOMEM;
            KV_TRC_FFDC(pAT, "Out of memory for store path %s bsize %"PRIu64" "
                             "size %"PRIu64" bcount %"PRIu64", errno = %d",
                             path, bsize, *size, *bcount, errno);
            goto error_exit;
        }

        *bcount = ((*size) / bsize);
        ea->st_memory = store;
    }
    else
    {
        KV_TRC(pAT, "EA_STORE_TYPE_FILE(%s)", path);

        // Using a file.  We don't care if it's an actual
        // file or a CAPI device, we let block layer
        // decide and we just use the chunk ID that is
        // passed back from the cblk_cg_open call.
        ea->st_type = EA_STORE_TYPE_FILE;

        // Check to see if we need to create the store on a
        // physical or virtual LUN.  Previously, in GA1,
        // we keyed off the size and if it was 0, then we
        // asked for the LUN to be physical.  Now, the user
        // can specify with a flag.
        if ( vlun == 0 )
        {
            KV_TRC(pAT, "cblk_cg_open PHYSICAL LUN: %s", path);
            chkid = cblk_cg_open(path, basyncs, O_RDWR, 1, 0,
                                 CBLK_GROUP_RAID0|CBLK_OPN_NO_INTRP_THREADS);

            if (NULL_CHUNK_ID == chkid)
            {
                KV_TRC_FFDC(pAT, "cblk_cg_open phys lun failed path:%s bsize:%ld "
                                 "size:%ld bcount:%ld, errno:%d",
                                 path, bsize, *size, *bcount, errno);
                goto error_exit;
            }

            rc = cblk_cg_get_size(chkid, (size_t *)bcount, CBLK_GROUP_RAID0);
            if ( (rc != 0) || (*bcount == 0) )
            {
                // An error was encountered, close the chunk
                cblk_cg_close(chkid, CBLK_GROUP_RAID0);
                chkid = NULL_CHUNK_ID;
                KV_TRC_FFDC(pAT, "cblk_cg_get_size failed path %s bsize %"PRIu64" "
                                 "size %"PRIu64" bcount %"PRIu64", errno = %d",
                                 path, bsize, *size, *bcount, errno);
                goto error_exit;
            }

            // Set the size to be returned
            *size = *bcount * bsize;
        }
        else
        {
            KV_TRC(pAT, "cblk_cg_open VIRTUAL LUN: %s", path);
            chkid = cblk_cg_open(path, basyncs, O_RDWR, 1, 0,
                  CBLK_GROUP_RAID0|CBLK_OPN_VIRT_LUN|CBLK_OPN_NO_INTRP_THREADS);

            if (NULL_CHUNK_ID == chkid)
            {
                KV_TRC_FFDC(pAT, "cblk_cg_open virt lun failed path:%s bsize:%ld "
                                 "size:%ld bcount:%ld, errno:%d",
                                 path, bsize, *size, *bcount, errno);
                goto error_exit;
            }

            // A specific size was passed in so we try to set the
            // size of the chunk.
            *bcount = *size / bsize;
            rc = cblk_cg_set_size(chkid, (size_t)*bcount, CBLK_GROUP_RAID0);
            if ( rc != 0 )
            {
                // An error was encountered, close the chunk
                cblk_cg_close(chkid, CBLK_GROUP_RAID0);
                chkid = NULL_CHUNK_ID;
                KV_TRC_FFDC(pAT, "cblk_cg_set_size failed path %s bsize %"PRIu64" "
                                 "size %"PRIu64" bcount %"PRIu64", errno = %d",
                                 path, bsize, *size, *bcount, errno);
                goto error_exit;
            }
        }

        // Save off the chunk ID and the device name
        ea->st_flash  = chkid;
        plen          = strlen(path) + 1;
        ea->st_device = (char *)am_malloc(plen);
        if (!ea->st_device)
        {
            cblk_cg_close(chkid, CBLK_GROUP_RAID0);
            KV_TRC_FFDC(pAT, "MALLOC st_device failed (%s) plen=%ld errno:%d",
                        path, plen, errno);
            goto error_exit;
        }

        memset(ea->st_device, 0, plen);
        strncpy(ea->st_device, path, plen);
    }

    // Fill in the EA struct
    pthread_rwlock_init(&(ea->ea_rwlock), NULL);
    ea->bsize  = bsize;
    ea->bcount = *bcount;
    ea->size   = *size;

    KV_TRC(pAT, "path %s bsize %"PRIu64" size %"PRIu64" bcount %"PRIu64"",
           path, bsize, *size, *bcount);
    goto done;

error_exit:
    am_free(ea);
    ea = NULL;
    if (!errno) {KV_TRC_FFDC(pAT, "UNSET_ERRNO"); errno=ENOSPC;}

done:
    return ea;
}

int ea_resize(EA *ea, uint64_t bsize, uint64_t bcount)
{
  uint64_t size = bcount * bsize;
  int rc        = 0;

  ARK_SYNC_EA_WRITE(ea);

  if ( ea->st_type == EA_STORE_TYPE_MEMORY )
  {
    // For an in-memory store, we simply "realloc"
    // the memory.
    uint8_t *store = realloc(ea->st_memory, size);
    if (store) {
      ea->bcount = bcount;
      ea->size = size;
      ea->st_memory = store;
    } 
    else {
      errno = ENOMEM;
      KV_TRC_FFDC(pAT, "ENOMEM, resize ea %p bsize %lu bcount %lu, errno = %d",
              ea, bsize, bcount, errno);
      rc = 1;
    }
  }
  else
  {
    // Call down to the block layer to set the
    // new size on the store.
    rc = cblk_cg_set_size(ea->st_flash, bcount, CBLK_GROUP_RAID0);
    if (rc == 0)
    {
      ea->bcount = bcount;
      ea->size = size;
    }
    else
    {
        errno = ENOSPC;
        KV_TRC_FFDC(pAT, "cblk_cg_set_size failed ea %p bsize %lu bcount %lu, "
                         "errno = %d",
                         ea, bsize, bcount, errno);
    }
  }

  ARK_SYNC_EA_UNLOCK(ea);

  return rc;
}

int ea_read(EA *ea, uint64_t lba, void *dst) {
  uint8_t *src = NULL;
  int      rc  = 0;

  if ( ea->st_type == EA_STORE_TYPE_MEMORY)
  {
    // Read out the value from the in-memor block
    src = ea->st_memory + lba * ea->bsize;
    rc = ( memcpy(dst, src, ea->bsize) == NULL ? 0 : 1);
  }
  else
  {
    // Call out to the block layer and retrive a block
    rc = cblk_cg_read(ea->st_flash, dst, lba, 1, CBLK_GROUP_RAID0);
  }

  return rc;
}

int ea_write(EA *ea, uint64_t lba, void *src) {
  uint8_t *dst = NULL;
  int      rc  = 0;

  if ( ea->st_type == EA_STORE_TYPE_MEMORY)
  {
    // Write the value to the in-memor block
    dst = ea->st_memory + lba * ea->bsize;
    rc = ( memcpy(dst, src, ea->bsize) == NULL ? 0 : 1);
  }
  else
  {
    // Send the value down to the block layer, 1 block
    rc = cblk_cg_write(ea->st_flash, src, lba, 1, CBLK_GROUP_RAID0);
  }

  return rc;
}

int ea_async_io(EA *ea, int op, void *addr, ark_io_list_t *blist, int64_t len, int nthrs)
{
  int64_t  i       = 0;
  int64_t  j       = 0;
  int64_t  comps   = 0;
  int      num     = 0;
  int      max_ops = 0;
  void    *m_rc    = NULL;
  int      rc      = 0;
  int      a_rc    = 0;
  uint64_t status  = 0;
  uint8_t *p_addr  = NULL;
  uint8_t *m_addr  = NULL;
  char     *ot     = NULL;

  ARK_SYNC_EA_READ(ea);

  if (op == ARK_EA_READ) {ot="IO_RD";}
  else                   {ot="IO_WR";}

  if ( ea->st_type == EA_STORE_TYPE_MEMORY)
  {
    // Loop through the block list to issue the IO
    for(i = 0; i < len; i++)
    {

      p_addr = ((uint8_t*)addr) + (i * ea->bsize);

      // For in-memory Store, we issue the memcpy
      // and wait for the return, no async here.
      // Read out the value from the in-memor block
      m_addr = ea->st_memory + (blist[i].blkno * ea->bsize);

      if (op == ARK_EA_READ) {m_rc = memcpy(p_addr, m_addr, ea->bsize);}
      else                   {m_rc = memcpy(m_addr, p_addr, ea->bsize);}

      if (check_sched_error_injects(op)) {m_rc=NULL;}
      if (check_harv_error_injects(op))  {m_rc=NULL;}

      if (m_rc == NULL)
      {
          rc = errno;
          break;
      }
    }
  }
  else
  {
    // divide up the cmd slots among
    // the threads and go 3 less
    max_ops = (ARK_EA_BLK_ASYNC_CMDS / nthrs) - 3;

    // Loop through the block list to issue the IO
    while ((comps < len) && (rc == 0))
    {
      for(i = comps, num = 0; 
             (i < len) && (num < max_ops); 
              i++, num++)
      {
        p_addr = ((uint8_t*)addr) + (i * ea->bsize);

        // Call out to the block layer and retrive a block
        // Do an async op for a single block and tell the block
        // layer to wait if there are no available command
        // blocks.  Upon return, we can either get an error
        // (rc == -1), the data will be available (rc == number
        // of blocks read), or IO has been scheduled (rc == 0).
        if (op == ARK_EA_READ)
        {
            rc = cblk_cg_aread(ea->st_flash, p_addr, blist[i].blkno, 1,
             &(blist[i].a_tag), NULL, CBLK_GROUP_RAID0|CBLK_ARW_WAIT_CMD_FLAGS);
        }
        else
        {
            rc = cblk_cg_awrite(ea->st_flash, p_addr, blist[i].blkno, 1,
              &(blist[i].a_tag), NULL,CBLK_GROUP_RAID0|CBLK_ARW_WAIT_CMD_FLAGS);
        }

        if (check_sched_error_injects(op)) {rc=-1;}

        KV_TRC_IO(pAT, "%s:  id:%d blkno:%"PRIi64" rc:%d",
                ot, ea->st_flash, blist[i].blkno, rc);

        if ( rc == -1 )
        {
          // Error was encountered.  Don't issue any more IO
          rc = errno;
          KV_TRC_FFDC(pAT, "IO_ERR: cblk_cg_aread/awrite failed, "
                           "blkno:%"PRIi64" tag:%d, errno = %d",
                           blist[i].blkno, blist[i].a_tag.tag, errno);
          break;
        }

        // Data has already been returned so we don't need to
        // wait for the response below
        if ( rc > 0 )
        {
          blist[i].a_tag.tag = -1;
          rc = 0;
        }
        //_arkp->stats.io_cnt++;
      }

      // For as many IOs that were performed, we loop t
      // see if we need to wait for the response or the
      // data has already been returned.
      for (j = comps; j < i; j++)
      {

        // Data has already been read
        if (blist[j].a_tag.tag == -1)
        {
          continue;
        }

        do
        {
          a_rc = cblk_cg_aresult(ea->st_flash, &(blist[j].a_tag),
                               &status, CBLK_GROUP_RAID0|CBLK_ARESULT_BLOCKING);

          if (check_harv_error_injects(op))  {a_rc=-1;}

          // There was an error, check to see if we haven't
          // encoutnered an error previously and if not, then
          // set rc.  Continue processing so that we harvest
          // all outstanding responses
          if (a_rc == -1)
          {
            if (rc == 0)
            {
              rc = errno;
            }
            KV_TRC_IO(pAT, "IO_ERR: id:%d blkno:%ld status:%ld a_rc:%d",
                    ea->st_flash, blist[j].blkno, status, a_rc);
          }
          else
          {
              KV_TRC_IO(pAT, "IO_CMP: id:%d blkno:%ld status:%ld a_rc:%d",
                      ea->st_flash, blist[j].blkno, status, a_rc);
          }

          // If a_rc is 0, that means we got interrupted somehow
          // so we need to retry the operation.
        } while (a_rc == 0);
      }

      // If we start another loop, start off where we finished
      // in this loop.
      comps = i;
    }
  }

  ARK_SYNC_EA_UNLOCK(ea);

  return rc;
}

int ea_delete(EA *ea)
{
    int rc = 0;

    if ( ea->st_type == EA_STORE_TYPE_MEMORY )
    {
        KV_TRC(pAT, "free ea %p ea->st_memory %p", ea, ea->st_memory);
        // Simple free the block of store
        if (ea->st_memory)
        {
            free(ea->st_memory);
            ea->st_memory=NULL;
        }
    }
    else
    {
        // Call to close out the chunk and free the space
        // for the device name
        rc = cblk_cg_close(ea->st_flash, CBLK_GROUP_RAID0);
        am_free(ea->st_device);
    }

    if ( rc == 0 )
    {
        KV_TRC(pAT, "free ea %p", ea);
        am_free(ea);
    }

    return rc;
}

