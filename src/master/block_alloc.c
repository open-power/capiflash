/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/master/block_alloc.c $                                    */
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
#include <stdio.h>
#include <sys/mman.h>

#include "block_alloc.h"
#include "block_alloc_internal.h"

/**************************************************************
 *                                                            *
 *            LUN BIT map table                               *
 *                                                            *
 *     0    1    2   3   4   5   6   ...    63                *
 *    64   65   66  67  68  69  70   ...   127                *
 *    ......                                                  *
 *                                                            *
 **************************************************************/


/**************************************************************
 *                                                            *
 *                      Defines                               *
 *                                                            *
 **************************************************************/

/* Bit operations */
#define SET_BIT(num, bit_pos)  num |= (uint64_t)0x01 << (63-bit_pos);
#define CLR_BIT(num, bit_pos)  num &= ~((uint64_t)0x01 << (63-bit_pos));
#define TEST_BIT(num, bit_pos)  (num & ((uint64_t)0x01 << (63-bit_pos)))


/**************************************************************
 *                                                            *
 *                Function Prototypes                         *
 *                                                            *
 **************************************************************/
static int find_free_bit(uint64_t lun_map_entry);


/**************************************************************
 *                                                            *
 *                Extern variables                            *
 *                                                            *
 **************************************************************/

extern unsigned int trc_lvl;

int
ba_init(ba_lun_t *ba_lun)
{
    lun_info_t   *lun_info = NULL;
    int           lun_size_au = 0, i = 0;
    int           last_word_underflow = 0;

    /* Allocate lun_fino */
    lun_info = (lun_info_t *) malloc(sizeof(lun_info_t));
    if (lun_info == NULL) {
       BA_TRACE_1(LOG_ERR, "block_alloc: Failed to allocate lun_info for lun_id %lx\n", ba_lun->lun_id);
       return -1;
    }

    /* Lock and bzero the allocated lun_info */
    mlock(lun_info, sizeof(lun_info_t));
    bzero(lun_info, sizeof(lun_info_t));

    BA_TRACE_3(LOG_INFO, "block_alloc: Initializing LUN: lun_id = %lx, ba_lun->lsize = %lx, ba_lun->au_size = %lx\n", ba_lun->lun_id, ba_lun->lsize, ba_lun->au_size);

    /* Calculate bit map size */
    lun_size_au = ba_lun->lsize / ba_lun->au_size;
#ifdef _FILEMODE_
    if (lun_size_au == 0) {
	lun_size_au = 1;
    }
#endif /* _FILEMODE_ */
    lun_info->total_aus = lun_size_au;
    lun_info->lun_bmap_size = lun_size_au / 64;
    if (lun_size_au % 64 > 0)
       lun_info->lun_bmap_size++;

    /* Allocate bitmap space */
    lun_info->lun_alloc_map = (uint64_t *)malloc(lun_info->lun_bmap_size * sizeof(uint64_t));
    if (lun_info->lun_alloc_map == NULL) {
       BA_TRACE_1(LOG_ERR, "block_alloc: Failed to allocate lun allocation map: lun_id = %lx\n", ba_lun->lun_id);
       free (lun_info);
       return -1;
    }

    /* Lock and initialize the bit map */
    mlock(lun_info->lun_alloc_map,
          lun_info->lun_bmap_size * sizeof(uint64_t));
    lun_info->free_aun_cnt = lun_size_au;

    /* set all bits in bitmap to '1' */
    for (i = 0; i < lun_info->lun_bmap_size; i++) {
       lun_info->lun_alloc_map[i] = (uint64_t)~0;
    }

    /* If the last word is not fully utilized, mark the extra bits as allocated */
    last_word_underflow = (lun_info->lun_bmap_size * 64) - lun_info->free_aun_cnt;
    if (last_word_underflow > 0 ) {
       for ( i = 63-last_word_underflow+1; i < 64 ; i++) {
           CLR_BIT(lun_info->lun_alloc_map[lun_info->lun_bmap_size-1], i);
       }
    }

    /* Initialize elevator indices */
    lun_info->free_low_idx  = 0;
    lun_info->free_curr_idx = 0;
    lun_info->free_high_idx = lun_info->lun_bmap_size;

    /* Allocate clone map */
    lun_info->aun_clone_map = (unsigned char *)malloc(lun_info->total_aus * sizeof(unsigned char));
    if (lun_info->aun_clone_map == NULL) {
        BA_TRACE_1(LOG_ERR, "block_alloc: Failed to allocate clone map: lun_id = %lx\n", ba_lun->lun_id);
        free (lun_info->lun_alloc_map);
        free (lun_info);
        return -1;
    }

    mlock(lun_info->aun_clone_map, lun_info->total_aus * sizeof(unsigned char));
    bzero(lun_info->aun_clone_map, lun_info->total_aus * sizeof(unsigned char));

    ba_lun->ba_lun_handle = (void *)lun_info;

    BA_TRACE_3(LOG_INFO, "block_alloc: Successfully initialized the LUN: lun_id = %lx, bitmap size = %x, free_aun_cnt = %lx\n", ba_lun->lun_id, lun_info->lun_bmap_size, lun_info->free_aun_cnt);

    return 0;

} /* End of ba_init */


