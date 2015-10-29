**CAPI Flash Block Layer API**

**1.1 cblk\_init**

***Purpose***

Initializes CAPI block library

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

int rc = cblk\_int(void \*arg, int flags)

***Parameters***

|               |                                                         |
|---------------|---------------------------------------------------------|
| **Parameter** | **Description**                                         |
| arg           | Currently unused (set to NULL)                          |
| flags         | Specifies flags for initialization. Currently set to 0. |

***Description***

The cblk\_init API initializes the CAPI block library prior to use. cblk\_init must be called before any other API in the library is called.

***Return Values***

Returns 0 on success; otherwise it is an error.

**1.2 cblk\_term**

***Purpose***

Cleans up CAPI block library resources after the library is no longer used.

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

int rc = cblk\_term(void \*arg, int flags)

***Parameters***

|               |                                                         |
|---------------|---------------------------------------------------------|
| **Parameter** | **Description**                                         |
| arg           | Currently unused (set to NULL)                          |
| flags         | Specifies flags for initialization. Currently set to 0. |

***Description***

The cblk\_term API terminates the CAPI block library after use.

***Return Values***

Returns 0 on success; otherwise it is an error.

**1.3 cblk\_open**

***Purpose***

Open a collection of contiguous blocks (currently called a “chunk”) on a CAPI flash storage device. for which I/O (read and writes) can be done. A chunk can be thought of as a lun, which the provides access to sectors 0 thru n-1 (where n is the size of the chunk in sectors). If virtual luns are specified then that chunk is a subset of sectors on a physical lun.

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

chunk\_id\_t chunk\_id = cblk\_open(const char \*path, int max\_num\_requests, int mode, uint64\_t ext\_arg, int flags)

***Parameters***

***Parameters***

|                    |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
|--------------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Parameter**      | **Description**                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| path               | This the CAPI disk special filename (i.e. /dev/sg0 for Linux, /dev/hdisk1 for AIX)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| max\_num\_requests | This indicates the maximum number of commands that can be queued to the adapter for this chunk at a given time. If this value is 0, then block layer will choose a default size. If the value specified it too large then the cblk\_open request will fail with an ENOMEM errno.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| mode               | Specifies the access mode for the child process (O\_RDONLY, O\_WRONLY, O\_RDWR).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| ext\_arg           | Reserved for future use                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| flags              | This is a collection of bit flags. The **CBLK\_OPN\_VIRT\_LUN**indicates a virtual lun on the one physical lun will be provisioned. If the **CBLK\_OPN\_VIRT\_LUN** is not specified, then direct access to the full physical lun will be provided. The **CBLK\_OPN\_VIRT\_LUN**flag must be set for Redis shards. The **CBLK\_OPN\_NO\_INTRP\_THREADS**flag indicates that the cflash block library will not start any back ground threads for processing/harvesting of asynchronous completions from the CAPI adapter. Instead the process using this library must either call cblk\_aresult, or cblk\_listio library calls to poll for I/O completions. The **CBLK\_OPN\_MPIO\_FO**flag (valid for only AIX) indicates that the cflash block library will use Multi-path I/O failover (i.e. one path will be used for all I/O unless path specific errors are encountered, in which case an alternate path will be used if available. To determine the paths for a CAPI flash disk, use the command lspath -l hdiskN). The **CBLK\_OPN\_RESERVE** flag (valid for only AIX) indicates the cflash block library will use the “reserve policy” attribute associated with the disk in terms of establishing disk reservations. The **CBLK\_OPN\_RESERVE** flag can not be used in conjunction with the **CBLK\_OPN\_MPIO\_FO.** The **CBLK\_OPN\_FORCED\_RESERVE** flag (valid for only AIX) has the same behavior as the **CBLK\_OPEN\_RESERVE** flag with the one addition, that when the device is opened it will break any outstanding disk reservations on the first open of this disk. The **CBLK\_OPN\_FORCED\_RESERVE**flag can not be used in conjunction with the **CBLK\_OPN\_MPIO\_FO.** |

***Description***

The cblk\_open API creates a “chunk” of blocks on a CAPI flash lun. This chunk will be used for I/O (cblk\_read/cblk\_write) requests. The returned chunk\_id is assigned to a specific path via a specific adapter transparently to the caller. The underlying physical sectors used by a chunk will not be directly visible to users of the block layer.

Upon successful completion, a chunk id representing the newly created chunk instance is returned to the caller to be used for future API calls.

***Return Values***

Returns NULL\_CHUNK\_ID on error; otherwise it is a chunk\_id handle.

