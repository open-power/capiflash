**ark\_create API**
===================

Purpose
-------

Create a key/value store instance

Syntax
------

int ark\_create(file, ark, flags)

char * file;

ARK ** handle;

uint64\_t flags;

Description
-----------

The **ark\_create** API will create a key/value store instance on the host system.

The **path** parameter can be used to specify the special file (e.g. /dev/sdx) representative of the physical LUN created on the flash store. If the **path** parameter is not a special file, the API will assume it is a file to be used for the key/value store. If the file does not exist, it will be created. If **path** is NULL, memory will be used for the key/value store.

The parameter, **flags**, will indicate the properties of the KV store. In the case of specifying a special file for the physical LUN, the user can specify whether the KV store is use the physical LUN as is or to create the KV store in a virtual LUN. By default, the entire physical LUN will be used for the KV store. If a virtual LUN is desired, the **ARK\_KV\_VIRTUAL\_LUN** bit must be set in the flags parameter.

In this revision, a KV store configured to use the entire physical LUN can be persisted. Persistence of a KV store allows the user to shut down an ARK instance and at a later time open the same physical LUN and load the previous ARK instance to the same state as it was when it closed. To configure an ARK instance to be persisted at shut down (ark\_delete), set the **ARK\_KV\_PERSIST\_STORE** bit in the flags parameter. By default, an ARK instance is not configured to be persisted. To load the persisted ARK instance resident on the physical LUN, set the **ARK\_KV\_PERSIST\_LOAD** bit in the flags parameter. By default, the persisted store, if present, will not be loaded and will potentially be overwritten by any new persisted data.

In this revision, only physical LUN KV stores can be persisted.

Upon successful completion, the handle parameter will represent the newly created key/value store instance to be used for future API calls.

Parameters
----------

|               |                                                                                                                                                                |
|---------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **Parameter** | **Description**                                                                                                                                                |
| path          | Allows the user to specify a specific CAPI adapter, a file, or memory for the key/value store.                                                                 |
| ark           | Handle representing the key/value store                                                                                                                        |
| flags         | Collection of bit flags determining the properties of the KV store.                                                                                            
                |                                                           
 |              |           **- ARK_KV_VIRTUAL_LUN**: KV store will use a virtual LUN created from the physical LUN represented by the special file, **file.**                              
                 |                                                              |                                                                                                  
                |           | **- ARK\_KV\_PERSIST\_STORE:** <span style="font-weight: normal">Configure the ARK instance to be persisted upon closing (ark\_delete)</span>                   
                                                                                                   |                                                                                      
              |         |  **- ARK\_KV\_PERSIST\_LOAD:** <span style="font-weight: normal">If persistence data is present on the physical LUN, then load the configuration stored.</span>  |

Return Values
-------------

Upon successful completion, the **ark\_create** API will return 0, and the handle parameter will point to the newly created key/value store instance. If unsuccessful, the **ark\_create** API will return a non-zero error code:

|           |                                                              |
|-----------|--------------------------------------------------------------|
| **Error** | **Reason**                                                   |
| EINVAL    | Invalid value for one of the parameters                      |
| ENOSPC    | Not enough memory or flash storage                           |
| ENOTREADY | System not ready for key/value store supported configuration |
|           |                                                              |

**ark\_delete API**
===================

Purpose
-------

Delete a key/value store instance

Syntax
------

int ark\_delete(ark)

ARK * ark;

Description
-----------

The **ark\_delete** API will delete a key/value store instance, specified by the **ark** parameter, on the host system. Upon successfully completion all associated in memory and storage resources will be released at this time.

If the ARK instance is configured to be persisted, it is at this time the configuration will be persisted so that it may be loaded at a future time.

Parameters
----------

|               |                                                   |
|---------------|---------------------------------------------------|
| **Parameter** | **Description**                                   |
| ark           | Handle representing the key/value store instance. |

Return Values
-------------