static int
find_free_bit(uint64_t lun_map_entry)
{
    int pos = -1;

#ifndef _AIX
    asm volatile ( "cntlzd %0, %1": "=r"(pos) : "r"(lun_map_entry) );
#else
    int i = 0;
    for (i = 1; i <= 64; i++) {
         if (lun_map_entry & ((uint64_t)0x1 << (64 - i))) {
             pos = i-1;
             break;
         }
    } /* End of for */
#endif

    return pos;

} /* End of find_free_bit */


aun_t
ba_alloc(ba_lun_t *ba_lun)
{
    aun_t        bit_pos = -1;
    int          i = 0;
    lun_info_t  *lun_info = NULL;

    lun_info = (lun_info_t *)ba_lun->ba_lun_handle;

    BA_TRACE_2(LOG_INFO, "block_alloc: Received block allocation request: lun_id = %lx, free_aun_cnt = %lx\n", ba_lun->lun_id, lun_info->free_aun_cnt);

    if (lun_info->free_aun_cnt == 0) {
       BA_TRACE_1(LOG_ERR, "block_alloc: No space left on LUN: lun_id = %lx\n", ba_lun->lun_id);
       return (aun_t)-1;
    }

    /* Search for free entry between free_curr_idx and free_high_idx */
    for (i = lun_info->free_curr_idx; i < lun_info->free_high_idx; i++)
    {
       if (lun_info->lun_alloc_map[i] != 0) {

           /* There are some free AUs .. find free entry */
           bit_pos = find_free_bit(lun_info->lun_alloc_map[i]);

           BA_TRACE_3(LOG_INFO, "block_alloc: Found free bit %lx in lun map entry %lx at bitmap index = %x\n", bit_pos, lun_info->lun_alloc_map[i], i);

           lun_info->free_aun_cnt--;
           CLR_BIT(lun_info->lun_alloc_map[i], bit_pos);

           break;
       }
    }

    if (bit_pos == -1) {
       /* Search for free entry between free_low_idx and free_curr_idx  */
       for (i = lun_info->free_low_idx; i < lun_info->free_curr_idx; i++)
       {
           if (lun_info->lun_alloc_map[i] != 0) {

              /* There are some free AUs .. find free entry */
              bit_pos = find_free_bit(lun_info->lun_alloc_map[i]);

              BA_TRACE_3(LOG_INFO, "block_alloc: Found free bit %lx in lun map entry %lx at bitmap index = %x\n", bit_pos, lun_info->lun_alloc_map[i], i);

              lun_info->free_aun_cnt--;
              CLR_BIT(lun_info->lun_alloc_map[i], bit_pos);

              break;
           }
       }
    }


    if (bit_pos == -1) {
       BA_TRACE_1(LOG_ERR, "block_alloc: Could not find an allocation unit on LUN: lun_id = %lx\n", ba_lun->lun_id);
       return (aun_t)-1;
    }

    /* Update the free_curr_idx                                     */
    if ( bit_pos == 63 )
        lun_info->free_curr_idx = i + 1;
    else
        lun_info->free_curr_idx = i;

    BA_TRACE_3(LOG_INFO, "block_alloc: Allocating AU number %lx, on lun_id %lx, free_aun_cnt = %lx\n", (i * 64 + bit_pos), ba_lun->lun_id, lun_info->free_aun_cnt);
    return (aun_t)(i * 64 + bit_pos);

} /* End of ba_alloc */


static int
validate_alloc(lun_info_t *lun_info, aun_t aun)
{
    int          idx = 0, bit_pos = 0;

    idx     = aun / 64;
    bit_pos = aun % 64;

    if (TEST_BIT(lun_info->lun_alloc_map[idx], bit_pos)) {
        return -1;
    }

    return 0;
} /* End of validate_alloc */