**1.4 cblk\_close**

***Purpose***

Closes a collection of contiguous blocks (called a “chunk”) on a CAPI flash storage device. for which I/O (read and writes) can be done.

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

int rc = cblk\_close(chunk\_id\_t chunk\_id, int flags))

***Description***

This service releases the blocks associated with a chunk to be reused by others. Prior to them being reused by others the data blocks will need to be “scrubbed” to remove user data if the **CBLK\_SCRUB\_DATA\_FLG** is set.

***Parameters***

|               |                                                                                                                                                                                                                                       |
|---------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Parameter** | **Description**                                                                                                                                                                                                                       |
| chunk\_id     | The handle for the chunk which is being closed (released for reuse)                                                                                                                                                                   |
| flags         | This is a collection of bit flags. The **CBLK\_SCRUB\_DATA\_FLG**indicates data blocks should be scrubbed before they can be reused by others,which is only valid for virtual luns (chunks opened with **CBLK\_OPN\_VIRT\_LUN**flag). |

***Return Values***

Returns 0 on success; otherwise it is an error.

**1.5 cblk\_get\_lun\_size**

***Purpose***

Returns the “size” (number of blocks) of the physical lun to which this chunk is associated. This call is not valid for a virtual lun.

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

int rc = cblk\_get\_lun\_size(chunk\_id\_t chunk\_id, size\_t \*size, int flags))

***Description***

This service returns the number of blocks of the physical lun associated with this chunk. The cblk\_get\_lun\_size service requires one has done a cblk\_open to receive a valid chunk\_id.

***Parameters***

|               |                                                             |
|---------------|-------------------------------------------------------------|
| **Parameter** | **Description**                                             |
| chunk\_id     | The handle for the chunk whose size is about to be changed. |
| size          | Total number of 4K block for this physical lun.             |
| flags         | This is a collection of bit flags.                          |

***Return Values***

Returns 0 on success; otherwise it is error.

**1.6 cblk\_get\_size**

***Purpose***

Returns the “size” (number of blocks) assigned to a specific (virtual lun) chunk id, which is a virtual lun (i.e. the cblk\_open call that returned this id, had the **CBLK\_OPN\_VIRT\_LUN** flag). This service is not valid for physical luns.

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

int rc = cblk\_get\_size(chunk\_id\_t chunk\_id, size\_t \*size, int flags))

***Description***

This service returns the number of blocks allocated to this chunk. The cblk\_get\_size service requires one has done a cblk\_open to receive a valid chunk\_id.

***Parameters***

|               |                                                             |
|---------------|-------------------------------------------------------------|
| **Parameter** | **Description**                                             |
| chunk\_id     | The handle for the chunk whose size is about to be changed. |
| size          | Number of 4K block to be used for this chunk.               |
| flags         | This is a collection of bit flags.                          |

***Return Values***

Returns 0 on success; otherwise it is error.

**1.7 cblk\_set\_size**

***Purpose***

Assign “size” blocks to a specific chunk id which is a virtual lun (i.e. the cblk\_open call that returned this id, had the **CBLK\_OPN\_VIRT\_LUN** flag). If blocks are already assigned to this chunk id, then one can increase/decrease the size by specifying a larger/smaller size, respectively. This service is not valid for physical luns.

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

int rc = cblk\_set\_size(chunk\_id\_t chunk\_id, size\_t size, int flags))

***Description***

This service allocates “size” blocks to this chunk. The cblk\_set\_size call must be done prior to any cblk\_read or cblk\_write calls to this chunk. The cblk\_set\_size service requires one has done a cblk\_open to receive a valid chunk\_id.

If there were blocks originally assigned to this chunk and they are not being reused after cblk\_set\_size allocates the new blocks and the CBLK\_SCRUB\_DATA\_FLG is set in the flags parameter, then those originally blocks will be “scrubbed” prior to allowing them to be reused by other cblk\_set\_size operations.

Upon successful completion, the chunk will has LBAs 0 thru size – 1 that can be read/written.

***Parameters***

|               |                                                                                                                                             |
|---------------|---------------------------------------------------------------------------------------------------------------------------------------------|
| **Parameter** | **Description**                                                                                                                             |
| chunk\_id     | The handle for the chunk whose size is about to be changed.                                                                                 |
| size          | Number of 4K block to be used for this chunk.                                                                                               |
| flags         | This is a collection of bit flags. The CBLK\_SCRUB\_DATA\_FLG indicates data blocks should be scrubbed before they can be reused by others. |

***Return Values***

Returns 0 on success; otherwise it is error.

**1.8 cblk\_get\_stats**