Upon successful completion, the **ark\_delete** API will clean and remove all resources associated with the key/value store instance and return 0. If unsuccessful, the **ark\_delete** API will return a non-zero error code:

|           |                                     |
|-----------|-------------------------------------|
| **Error** | **Reason**                          |
| EINVAL    | key/value store handle is not valid |
|           |                                     |

**ark\_set, ark\_set\_async\_cb API**
=====================================

Purpose
-------

Write a key/value pair

Syntax
------

int ark\_set(ark, klen, key, vlen, val, res)

int ark\_set\_async\_cb(ark, klen, key, vlen, val, callback, dt)

ARK * ark;

uint64\_t klen;

void * key;

uint64\_t vlen;

void * val;

void *(*callback)(int errcode, uint64\_t dt, uint64\_t res);

uint64\_t dt;

Description
-----------

The **ark\_set** API will store the key, **key**, and value, **val**, into the store for the key/value store instance represented by the **ark** parameter The API, **ark\_set\_async\_cb**, will behave in the same manner, but in an asynchronous fashion, <span style="font-weight: normal">where the API immediately returns to the caller and the actual operation is scheduled to run. A</span>fter the operation is executed, the **callback** function will be called to notify the caller of completion.

If the **key** is present, the stored value will be replaced with the **val** value.

Upon successful completion, the key/value pair will be present in the store and the number of bytes written will be returned to the caller through the **res** parameter.

Parameters
----------

|               |                                                             |
|---------------|-------------------------------------------------------------|
| **Parameter** | **Description**                                             |
| ark           | Handle representing the key/value store instance connection |
| klen          | Length of the key in bytes.                                 |
| key           | Key                                                         |
| vlen          | Length of the value in bytes.                               |
| val           | Value                                                       |
| res           | Upon success, number of bytes written to the store.         |
| callback      | Function to call upon completion of the I/O operation.      |
| dt            | 64bit value to tag an asynchronous API call.                |

Return Values
-------------

Upon successful completion, the **ark\_set** and **ark\_set\_async\_cb**API will write the key/value in the store associated with the key/value store instance and return the number of bytes written. The return of **ark\_set** will indicate the status of the operation. The **ark\_set\_async\_cb** API return will indicate whether the asynchronous operation was accepted or rejected. The true status will be stored in the **errcode** parameter when the **callback** function is executed. If unsuccessful, the **ark\_set** and **ark\_set\_async\_cb** API will return a non-zero error code:

|           |                                                |
|-----------|------------------------------------------------|
| **Error** | **Reason**                                     |
| EINVAL    | Invalid parameter                              |
| ENOSPC    | Not enough space left in key/value store store |
|           |                                                |

**ark\_get, ark\_get\_async\_cb API**
=====================================

Purpose
-------

Retrieve a value for a given key

Syntax
------

int ark\_get(ark, klen, key, vbuflen, vbuf, voff, res)

int ark\_get\_async\_cb(ark, klen, key, vbuflen, vbuf, voff, callback, dt)

ARK * ark;

uint64\_t klen;

void * key;

uint64\_t vbuflen;

void * vbuf;

uint64\_t voff;

void *(\*callback)(int errcode, uint64\_t dt, uint64\_t res);

uint64\_t dt;

Description
-----------

The **ark\_get** and **ark\_get\_async\_cb** API will query the key/value store store associated with the **ark** paramter for the given key, **key**. If found, the key's value will be returned in the **vbuf** parameter with at most **vbuflen** bytes written starting at the offset, **voff,** <span style="font-weight: normal">in the key's value. The API, </span>**ark\_get\_async\_cb**<span style="font-weight: normal">, will behave in the same manner, but in an asynchronous fashion, where the API immediately returns to the caller and the actual operation is scheduled to run. After the operation is executed, the </span>**callback**<span style="font-weight: normal"> function will be called to notify the caller of completion.</span>

If successful, the length of the key's value is stored in the **res** parameter of the callback function.

Parameters
----------

