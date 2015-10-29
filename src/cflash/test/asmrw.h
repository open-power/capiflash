#ifndef _ASMRW_H
#define _ASMRW_H

#ifdef _AIX
#include<cflash_sisl.h>
#else
#include<sislite.h>
#endif

typedef struct mc_stat_s {
  __u32       blk_len;   /* length of 1 block in bytes as reported by device */
  __u8        nmask;     /* chunk_size = (1 << nmask) in device blocks */
  __u8        rsvd[3];
  __u64       size;      /* current size of the res_hndl in chunks */
  __u64       flags;     /* permission flags */
} mc_stat_t;

/* The write_nn or read_nn routines can be used to do byte reversed MMIO
   or byte reversed SCSI CDB/data.
*/
static inline void write_64(volatile __u64 *addr, __u64 val)
{
    __u64 zero = 0;
#ifndef _AIX
    asm volatile ( "stdbrx %0, %1, %2" : : "r"(val), "r"(zero), "r"(addr) );
#else
#ifndef __64BIT__
	*((volatile __u32 *)(addr)) = (val & 0xffffffff);
#else
    *((volatile __u64 *)(addr)) = val;
#endif
#endif /* _AIX */
}

static inline void write_32(volatile __u32 *addr, __u32 val)
{
    __u32 zero = 0;
#ifndef _AIX
    asm volatile ( "stwbrx %0, %1, %2" : : "r"(val), "r"(zero), "r"(addr) );
#else
    *((volatile __u32 *)(addr)) = val;
#endif /* _AIX */
}

static inline void write_16(volatile __u16 *addr, __u16 val)
{
    __u16 zero = 0;
#ifndef _AIX
    asm volatile ( "sthbrx %0, %1, %2" : : "r"(val), "r"(zero), "r"(addr) );
#else
    *((volatile __u16 *)(addr)) = val;
#endif /* _AIX */
}

static inline __u64 read_64(volatile __u64 *addr)
{
    __u64 val;
    __u64 zero = 0;
#ifndef _AIX
    asm volatile ( "ldbrx %0, %1, %2" : "=r"(val) : "r"(zero), "r"(addr) );
#else
#ifndef __64BIT__
	val =  *((volatile __u32 *)(addr));
#else
  val =  *((volatile __u64 *)(addr));
#endif
#endif /* _AIX */

    return val;
}
static inline __u32 read_32(volatile __u32 *addr)
{
    __u32 val;
    __u32 zero = 0;
#ifndef _AIX
    asm volatile ( "lwbrx %0, %1, %2" : "=r"(val) : "r"(zero), "r"(addr) );
#else
     val =  *((volatile __u32 *)(addr));
#endif /* _AIX */
    return val;
}

static inline __u16 read_16(volatile __u16 *addr)
{
    __u16 val;
    __u16 zero = 0;
#ifndef _AIX
    asm volatile ( "lhbrx %0, %1, %2" : "=r"(val) : "r"(zero), "r"(addr) );
#else
     val =  *((volatile __u16 *)(addr));
#endif /* _AIX */
    return val;
}
static inline void write_lba(__u64* addr, __u64 lba)
{
	#ifndef __64BIT__
    __u32 *p_u32 = (__u32*)addr;
    write_32(p_u32,lba >>32);
    write_32(p_u32+1, lba & 0xffffffff);
#else
        write_64(addr, lba);
#endif
}
#endif /*_ASMRW_H*/