***Purpose***

Returns statistics for a specific chunk id.

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

typedef struct chunk\_stats\_s {

uint64\_t max\_transfer\_size; /\* Maximum transfer size in \*/

/\* blocks of this chunk. \*/

uint64\_t num\_reads; /\* Total number of reads issued \*/

/\* via cblk\_read interface \*/

uint64\_t num\_writes; /\* Total number of writes issued \*/

/\* via cblk\_write interface \*/

uint64\_t num\_areads; /\* Total number of async reads \*/

/\* issued via cblk\_aread interface \*/

uint64\_t num\_awrites; /\* Total number of async writes \*/

/\* issued via cblk\_awrite interface\*/

uint32\_t num\_act\_reads; /\* Current number of reads active \*/

/\* via cblk\_read interface \*/

uint32\_t num\_act\_writes; /\* Current number of writes active \*/

/\* via cblk\_write interface \*/

uint32\_t num\_act\_areads; /\* Current number of async reads \*/

/\* active via cblk\_aread interface \*/

uint32\_t num\_act\_awrites; /\* Current number of async writes \*/

/\* active via cblk\_awrite interface\*/

uint32\_t max\_num\_act\_writes; /\* High water mark on the maximum \*/

/\* number of writes active at once \*/

uint32\_t max\_num\_act\_reads; /\* High water mark on the maximum \*/

/\* number of reads active at once \*/

uint32\_t max\_num\_act\_awrites; /\* High water mark on the maximum \*/

/\* number of asyync writes active \*/

/\* at once. \*/

uint32\_t max\_num\_act\_areads; /\* High water mark on the maximum \*/

/\* number of asyync reads active \*/

/\* at once. \*/

uint64\_t num\_blocks\_read; /\* Total number of blocks read \*/

uint64\_t num\_blocks\_written; /\* Total number of blocks written \*/

uint64\_t num\_errors; /\* Total number of all error \*/

/\* responses seen \*/

uint64\_t num\_aresult\_no\_cmplt; /\* Number of times cblk\_aresult \*/

/\* returned with no command \*/

/\* completion \*/

uint64\_t num\_retries; /\* Total number of all commmand \*/

/\* retries. \*/

uint64\_t num\_timeouts; /\* Total number of all commmand \*/

/\* time-outs. \*/

uint64\_t num\_fail\_timeouts; /\* Total number of all commmand \*/

/\* time-outs that led to a command \*/

/\* failure. \*/

uint64\_t num\_no\_cmds\_free; /\* Total number of times we didm't \*/

/\* have free command available \*/

uint64\_t num\_no\_cmd\_room ; /\* Total number of times we didm't \*/

/\* have room to issue a command to \*/

/\* the AFU. \*/

uint64\_t num\_no\_cmds\_free\_fail; /\* Total number of times we didn't \*/

/\* have free command available and \*/

/\* failed a request because of this\*/

uint64\_t num\_fc\_errors; /\* Total number of all FC \*/

/\* error responses seen \*/

uint64\_t num\_port0\_linkdowns; /\* Total number of all link downs \*/

/\* seen on port 0. \*/

uint64\_t num\_port1\_linkdowns; /\* Total number of all link downs \*/

/\* seen on port 1. \*/

uint64\_t num\_port0\_no\_logins; /\* Total number of all no logins \*/

/\* seen on port 0. \*/

uint64\_t num\_port1\_no\_logins; /\* Total number of all no logins \*/

/\* seen on port 1. \*/

uint64\_t num\_port0\_fc\_errors; /\* Total number of all general FC \*/

/\* errors seen on port 0. \*/

uint64\_t num\_port1\_fc\_errors; /\* Total number of all general FC \*/

/\* errors seen on port 1. \*/

uint64\_t num\_cc\_errors; /\* Total number of all check \*/

/\* condition responses seen \*/

uint64\_t num\_afu\_errors; /\* Total number of all AFU error \*/

/\* responses seen \*/

uint64\_t num\_capi\_false\_reads; /\* Total number of all times \*/

/\* poll indicated a read was ready \*/

/\* but there was nothing to read. \*/

uint64\_t num\_capi\_adap\_resets; /\* Total number of all adapter \*/

/\* reset errors. \*/

uint64\_t num\_capi\_afu\_errors; /\* Total number of all \*/

/\* CAPI error responses seen \*/

uint64\_t num\_capi\_afu\_intrpts; /\* Total number of all \*/

/\* CAPI AFU interrupts for command \*/

/\* responses seen. \*/

uint64\_t num\_capi\_unexp\_afu\_intrpts; /\* Total number of all of \*/

/\* unexpected AFU interrupts \*/

uint64\_t num\_active\_threads; /\* Current number of threads \*/

/\* running. \*/

uint64\_t max\_num\_act\_threads; /\* Maximum number of threads \*/

/\* running simultaneously. \*/

uint64\_t num\_cache\_hits; /\* Total number of cache hits \*/

/\* seen on all reads \*/

} chunk\_stats\_t;