|               |                                                              |
|---------------|--------------------------------------------------------------|
| **Parameter** | **Description**                                              |
| ark           | Handle representing the key/value store instance connection. |
| klen          | Length of the key in bytes.                                  |
| key           | Key                                                          |
| vbuflen       | Length of the buffer, vbuf                                   |
| vbuf          | Buffer to store the key's value                              |
| voff          | Offset into the key to start reading.                        |
| res           | If successful, will store the size of the key in bytes       |
| callback      | Function to call upon completion of the I/O operation.       |
| dt            | 64bit value to tag an asynchronous API call.                 |

Return Values
-------------

Upon successful completion, the **ark\_get** and **ark\_get\_async\_cb** API will return 0. The return of **ark\_get** will indicate the status of the operation. The **ark\_get\_async\_cb** API return will indicate whether the asynchronous operation was accepted or rejected. The true status of the asynchronous API will be stored in the **errcode** parameter of the **callback** function. If unsuccessful, the **ark\_get** and **ark\_set\_async\_cb** API will return a non-zero error code:

|           |                                     |
|-----------|-------------------------------------|
| **Error** | **Reason**                          |
| EINVAL    | Invalid parameter                   |
| ENOENT    | Key not found                       |
| ENOSPC    | Buffer not big enough to hold value |

**ark\_del, ark\_del\_async\_cb API**
=====================================

Purpose
-------

Delete the value associated with a given key

Syntax
------

int ark\_del(ark, klen, key, res)

int ark\_del\_async\_cb(ark, klen, key, callback, dt)

ARK * ark;

uint64\_t klen;

void * key;

void *(\*callback)(int errcode, uint64\_t dt, uint64\_t res);

uint64\_t dt;

Description
-----------

The **ark\_del** and **ark\_del\_async\_cb** API will query the key/value store store associated with the **handle** paramter for the given key, **key,** <span style="font-weight: normal">and if found, will delete the value. The API, </span>**ark\_del\_async\_cb**<span style="font-weight: normal">, will behave in the same manner, but in an asynchronous fashion, where the API immediately returns to the caller and the actual operation is scheduled to run. After the operation is executed, the </span>**callback**<span style="font-weight: normal"> function will be called to notify the caller of completion.</span>

If successful, the length of the key's value is returned to the caller in the **res** parameter of the callback function.

Parameters
----------

|               |                                                              |
|---------------|--------------------------------------------------------------|
| **Parameter** | **Description**                                              |
| ark           | Handle representing the key/value store instance connection. |
| klen          | Length of the key in bytes.                                  |
| key           | Key                                                          |
| res           | If successful, will store the size of the key in bytes       |
| callback      | Function to call upon completion of the I/O operation.       |
| dt            | 64bit value to tag an asynchronous API call.                 |

Return Values
-------------

Upon successful completion, the **ark\_del** and **ark\_del\_async\_cb** API will return 0. The return of **ark\_del** will indicate the status of the operation. The **ark\_del\_async\_cb** API return will indicate whether the asynchronous operation was accepted or rejected. The true status will be returned in the **errcode** parameter when the **callback** function is executed. If unsuccessful, the **ark\_del** and **ark\_del\_async\_cb** API will return a non-zero error code:

|           |                   |
|-----------|-------------------|
| **Error** | **Reason**        |
| EINVAL    | Invalid parameter |
| ENOENT    | Key not found     |
|           |                   |

**ark\_exists, ark\_exists\_async\_cb API**
===========================================

Purpose
-------

Query the key/value store store to see if a given key is present

Syntax
------

int ark\_exists(ark, klen, key, res)

int ark\_exists\_async\_cb(ark, klen, key, callback, dt)

ARK * ark;

uint64\_t klen;

void * key;

void *(\*callback)(int errcode, uint64\_t dt, uint64\_t res);

uint64\_t dt;


Description
-----------

