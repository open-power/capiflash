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
#include <errno.h>
#include <fcntl.h>

#include "bl.h"
#include "ea.h"
#include "capiblock.h"
#include "am.h"

#include <test/fvt_kv_inject.h>
#include <arkdb_trace.h>

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
  int      rc    = 0;
  size_t   plen  = 0;
  uint8_t *store = NULL;
  EA      *ea    = NULL;
  chunk_id_t chkid = NULL_CHUNK_ID;
  chunk_ext_arg_t ext = 0;



  if (!(fetch_and_or(&cflsh_blk_lib_init,1))) {


      // We need to call cblk_init once before 
      // we use any other cblk_ interfaces

      rc = cblk_init(NULL,0);

      if (rc) {

	  KV_TRC_FFDC(pAT, "cblk_init failed path %s bsize %"PRIu64" size %"PRIu64" bcount %"PRIu64", errno = %d",
		      path, bsize, *size, *bcount, errno);

	  return ea;
      }
  }

  ea = am_malloc(sizeof(EA));
  if ( NULL == ea )
  {
    errno = ENOMEM;
    KV_TRC_FFDC(pAT, "Out of memory path %s bsize %"PRIu64" size %"PRIu64" bcount %"PRIu64", errno = %d",
            path, bsize, *size, *bcount, errno);
  }
  else
  {
    // We need to check the path parameter to see if
    // we are going to use memory or a file/capi
    // device (to be determined by the block layer)
    if ( (NULL == path) || (strlen(path) == 0) )
    {
      // Using memory for store
      ea->st_type = EA_STORE_TYPE_MEMORY;

      store = malloc(*size);
      if ( NULL == store )
      {
        errno = ENOMEM;
        KV_TRC_FFDC(pAT, "Out of memory for store path %s bsize %"PRIu64" size %"PRIu64" bcount %"PRIu64", errno = %d",
                path, bsize, *size, *bcount, errno);
        am_free(ea);
        ea = NULL;
      }
      else
      {
        *bcount = ((*size) / bsize);
        ea->st_memory = store;
      }
    }
    else
    {

      
      // Using a file.  We don't care if it's an actual
      // file or a CAPI device, we let block layer
      // decide and we just use the chunk ID that is
      // passed back from the cblk_open call.
      ea->st_type = EA_STORE_TYPE_FILE;
          
      // Check to see if we need to create the store on a
      // physical or virtual LUN.  Previously, in GA1,
      // we keyed off the size and if it was 0, then we
      // asked for the LUN to be physical.  Now, the user
      // can specify with a flag.
      if ( vlun == 0 )
      {
        chkid = cblk_open((const char *)path, 
                           basyncs, O_RDWR, ext, 0);
        if ( NULL_CHUNK_ID != chkid )
        {
          rc = cblk_get_size(chkid, (size_t *)bcount, 0);
          if ( (rc != 0) || (*bcount == 0) )
          {
            // An error was encountered, close the chunk

            // If we are here, and the call to cblk_get_size() 
            // was successful then that means bcount is 0
            if ( rc == 0 )
            {
              rc = ENOSPC;
            }

            cblk_close(chkid, 0);
            chkid = NULL_CHUNK_ID;
            KV_TRC_FFDC(pAT, "cblk_get_size failed path %s bsize %"PRIu64" size %"PRIu64""
                        "bcount %"PRIu64", errno = %d",
                        path, bsize, *size, *bcount, errno);
          }
          else
          {
            // Set the size to be returned
            *size = *bcount * bsize;
          }
        }
      }
      else
      {
        chkid = cblk_open((const char *)path, basyncs, O_RDWR, ext, 
                             CBLK_OPN_VIRT_LUN);
        if ( NULL_CHUNK_ID != chkid )
        {
          // A specific size was passed in so we try to set the
	  // size of the chunk.
          *bcount = *size / bsize;
          rc = cblk_set_size(chkid, (size_t)*bcount, 0);
          if ( rc != 0 )
          {
            // An error was encountered, close the chunk
            cblk_close(chkid, 0);
            chkid = NULL_CHUNK_ID;
            KV_TRC_FFDC(pAT, "cblk_set_size failed path %s bsize %"PRIu64" size %"PRIu64""
                        " bcount %"PRIu64", errno = %d",
                        path, bsize, *size, *bcount, errno);
          }
        }
      }

      if ( NULL_CHUNK_ID == chkid )
      {
        printf("cblk_open failed\n");
        am_free(ea);
        ea = NULL;
        KV_TRC_FFDC(pAT, "cblk_open failed path %s bsize %"PRIu64" size %"PRIu64""
	            " bcount %"PRIu64", errno = %d",
	            path, bsize, *size, *bcount, errno);
      }
      else
      {
        // Save off the chunk ID and the device name
        ea->st_flash = chkid;
        plen = strlen(path) + 1;
        ea->st_device = (char *)am_malloc(plen);
        memset(ea->st_device, 0, plen);
        strncpy(ea->st_device, path, plen);
      }
    }

    if (ea != NULL)
    {
      // Fill in the EA struct
      pthread_rwlock_init(&(ea->ea_rwlock), NULL);
      ea->bsize = bsize;
      ea->bcount = *bcount;
      ea->size = *size;
    }
  }

  KV_TRC(pAT, "path %s bsize %"PRIu64" size %"PRIu64" bcount %"PRIu64"",
          path, bsize, *size, *bcount);
  return ea;
}