int rc = cblk\_get\_stats(chunk\_id\_t chunk\_id, chunk\_stats\_t \*stats, int flags))

***Description***

This service returns statistics for a specific chunk\_id.

***Parameters***

|               |                                                             |
|---------------|-------------------------------------------------------------|
| **Parameter** | **Description**                                             |
| chunk\_id     | The handle for the chunk whose size is about to be changed. |
| stats         | Address of a chunk\_stats\_t structure.                     |
| flags         | This is a collection of bit flags.                          |

***Return Values***

Returns 0 on success; otherwise it is error.

**1.9 cblk\_read**

***Purpose***

Read 4K blocks from the chunk at the specified logical block address (LBA) into the buffer specified. It should be noted that his LBA is not the same as the LUNs LBA, since the chunk does not necessarily start at the lun's LBA 0

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

int rc = cblk\_read(chunk\_id\_t chunk\_id, void \*buf, off\_t lba, size\_t nblocks, int flags));

***Description***

This service reads data from the chunk and places that data into the supplied buffer. This call will block until the read completes with success or error. The cblk\_set\_size call must be done prior to any cblk\_read, cblk\_write, cblk\_aread, or cblk\_awrite calls to this chunk.

***Parameters***

|               |                                                                                                                            |
|---------------|----------------------------------------------------------------------------------------------------------------------------|
| **Parameter** | **Description**                                                                                                            |
| chunk\_id     | The handle for the chunk which is being read.                                                                              |
| buf           | Buffer to which data is read into from the chunk must be aligned on 16 byte boundaries.                                    |
| lba           | Logical Block Address (4K offset) inside chunk.                                                                            |
| nblocks       | Specifies the size of the transfer in 4K sectors. Upper bound is 16 MB for physical lun. Upper bound for virtual lun is 4K |
| flags         | This is a collection of bit flags.                                                                                         |

***Return Values***

|                  |                                         |
|------------------|-----------------------------------------|
| **Return Value** | **Description**                         |
| -1               | Error and errno is set for more details |
| 0                | No data was read.                       |
| n &gt;0          | Number, n, of blocks read.              |

**1.10 cblk\_write**

***Purpose***

Write 4K blocks to the chunk at the specified logical block address (LBA) using the data from the buffer. It should be noted that his LBA is not the same as the LUNs LBA, since the chunk does not start at LBA 0

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

int rc = cblk\_write(chunk\_id\_t chunk\_id, void \*buf, off\_t lba, size\_t nblocks, int flags));

***Description***

This service writes data from the chunk and places that data into the supplied buffer. This call will block until the write completes with success or error. The cblk\_set\_size call must be done prior to any cblk\_write calls to this chunk.

***Parameters***

|               |                                                                                                                            |
|---------------|----------------------------------------------------------------------------------------------------------------------------|
| **Parameter** | **Description**                                                                                                            |
| chunk\_id     | The handle for the chunk which is being written.                                                                           |
| buf           | Buffer to which data is written from onto the chunk must be aligned on 16 byte boundaries.                                 |
| lba           | Logical Block Address (4K offset) inside chunk.                                                                            |
| nblocks       | Specifies the size of the transfer in 4K sectors. Upper bound is 16 MB for physical lun. Upper bound for virtual lun is 4K |
| flags         | This is a collection of bit flags.                                                                                         |

***Return Values***

|                  |                                         |
|------------------|-----------------------------------------|
| **Return Value** | **Description**                         |
| -1               | Error and errno is set for more details |
| 0                | No data was written                     |
| n &gt;0          | Number, n, of blocks written.           |

**1.11 cblk\_aread**

***Purpose***

Read 4K blocks from the chunk at the specified logical block address (LBA) into the buffer specified. It should be noted that his LBA is not the same as the LUNs LBA, since the chunk does not start at LBA 0

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

typedef enum {

CBLK\_ARW\_STATUS\_PENDING = 0, /\* Command has not completed \*/

CBLK\_ARW\_STATUS\_SUCCESS = 1 /\* Command completed successfully \*/

CBLK\_ARW\_STATUS\_INVALID = 2 /\* Caller's request is invalid \*/

CBLK\_ARW\_STATUS\_FAIL = 3 /\* Command completed with error \*/

} cblk\_status\_type\_t;