The **ark\_exists** and **ark\_exists\_async\_cb** API will query the key/value store store associated with the **ark or arc** paramter for the given key, **key,** <span style="font-weight: normal">and if found, return the size of the value in bytes in the </span>**res**<span style="font-weight: normal"> parameter. The key and it's value will not be altered. The API, </span>**ark\_exists\_async\_cb**<span style="font-weight: normal">, will behave in the same manner, but in an asynchronous fashion, where the API immediately returns to the caller and the actual operation is scheduled to run. After the operation is executed, the </span>**callback**<span style="font-weight: normal"> function will be called to notify the caller of completion.</span>

Parameters
----------

|               |                                                              |
|---------------|--------------------------------------------------------------|
| **Parameter** | **Description**                                              |
| ark           | Handle representing the key/value store instance connection. |
| klen          | Length of the key in bytes.                                  |
| key           | Key                                                          |
| res           | If successful, will store the size of the key in bytes       |
| callback      | Function to call upon completion of the I/O operation.       |
| dt            | 64bit value to tag an asynchronous API call.                 |

Return Values
-------------

Upon successful completion, the **ark\_exists** and **ark\_exists\_async\_cb** API will return 0. The return of **ark\_exists** will indicate the status of the operation. The **ark\_exists\_async\_cb** API return will indicate whether the asynchronous operation was accepted or rejected. The true status will be returned in the **errcode** <span style="font-weight: normal">parameter when the </span>**callback** <span style="font-weight: normal">function is executed. If</span> unsuccessful, the **ark\_exists and ark\_exists\_async\_cb** API will return a non-zero error code:

|           |                   |
|-----------|-------------------|
| **Error** | **Reason**        |
| EINVAL    | Invalid parameter |
| ENOENT    | Key not found     |
|           |                   |

**ark\_first API**
==================

Purpose
-------

Return the first key and handle to iterate through store.

Syntax
------

ARI \* ark\_first(ark, kbuflen, klen, kbuf)

ARK * ark;

uint64\_t kbuflen

int64\_t klen;

void * kbuf;


Description
-----------

The **ark\_first** API will return the first key found in the store in the buffer, **kbuf**, and the size of the key in **klen**, as long as the size is less than the size of the kbuf, **kbuflen**.

If successful, an iterator handle will be returned to the caller to be used to retrieve the next key in the store by calling the **ark\_next** API.

Parameters
----------

|               |                                                              |
|---------------|--------------------------------------------------------------|
| **Parameter** | **Description**                                              |
| ark           | Handle representing the key/value store instance connection. |
| kbuflen       | Length of the kbuf parameter.                                |
| klen          | Size of the key returned in kbuf                             |
| kbuf          | Buffer to hold the key                                       |

Return Values
-------------

Upon successful completion, the **ark\_first** API will return a handle to be used to iterate through the store on subsequent calls using the **ark\_next** API. If unsuccessful, the **ark\_first** API will return NULL with **errno** set to one of the following:

|           |                               |
|-----------|-------------------------------|
| **Error** | **Reason**                    |
| EINVAL    | Invalid parameter             |
| ENOSPC    | kbuf is too small to hold key |
|           |                               |

**ark\_next API**
=================

Purpose
-------

Return the next key in the store.

Syntax
------

ARI \* ark\_next(iter, kbuflen, klen, kbuf)

ARI * iter;

uint64\_t kbuflen

int64\_t klen;

void * kbuf;


Description
-----------

The **ark\_next** API will return the next key found in the store based on the iterator handle, **iter**, in the buffer, **kbuf**, and the size of the key in **klen**, as long as the size is less than the size of the kbuf, **kbuflen**.

If successful, a handle will be returned to the caller to be used to retrieve the next key in the store by calling the **ark\_next** API. If the end of the store is reached, a NULL value is returned and errno set to **ENOENT**.

Because of the dynamic nature of the store, some recently written keys may not be returned.

Parameters
----------