int ea_resize(EA *ea, uint64_t bsize, uint64_t bcount) {
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
      KV_TRC_FFDC(pAT, "Out of memory to resize ea %p bsize %lu bcount %lu, errno = %d",
              ea, bsize, bcount, errno);
      rc = 1;
    }
  }
  else
  {
    // Call down to the block layer to set the
    // new size on the store.
    rc = cblk_set_size(ea->st_flash, bcount, 0);
    if (rc == 0)
    {
      ea->bcount = bcount;
      ea->size = size;
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
    rc = cblk_read(ea->st_flash, dst, lba, 1, 0);
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
    rc = cblk_write(ea->st_flash, src, lba, 1, 0);
  }

  return rc;
}

int ea_async_io(EA *ea, int op, void *addr, ark_io_list_t *blist, int64_t len, int nthrs)
{
  int64_t  i = 0;
  int64_t  j = 0;
  int64_t  comps  = 0;
  int      num = 0;
  int      max_ops = 0;
  int     rc = 0;
  int     a_rc = 0;
  uint64_t status = 0;
  uint8_t *p_addr = NULL;
  uint8_t *m_addr = NULL;

  ARK_SYNC_EA_READ(ea);

  if ( ea->st_type == EA_STORE_TYPE_MEMORY)
  {
    // Loop through the block list to issue the IO
    for(i = 0; i < len; i++) {

      p_addr = ((uint8_t*)addr) + (i * ea->bsize);

      // For in-memory Store, we issue the memcpy
      // and wait for the return, no async here.
      // Read out the value from the in-memor block
      m_addr = ea->st_memory + (blist[i].blkno * ea->bsize);

      if (op == ARK_EA_READ)
      {
        if (FVT_KV_READ_ERROR_INJECT)
        {
            FVT_KV_CLEAR_READ_ERROR; rc = errno = EIO;
            KV_TRC_FFDC(pAT, "READ_ERROR_INJECT rc = %d", EIO);
            break;
        }
        if ( memcpy(p_addr, m_addr, ea->bsize) == NULL )
        {
          rc = errno;
          break;
        }
      }
      else
      {
        if (FVT_KV_WRITE_ERROR_INJECT)
        {
            FVT_KV_CLEAR_WRITE_ERROR; rc = errno = EIO;
            KV_TRC_FFDC(pAT, "WRITE_ERROR_INJECT rc = %d", EIO);
            break;
        }
        if ( memcpy(m_addr, p_addr, ea->bsize) == NULL )
        {
          rc = errno;
	  break;
        }
      }
    }
  }
  else
  {
    // Because we have 4 pool threads, we want to ensure
    // that at any given time, if all threads are running
    // large K/V operations, we don't hang because
    // we exhausted the async command slots in the block
    // layer. So we divide up the cmd slots among
    // the 4 threads and go 1 more less to be sure
    max_ops = (ARK_EA_BLK_ASYNC_CMDS / nthrs) - 1;

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
        if (op== ARK_EA_READ)
        {
            if (FVT_KV_READ_ERROR_INJECT)
            {
                FVT_KV_CLEAR_READ_ERROR; rc = errno = EIO;
                KV_TRC_FFDC(pAT, "READ_ERROR_INJECT rc = %d", EIO);
                break;
            }

            KV_TRC_IO(pAT, "RD: id:%d blkno:%"PRIi64"",
                    ea->st_flash,
                    blist[i].blkno);
          //printf("cblk_aread for block: %"PRIu64"\n", blist[i].blkno);
          rc = cblk_aread(ea->st_flash, p_addr, blist[i].blkno, 1, 
			  &(blist[i].a_tag), NULL,CBLK_ARW_WAIT_CMD_FLAGS);
        }
        else
        {
            if (FVT_KV_WRITE_ERROR_INJECT)
            {
                FVT_KV_CLEAR_WRITE_ERROR; rc = errno = EIO;
                KV_TRC_FFDC(pAT, "WRITE_ERROR_INJECT rc = %d", EIO);
                break;
            }

            KV_TRC_IO(pAT, "WR: id:%d blkno:%"PRIi64"",
                    ea->st_flash,
                    blist[i].blkno);
          rc = cblk_awrite(ea->st_flash, p_addr, blist[i].blkno, 1, 
                        &(blist[i].a_tag), NULL,CBLK_ARW_WAIT_CMD_FLAGS);
        }

        if ( rc == -1 )
        {
          // Error was encountered.  Don't issue any more IO
          rc = errno;
          KV_TRC_FFDC(pAT, "cblk_aread/awrite failed, IO ERROR, blkno:%"PRIi64"\
 tag:%d, errno = %d", blist[i].blkno, blist[i].a_tag, errno);
          break;
        }

        // Data has already been returned so we don't need to
        // wait for the response below
        if ( rc > 0 )
        {
          blist[i].a_tag = -1;
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
        if (blist[j].a_tag == -1)
        {
          continue;
        }

        do
	{
          a_rc = cblk_aresult(ea->st_flash, &(blist[j].a_tag), 
                            &status, CBLK_ARESULT_BLOCKING);
          KV_TRC_IO(pAT, "RT: id:%d blkno:%"PRIi64" status:%"PRIi64"",
                  ea->st_flash,
                  blist[i].blkno,
                  status);
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

int ea_delete(EA *ea) {
  int rc = 0;

  if ( ea->st_type == EA_STORE_TYPE_MEMORY )
  {
      KV_TRC(pAT, "ea %p ea->st_memory %p", ea, ea->st_memory);
    // Simple free the block of store
    free(ea->st_memory);
  }
  else
  {
    // Call to close out the chunk and free the space
    // for the device name
    rc = cblk_close(ea->st_flash, 0);
    am_free(ea->st_device);
  }

  if ( rc == 0 )
  {
    KV_TRC(pAT, "ea %p", ea);
    am_free(ea);
  }
  
  return rc;
}