typedef struct cblk\_arw\_status\_s {

cblk\_status\_type\_t status; /\* Status of command \*/

/\* See errno field for additional \*/

/\* details on failure \*/

size\_t blocks\_transferred;/\* Number of block transferred by \*/

/\* this reqeuest. \*/

int errno; /\* Errno when status indicates \*/

/\* CBLK\_ARW\_STAT\_FAIL \*/

} cblk\_arw\_status\_t;

int rc = cblk\_aread(chunk\_id\_t chunk\_id, void \*buf, off\_t lba, size\_t nblocks, int \*tag, cblk\_arw\_status\_t \*status, int flags));

***Description***

This service reads data from the chunk and places that data into the supplied buffer. This call will not block to wait for the read to complete. A subsequent cblk\_aresult call must be invoked to poll on completion. The cblk\_set\_size call must be done prior to any cblk\_aread calls to this chunk.

***Parameters***

<table>
<colgroup>
<col width="50%" />
<col width="50%" />
</colgroup>
<tbody>
<tr class="odd">
<td align="left"><strong>Parameter</strong></td>
<td align="left"><strong>Description</strong></td>
</tr>
<tr class="even">
<td align="left">chunk_id</td>
<td align="left">The handle for the chunk which is being written.</td>
</tr>
<tr class="odd">
<td align="left">buf</td>
<td align="left">Buffer to which data is written from onto the chunk must be aligned on 16 byte boundaries.</td>
</tr>
<tr class="even">
<td align="left">lba</td>
<td align="left">Logical Block Address (4K offset) inside chunk.</td>
</tr>
<tr class="odd">
<td align="left">nblocks</td>
<td align="left">Specifies the size of the transfer in 4K sectors. Upper bound is 16 MB for physical lun. Upper bound for virtual lun is 4K</td>
</tr>
<tr class="even">
<td align="left">tag</td>
<td align="left">Returned Identifier that allows the caller to uniquely identify each command issued.</td>
</tr>
<tr class="odd">
<td align="left">status</td>
<td align="left"><p>Address or 64-bit field provided by the caller which the capiblock library will update when a this command completes. This can be used by an application in place of using the cblk_aresult service.</p>
<p>It should be noted that the CAPI adapter can not do this directly it would require software threads to update the status region. This field is not used if the <strong>CBLK_OPN_NO_INTRP_THREADS</strong>flags was specified for cblk_open the returned this chunk_id.</p></td>
</tr>
<tr class="even">
<td align="left">flags</td>
<td align="left">This is a collection of bit flags. The <strong>CBLK_ARW_WAIT_CMD_FLAGS</strong>will cause this service to block to wait for a free command to issue the request. Otherwise this service could return a value of -1 with an errno of EWOULDBLOCK (if there is no free command currently available). The <strong>CBLK_ARW_USER_TAG_FLAGS</strong>indicates the caller is specifying a user defined tag for this request. The caller would then need to use this tag with cblk_aresult and set its <strong>CBLK_ARESULT_USER_TAG</strong> flag. The <strong>CBLK_ARW_USER_STATUS_FLAG</strong>indicates the caller has set the status parameter which it expects will be updated when the command completes.</td>
</tr>
</tbody>
</table>

***Return Values***

|                  |                                                                                  |
|------------------|----------------------------------------------------------------------------------|
| **Return Value** | **Description**                                                                  |
| -1               | Error and errno is set for more details                                          |
| 0                | Successfully issued                                                              |
| n &gt;0          | Indicates read completed (possibly from cache) and Number, n, of blocks written. |

**1.12 cblk\_awrite**

***Purpose***

Write one 4K block to the chunk at the specified logical block address (LBA) using the data from the buffer. It should be noted that his LBA is not the same as the LUNs LBA, since the chunk does not start at LBA 0

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

typedef enum {

CBLK\_ARW\_STAT\_NOT\_ISSUED = 0, /\* Command is has not been issued \*/

CBLK\_ARW\_STAT\_PENDING = 1, /\* Command has not completed \*/

CBLK\_ARW\_STAT\_SUCCESS = 2 /\* Command completed successfully \*/

CBLK\_ARW\_STAT\_FAIL = 3 /\* Command completed with error \*/

} cblk\_status\_type\_t;