|               |                                                |
|---------------|------------------------------------------------|
| **Parameter** | **Description**                                |
| iter          | Iterator handle where to begin search in store |
| kbuflen       | Length of the kbuf parameter.                  |
| klen          | Size of the key returned in kbuf               |
| kbuf          | Buffer to hold the key                         |

Return Values
-------------

Upon successful completion, the **ark\_next** API will return a handle to be used to iterate through the store on subsequent calls using the **ark\_next** API. If unsuccessful, the **ark\_next** API will return NULL with **errno** set to one of the following:

|           |                                    |
|-----------|------------------------------------|
| **Error** | **Reason**                         |
| EINVAL    | Invalid parameter                  |
| ENOSPC    | kbuf is too small to hold key      |
| ENOENT    | End of the store has been reached. |

**ark\_allocated API**
======================

Purpose
-------

Return the number of bytes allocated in the store.

Syntax
------

int ark\_allocated(ark, size)

ARK * ark;

uint64\_t *size;

Description
-----------

The **ark\_allocated** API will return the number of bytes allocated in the store in the **size**<span style="font-weight: normal"> parameter.</span>

Parameters
----------

|               |                                                   |
|---------------|---------------------------------------------------|
| **Parameter** | **Description**                                   |
| ark           | Handle representing the key/value store instance. |
| size          | Will hold the size of the store in bytes          |

Return Values
-------------

Upon successful completion, the **ark\_allocated** API will return 0. If unsuccessful, the **ark\_allocated** API will return one of the following error codes.

|           |                   |
|-----------|-------------------|
| **Error** | **Reason**        |
| EINVAL    | Invalid parameter |

**ark\_inuse API**
==================

Purpose
-------

Return the number of bytes in use in the store.

Syntax
------

int ark\_inuse(ark, size)

ARK *ark;

uint64\_t *size;

Description
-----------

The **ark\_inuse** API will return the number of bytes in use in the store in the **size**<span style="font-weight: normal"> parameter.</span>

Parameters
----------

|               |                                                                       |
|---------------|-----------------------------------------------------------------------|
| **Parameter** | **Description**                                                       |
| ark           | Handle representing the key/value store instance.                     |
| size          | Will hold the size of number of blocks in use. Size will be in bytes. |

Return Values
-------------

Upon successful completion, the **ark\_inuse** API will return 0. If unsuccessful, the **ark\_inuse** API will return one of the following error codes:

|           |                   |
|-----------|-------------------|
| **Error** | **Reason**        |
| EINVAL    | Invalid parameter |

**ark\_actual API**
===================

Purpose
-------

Return the actual number of bytes in use in the store.

Syntax
------

int ark\_actual(ark, size)

ARK *ark;

uint64\_t *size;


Description
-----------

The **ark\_actual** API will return the actual number of bytes in use in the store in the size parameter. This differs from the **ark\_inuse** API as this takes into account the actual sizes of the individual keys and their values instead of generic allocations based on blocks to store these values.

Parameters
----------

|               |                                                           |
|---------------|-----------------------------------------------------------|
| **Parameter** | **Description**                                           |
| ark           | Handle representing the key/value store instance.         |
| size          | Will hold the actual number of bytes in use in the store. |

Return Values
-------------

Upon successful completion, the **ark\_actual** API will return the 0. If unsuccessful, the **ark\_actual** API will return one of the following error codes:

|           |                   |
|-----------|-------------------|
| **Error** | **Reason**        |
| EINVAL    | Invalid parameter |

**ark\_error, ark\_errorstring API**
====================================

Purpose
-------

Return additional error information on a failure.

Syntax
------

int ark\_error(ark)

ARK * ark;

char * ark\_errorstring(ark)

ARK * ark;

Description
-----------

The **ark\_error** API will return a more descriptive error code for the last error encountered on a key/value store API.

The **ark\_errorstring** API will return a human readable error string for the last error encountered on a key/value store API.

Parameters
----------

|               |                                                   |
|---------------|---------------------------------------------------|
| **Parameter** | **Description**                                   |
| ark           | Handle representing the key/value store instance. |