int
ba_free(ba_lun_t *ba_lun, aun_t to_free)
{
    int          idx = 0, bit_pos = 0;
    lun_info_t  *lun_info = NULL;

    lun_info = (lun_info_t *)ba_lun->ba_lun_handle;

    if (validate_alloc(lun_info, to_free) != 0) {
       BA_TRACE_2(LOG_ERR, "block_alloc: The AUN %lx is not allocated on lun_id %lx\n", to_free, ba_lun->lun_id);
       return -1;
    }

    BA_TRACE_3(LOG_INFO, "block_alloc: Received a request to free AU %lx on lun_id %lx, free_aun_cnt = %lx\n", to_free, ba_lun->lun_id, lun_info->free_aun_cnt);


    if (lun_info->aun_clone_map[to_free] > 0) {
        BA_TRACE_3(LOG_INFO, "block_alloc: AU %lx on lun_id %lx has been cloned. Clone count = %x\n", to_free, ba_lun->lun_id, lun_info->aun_clone_map[to_free]);
        lun_info->aun_clone_map[to_free]--;
        return 0;
    }

    idx     = to_free / 64;
    bit_pos = to_free % 64;

    SET_BIT(lun_info->lun_alloc_map[idx], bit_pos);
    lun_info->free_aun_cnt++;

    if (idx < lun_info->free_low_idx)
        lun_info->free_low_idx = idx;
    else if (idx > lun_info->free_high_idx)
        lun_info->free_high_idx = idx;

    BA_TRACE_4(LOG_INFO, "block_alloc: Successfully freed AU at bit_pos %x, bit map index %x on lun_id %lx, free_aun_cnt = %lx\n", bit_pos, idx, ba_lun->lun_id, lun_info->free_aun_cnt);
    return 0;

} /* End of ba_free */


int
ba_clone(ba_lun_t *ba_lun, aun_t to_clone)
{
    lun_info_t  *lun_info = NULL;

    lun_info = (lun_info_t *)ba_lun->ba_lun_handle;

    if (validate_alloc(lun_info, to_clone) != 0) {
       BA_TRACE_2(LOG_ERR, "block_alloc: AUN %lx is not allocated on lun_id %lx\n", to_clone, ba_lun->lun_id);
       return -1;
    }

    BA_TRACE_2(LOG_INFO, "block_alloc: Received a request to clone AU %lx on lun_id %lx\n", to_clone, ba_lun->lun_id);

    if (lun_info->aun_clone_map[to_clone] == MAX_AUN_CLONE_CNT) {
        BA_TRACE_2(LOG_ERR, "block_alloc: AUN %lx on lun_id %lx has hit max clones already\n", to_clone, ba_lun->lun_id);
        return -1;
    }

    lun_info->aun_clone_map[to_clone]++;

    return 0;
} /* End of ba_clone */


uint64_t
ba_space(ba_lun_t *ba_lun)
{
    lun_info_t   *lun_info = NULL;

    lun_info = (lun_info_t *)ba_lun->ba_lun_handle;

    return (lun_info->free_aun_cnt);
} /* End of ba_space */

#ifdef BA_DEBUG
void
dump_ba_map(ba_lun_t *ba_lun)
{
    lun_info_t   *lun_info = NULL;
    int           i = 0, j = 0;

    lun_info = (lun_info_t *)ba_lun->ba_lun_handle;

    printf("Dumping block allocation map: map size = %d\n", lun_info->lun_bmap_size);

    for (i = 0; i < lun_info->lun_bmap_size; i++) {
       printf("%4d ", i * 64);
       for (j = 0; j < 64; j++) {
          if ( j % 4 == 0)
             printf(" ");
          printf("%1d", TEST_BIT(lun_info->lun_alloc_map[i], j) ? 1:0);
       }
       printf("\n");
    }

    printf("\n");
    return;
}

void
dump_ba_clone_map(ba_lun_t *ba_lun)
{
    lun_info_t   *lun_info = NULL;
    int           i = 0;

    lun_info = (lun_info_t *)ba_lun->ba_lun_handle;

    printf("Dumping clone map: map size = %d\n", lun_info->total_aus);

    for (i = 0; i < lun_info->total_aus; i++) {
        if (i % 64 == 0) {
           printf("\n%3d", i);
        }

        if (i %4 == 0)
           printf("   ");
        printf("%2x", lun_info->aun_clone_map[i]);
    }

    printf("\n");
    return;
}
#endif