typedef struct cblk\_arw\_status\_s {

cblk\_status\_type\_t status; /\* Status of command \*/

/\* See errno field for additional \*/

/\* details on failure \*/

size\_t blocks\_transferred;/\* Number of block transferred by \*/

/\* this reqeuest. \*/

int errno; /\* Errno when status indicates \*/

/\* CBLK\_ARW\_STAT\_FAIL \*/

} cblk\_arw\_status\_t;

int rc = cblk\_awrite(chunk\_id\_t chunk\_id, void \*buf, off\_t lba, size\_t nblocks, int \*tag, cblk\_arw\_status\_t \*status, int flags));

***Description***

This service writes data from the chunk and places that data into the supplied buffer. This call will not block waiting for the write to complete. A subsequent cblk\_aresult call must be invoked to poll on completion. The cblk\_set\_size call must be done prior to any cblk\_awrite calls to this chunk.

***Parameters***

<table>
<colgroup>
<col width="50%" />
<col width="50%" />
</colgroup>
<tbody>
<tr class="odd">
<td align="left"><strong>Parameter</strong></td>
<td align="left"><strong>Description</strong></td>
</tr>
<tr class="even">
<td align="left">chunk_id</td>
<td align="left">The handle for the chunk which is being written.</td>
</tr>
<tr class="odd">
<td align="left">buf</td>
<td align="left">Buffer to which data is written from onto the chunk must be aligned on 16 byte boundaries.</td>
</tr>
<tr class="even">
<td align="left">lba</td>
<td align="left">Logical Block Address (4K offset) inside chunk.</td>
</tr>
<tr class="odd">
<td align="left">nblocks</td>
<td align="left">Specifies the size of the transfer in 4K sectors. Upper bound is 16 MB for physical lun. Upper bound for virtual lun is 4K</td>
</tr>
<tr class="even">
<td align="left">tag</td>
<td align="left">Returned identifier that allows the caller to uniquely identify each command issued.</td>
</tr>
<tr class="odd">
<td align="left">status</td>
<td align="left"><p>Address or 64-bit field provided by the caller which the capiblock library will updatewhen a this command completes. This can be used by an application in place of using the cblk_aresult service.</p>
<p>It should be noted that the CAPI adapter can not do this directly it would require software threads to update the status region. This field is not used if the <strong>CBLK_OPN_NO_INTRP_THREADS</strong>flags was specified for cblk_open the returned this chunk_id.</p></td>
</tr>
<tr class="even">
<td align="left">flags</td>
<td align="left">This is a collection of bit flags. The <strong>CBLK_ARW_WAIT_CMD_FLAGS</strong>will cause this service to block to wait for a free command to issue the request. Otherwise this service could return a value of -1 with an errno of EWOULDBLOCK (if there is no free command currently available). The <strong>CBLK_ARW_USER_TAG_FLAGS</strong>indicates the caller is specifying a user defined tag for this request. The caller would then need to use this tag with cblk_aresult and set its <strong>CBLK_ARESULT_USER_TAG</strong> flag. The <strong>CBLK_ARW_USER_STATUS_FLAG</strong>indicates the caller has set the status parameter which it expects will be updated when the command completes.</td>
</tr>
</tbody>
</table>

***Return Values***

Returns 0 on success; otherwise it returns -1 and errno is set.

**1.13 cblk\_aresult**

***Purpose***

Return status and completion information for asynchronous requests.

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

rc = cblk\_aresult(chunk\_id\_t chunk\_id, int \*tag, uint64\_t \*status, int flags);

***Description***

This service returns an indication if the pending request issued via cblk\_aread or cblk\_awrite, which may have completed and if so its status.

***Parameters***

|               |                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
|---------------|------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Parameter** | **Description**                                                                                                                                                                                                                                                                                                                                                                                                                                                                          |
| chunk\_id     | The handle for the chunk which is being written.                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| tag           | Pointer to tag caller is waiting for completion. If the CBLK\_ARESULT\_NEXT\_TAG is set, then this field returns the tag for the next asynchronous completion                                                                                                                                                                                                                                                                                                                            |
| status        | Pointer to status. The status will be returned when a request completes.                                                                                                                                                                                                                                                                                                                                                                                                                 |
| flags         | Flags passed from the caller to cblk\_aresult. The flag **CBLK\_ARESULT\_BLOCKING**is set by the caller if they want cblk\_aresult to block until a command completes (provided there are active commands). If the **CBLK\_ARESULT\_NEXT\_TAG**flag is set, then this call returns whenever any asynchronous I/O request completes. The **CBLK\_ARESULT\_USER\_TAG** flag indicates the caller checking for status of an asynchronous request that was issued with a user specified tag. |

***Return Values***