Return Values
-------------

Upon successful completion, **ark\_error** will return a non-zero value and **ark\_errorstring** will return a non-NULL value. If an error is encountered, **ark\_error** will return a negative error code and **ark\_errorstring** will return NULL with errno set to one of the following:

|           |                   |
|-----------|-------------------|
| **Error** | **Reason**        |
| EINVAL    | Invalid parameter |

**ark\_fork, ark\_fork\_done API**
==================================

Purpose
-------

Fork a key/value store for archiving purposes.

Syntax
------

int ark\_fork(ark)

int ark\_fork\_done(ark)

ARK * handle;

Description
-----------

The **ark\_fork** and **ark\_fork\_done** API's are to be called by the parent key/value store process to prepare the key/value store to be forked, fork the child process, and to perform any cleanup once it has been detected the child process has exited.

The **ark\_fork**<span style="font-weight: normal"> API will fork a child process and upon return, will return the process ID of the child in the parent process, and 0 in the child process. Once the parent detects the child has exited, a call to </span>**ark\_fork\_done**<span style="font-weight: normal"> will be needed to clean up any state from the </span>**ark\_fork**<span style="font-weight: normal"> call.</span>

<span style="font-weight: normal">Note, the </span>**ark\_fork**<span style="font-weight: normal"> API will fail if there are any outstanding asynchronous commands.</span>

Parameters
----------

|               |                                                   |
|---------------|---------------------------------------------------|
| **Parameter** | **Description**                                   |
| ark           | Handle representing the key/value store instance. |

Return Values
-------------

Upon successful completion, **ark\_fork** <span style="font-weight: normal">and</span> **ark\_fork\_done** will return 0, otherwise one of the following errors:

|           |                                     |
|-----------|-------------------------------------|
| **Error** | **Reason**                          |
| EINVAL    | Invalid parameter                   |
| EBUSY     | Outstanding asynchronous operations |
| ENOMEM    | Not enough space to clone store     |

**ark\_random API**
===================

Purpose
-------

Return a random key from the key/value store store.

Syntax
------

int ark\_random(ark, kbuflen, klen, kbuf)

ARK * <span style="font-weight: normal">ark;</span>

uint64\_t kbuflen;

int64\_t * klen;

void * kbuf;

Description
-----------

The **ark\_random** API will return a random key found in the store based on the handle, **ark**, in the buffer, **kbuf**, and the size of the key in **klen**, as long as the size is less than the size of the kbuf, **kbuflen**.

Parameters
----------

|               |                                                |
|---------------|------------------------------------------------|
| **Parameter** | **Description**                                |
| ark           | Handle respresenting the key/value store store |
| kbuflen       | Length of the kbuf parameter.                  |
| klen          | Size of the key returned in kbuf               |
| kbuf          | Buffer to hold the key                         |

Return Values
-------------

Upon successful completion, **ark\_random** will 0. Otherwise, **ark\_random** will return the following error codes:

|           |                   |
|-----------|-------------------|
| **Error** | **Reason**        |
| EINVAL    | Invalid parameter |

**ark\_count API**
==================

Purpose
-------

Return the count of the number of keys in the key/value store store

Syntax
------

int ark\_count(ark, int \*count)

ARK * ark;

int * count;

Description
-----------

The **ark\_count** API will return a the total number of keys in the store based on the handle, ark**,** <span style="font-weight: normal">and store the result in the </span>**count** <span style="font-weight: normal">parameter.</span>

Parameters
----------

|               |                                                   |
|---------------|---------------------------------------------------|
| **Parameter** | **Description**                                   |
| ark           | Handle representing the key/value store instance. |
| count         | Number of keys found in the key/value store.      |

Return Values
-------------

Upon successful completion, **ark\_count** will return 0. Otherwise, a non-zero error code will be returned:

|           |                   |
|-----------|-------------------|
| **Error** | **Reason**        |
| EINVAL    | Invalid parameter |