|                  |                                                                                                                                                   |
|------------------|---------------------------------------------------------------------------------------------------------------------------------------------------|
| **Return Value** | **Description**                                                                                                                                   |
| -1               | Error and errno is set for more details                                                                                                           |
| 0                | Returned without error but no tag is set. This may occur if an I/O request has not yet completed and the CBLK\_ARESULT\_BLOCKING flag is not set. |
| n &gt;0          | Number, n, of blocks read/written.                                                                                                                |

**1.14 cblk\_clone\_after\_fork**

***Purpose***

Allows a child process to access the same virtual lun as the parent process. The child process must do this operation immediately after the fork, using the parent's chunk id in order to access that storage. If the child does not do this operation then it will not have any access to the parent's chunk ids. This service is not valid for physical luns. This is service is only valid for linux.

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

rc = cblk\_clone\_after\_fork(chunk\_id\_t chunk\_id, int mode, int flags);

***Description***

This service allows a child process to access data from the parents process.

***Parameters***

|               |                                                                                                                                                                                                                                        |
|---------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Parameter** | **Description**                                                                                                                                                                                                                        |
| chunk\_id     | The handle for the chunk which is in use by the parent process. If this call returns successfully, then this chunk id can also be used by the child process.                                                                           |
| mode          | Specifies the access mode for the child process (O\_RDONLY, O\_WRONLY, O\_RDWR). NOTE: child process can not have greater access than parent. cblk\_open is O\_RDWR for the initial parent. Descendant processes can have less access. |
| flags         | Flags passed from the caller.                                                                                                                                                                                                          |

***Return Values***

|                  |                                         |
|------------------|-----------------------------------------|
| **Return Value** | **Description**                         |
| 0                | The request completed successfully.     |
| -1               | Error and errno is set for more details |

**1.15 cblk\_listio**

***Purpose***

Issues multiple I/O requests to CAPI flash disk with a single call and/or waits for multiple I/O requests from a CAPI flash disk to complete..

***Syntax***

\#include &lt;capiblock.h&gt; for linux or &lt;sys/capiblock.h&gt; for AIX

typedef struct cblk\_io {

uchar version; /\* Version of structure \*/

\#define CBLK\_IO\_VERSION\_0 “I” /\* Initial version 0 \*/

int flags; /\* Flags for request \*/

\#define CBLK\_IO\_USER\_TAG 0x0001 /\* Caller is specifying a user defined \*/

/\* tag. \*/

\#define CBLK\_IO\_USER\_STATUS 0x0002 /\* Caller is specifying a status location \*/

/\* to be updated \*/

\#define CBLK\_IO\_PRIORITY\_REQ 0x0004/\* This is (high) priority request that \*/

/\* should be expediated vs non-priority \*/

/\* requests \*/

uchar request\_type; /\* Type of request \*/

\#define CBLK\_IO\_TYPE\_READ 0x01 /\* Read data request \*/

\#define CBLK\_IO\_TYPE\_WRITE 0x02 /\* Write data request \*/

void \*buf; /\* Data buffer for request \*/

offset\_t lba; /\* Starting Logical block address for \*/

/\* request. \*/

size\_t nblocks; /\* Size of request based on number of \*/

/\* blocks. \*/

int tag; /\* Tag for request. \*/

cblk\_arw\_status\_t stat; /\* Status of request \*/

} cblk\_io\_t

int rc = cblk\_listio(chunk\_id\_t chunk\_id,cblk\_io\_t \*issue\_io\_list\[\],int issue\_items,cblk\_io\_t \*pending\_io\_list\[\],int pending\_items, cblk\_io\_t \*wait\_io\_list\[\], int wait\_items, cblk\_io\_t \*completion\_io\_list\[\],int \*completion\_items, uint64\_t timeout, int flags));

***Description***

This service provides an interface to issue multiple I/O requests with one call and/or poll for completions of multiple I/O requests via one call. The individual requests are specified by the **cblk\_io\_t** type, which includes a chunk id, data buffer, starting Logical Block Address, a transfer size in 4K blocks. This service can update the I/O requests associated cblk\_io\_t (i.e. update status, tags and flags based on disposition of the I/O request).

This service can not be used to check for completion on I/O requests issued via cblk\_aread or cblk\_awrite.

***Parameters***

<table>
<colgroup>
<col width="50%" />
<col width="50%" />
</colgroup>
<tbody>
<tr class="odd">
<td align="left"><strong>Parameter</strong></td>
<td align="left"><strong>Description</strong></td>
</tr>
<tr class="even">
<td align="left">chunk_id</td>
<td align="left">The handle for the chunk which these I/O requests are associated.</td>
</tr>
<tr class="odd">
<td align="left">issue_io_list</td>
<td align="left"><p>This specifies an array of I/O requests to issue to CAPI flash disks. Each individual array element of type cblk_io_t specifies an individual I/O request containing chunk id, data buffer, starting Logical Block Address, a transfer size in 4K blocks. These array elements can be updated by this service to indicate completion status and/or tags. The status field of the individual cblk_io_t array elements will be initialized by this service.</p>
<p>If this field is null, then this service can be used to wait for completions of other requests issued from previous cblk_listio calls by setting the pending_io_list</p></td>
</tr>
<tr class="even">
<td align="left">issue_io_items</td>
<td align="left">Specifies the number of array elements in the issue_io_list array</td>
</tr>
<tr class="odd">
<td align="left">pending_io_list</td>
<td align="left">This specifies an array of I/O requests that were issued via a previous cblk_listio request. This allows one to poll for I/O request completions, without requiring them to wait for all completions (i.e. setting the completion_io_list parameter).</td>
</tr>
<tr class="even">
<td align="left">pending_io_items</td>
<td align="left">Specifies the number of array elements in the pending_io_list array</td>
</tr>
<tr class="odd">
<td align="left">wait_io_list</td>
<td align="left">Array of I/O requests, which for which this service will block until these requests complete. These I/O requests must also be specified in the either the issue_io_list or the pending_io_list. If an I/O request in the issue_io_list fails to be issued due to invalid settings by the caller or no resources, then that I/O request's elements in the io_list will be updated to indicate this (its status will be left as CBLK_ARW_STAT_NOT_ISSUED) and this service will not wait on that I/O request. Thus all I/O requests in the wait_io_list which completed will have a status of CBLK_ARW_STAT_SUCCESS or CBLK_ARW_STAT_FAIL. I/O requests which did not complete will not have their status updated.</td>
</tr>
<tr class="even">
<td align="left">wait_items</td>
<td align="left">Specifies the number of array elements in the wait_io_list array</td>
</tr>
<tr class="odd">
<td align="left">completion_io_list</td>
<td align="left">The caller will set this to an initialized (zeroed) array of I/O requests and set the completion_items to the number of array elements in this array. When this service returns the array will contain I/O requests specified in the issue_io_list and/or pending_io_list that were completed by the CAPI device, but which were not specified in the wait_io_list. If an I/O request in the io_list fails to be issued due to invalid settings by the caller or no resources, then that I/O requests element will not be copied to the completion_io_list and its status in the io_list will be updated to indicate this (its status will be left as CBLK_ARW_STAT_NOT_ISSUED) . Thus all I/O requests returned in this list will have a status of CBLK_ARW_STAT_SUCCESS or CBLK_ARW_STAT_FAIL.</td>
</tr>
<tr class="even">
<td align="left">completion_items</td>
<td align="left">The caller sets this to the address of the number of array elements it placed in the completion_io_list. When this service returns, this value is updated to the number of I/O requests placed in the completion_io_list.</td>
</tr>
<tr class="odd">
<td align="left">timeout</td>
<td align="left">Timeout in microseconds to wait for all I/O requests in the wait_io_list. This is only valid if the wait_io_list is not null. If any of the I/O requests in the wait_io_list do not complete within the time-out value, then this service returns a value of -1 and sets errno to a value <strong>ETIMEDOUT</strong> (when this occurs some commands may have completed in the wait_io_list, Thus the caller needs to check each request in the wait_io_list to determine which ones completed.) It is the caller's responsibility to remove completed items from the pending_io_list before the next invocation of this service. A timeout value of 0, indicates that this service will block until requests in the wait_io_list complete.</td>
</tr>
<tr class="even">
<td align="left">flags</td>
<td align="left">This is a collection of bit flags. The <strong>CBLK_LISTIO_WAIT_ISSUE_CMD</strong> flagswill cause this service to block to wait for a free commands to issue all the requests even if the timeout value is exceeded and <strong>CBLK_LISTIO_WAIT_CMD_FLAG</strong> is set. Otherwise this service could return a value of -1 with an errno of <strong>EWOULDBLOCK</strong>if there are not enough free commands currently available (for this situation, some commands may have successfully queued. The caller would need to examine the individual I/O requests in the issue_io_list to determine which ones failed)</td>
</tr>
</tbody>
</table>

***Return Values***

|                  |                                         |
|------------------|-----------------------------------------|
| **Return Value** | **Description**                         |
| -1               | Error and errno is set for more details |
| 0                | This service completed without error.   |


