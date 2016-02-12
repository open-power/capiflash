/* IBM_PROLOG_BEGIN_TAG                                                   */
/* This is an automatically generated prolog.                             */
/*                                                                        */
/* $Source: src/kv/arkdb.h $                                              */
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
#ifndef __ARKDB_H__
#define __ARKDB_H__

#include <stdint.h>
#include <sys/types.h>
#include <inttypes.h>
#include <pthread.h>

typedef uint64_t ARK;
#define ARC ARK
typedef uint64_t ARI;

// Bits that can be set in the flags parameter of ark_create
// and ark_create_verbose

// By default, a physical LUN will be used for the ARK
// store.  To use a virtual LUN on the provided device
// use the following flag.
#define ARK_KV_VIRTUAL_LUN    0x0000000000000001

// Will this store have persistence enabled.  By
// default persistence is not done.
#define ARK_KV_PERSIST_STORE  0x0000000000000002

// Ignore persistence data (if present) and start
// from scratch
#define ARK_KV_PERSIST_LOAD   0x0000000000000004


int ark_create_verbose(char *path, ARK **ark,
                       uint64_t size, uint64_t bsize, 
                       uint64_t hcount, int nthreads, int nqueue, 
                       int basyncs, uint64_t flags);

/**
 * @brief Create a key value store instance
 *
 * The ark_create API will create a key value store instance on the
 * host system.
 *
 * The path parameter can be used to specify a specific CAPI adapter.
 * If the path parameter is not a CAPI adapter, the API will assume it
 * is a file to be used for the key value store. If path is NULL,
 * memory will be used for the key value store.
 *
 * Upon successful completion, the handle parameter will represent the
 * newly created key value store instance to be used for future
 * API calls.
 *
 * @param path Allows the user to specify a specific CAPI adapter
 *             a file or memory for the key value store
 * @param ark  Handle representing the key value store
 *
 * @param flags Bit field to hold configuration options for
 *              the ARK instance that is to be created.
 *
 * @return Upon successful completion, the ark_create API will return
 *         0, and the handle parameter will point to the newly created
 *         key value store instance.  If unsuccessful, the ark_create
 *         API will return a non-zero error code:
 *
 * @retval EINVAL Invalid value for one of the parameters
 * @retval ENOMEM Not enough memory
 * @retval ENOSPC Not enough flash storage
 * @retval ENOTREADY System not ready for key value store support
 */
int ark_create(char *path, ARK **ark, uint64_t flags);

/**
 * @brief Delete a key value store instance
 *
 * The ark_delete API will delete a key value store instance specified
 * by the ark parameter, on the host system.  Upon successful
 * completion all associated in memory and storage resources will
 * be released at this time.
 *
 * @param ark  Handle representing the key value store
 *
 * @return Upon successful completion, the ark_delete API will clean
 *         and remove all resources associated with the key value
 *         store instance and return 0.  If unsuccessful, the
 *         ark_delete API will return a non-zero error code.
 *
 * @retval EINVAL Invalid value for one of the parameters
 */
int ark_delete(ARK *ark);

int ark_connect_verbose(ARC **arc, ARK *ark, int nasync);

int ark_connect(ARC **arc, ARK *ark);

int ark_disconnect(ARC *arc);

/**
 * @brief Write a key/value pair
 *
 * The ark_set API will store the key, key, and value, val, into
 * the store for the key/value store instance represented by the
 * ark parameter.  The API, ark_set_async_cb, will behave in the same
 * manner, but in an asynchronous fashion, where the API immediately
 * returns to the caller and the actual operation is scheduled to
 * run.  After the operation is executed, the callback function will
 * be called to notify the caller of completion.
 *
 * If the key is present, the store value will be replaced with the
 * val value.
 *
 * Upon successful completion, the key/value pair will be present in
 * the store and the number of bytes written will be returned to
 * the caller through the res parameter.
 *
 * @param ark Handle representing the key/value store instance
 * @param klen Length of the key in bytes
 * @param key Key
 * @param vlen Length of the value in bytes
 * @param val Value
 * @param res Upon success, number of bytes written to the store
 * @param callback Function to call upon completion of the I/O operation
 * @param dt 64bit value to tag an asynchronous API call.
 *
 * @return Upon successful completion, the ark_set and ark_set_async_cb
 * API will write the key/value in the store associated with the
 * key/value store instance and return the number of bytes written.
 * The return of ark_set will indicate the status of the operation.  The
 * ark_set_async_cb API return will indicate whether the asynchronous
 * operation was accepted or rejected.  The true status will be stored
 * in the errocde parameter when the callback function is executed.
 * if unsuccessful, the ark_set and ark_set_async_cb API will return
 * a non-zero error code.
 *
 * @retval EINVAL Invalid value for one of the parameters
 * @retval ENOSPC Not enough space left in key/value store
 */
int ark_set_async_cb(ARK *ark, uint64_t klen, void *key, uint64_t vlen, 
                     void *val, 
                     void (*cb)(int errcode, uint64_t dt, int64_t res), 
                     uint64_t dt);

int ark_set(ARK *ark, uint64_t klen, void *key, uint64_t vlen, 
            void *val, int64_t *res);

/**
 * @brief Retrieve a value for a given key
 *
 * The ark_get API will query th ekey/value store associated with the
 * ark parameter for the given key, key.  If found, the key's value will
 * be returned in the vbuf parameter with at most vbuflen bytes written
 * starting at the offset, voff, in the key's value.  The API,
 * ark_get_async_cb, will behave in the same manner, but in an
 * asynchronous fashion, where the API immediately returns to the 
 * caller and the actual operation is scheduled to run.  
 * After the operation is executed, the callback
 * function will be called to notify the caller of completion.
 * 
 *
 * Upon successful completion, the length of the key's value will
 * be stored in the res parameter.
 *
 * @param ark Handle representing the key/value store instance
 * @param klen Length of the key in bytes
 * @param key Key
 * @param vbuflen Length of the buffer, vbuf
 * @param vbuf Buffer to store the key's value
 * @param voff Offset into the key to start reading.
 * @param res Upon success, size of the key value
 * @param callback Function to call upon completion of the I/O operation
 * @param dt 64bit value to tag an asynchronous API call.
 *
 * @return Upon successful completion, the ark_get and ark_get_async_cb
 * API will return 0.  THe return of ark_get will indicate the status
 * of the operation.  The ark_get_async_cb API return will indicate
 * whether the asynchronous operation was accepted or rejected.  The
 * true status of the asynchronous API will be stored in the errocde
 * parameter of the callback function.  If unsuccessful, the ark_get
 * and ark_get_async_cb API will return a non-zero error code
 *
 * @retval EINVAL Invalid value for one of the parameters
 * @retval ENOSPC Not enough space left in key/value store
 */
int ark_get_async_cb(ARK *ark, uint64_t klen, void *key, uint64_t vbuflen,
                     void *vbuf, uint64_t voff, 
                     void (*cb)(int errcode, uint64_t dt,int64_t res), 
                     uint64_t dt);
int ark_get(ARK *ark, uint64_t klen, void *key, uint64_t vbuflen, 
            void *vbuf, uint64_t voff, int64_t *res);

/**
 * @brief Delete the value associated with a given key
 *
 * The ark_del API will query the key/value store associated with the
 * handle parameter for the given key, key, and if found, will delete
 * the value.  The API, ark_del_async_cb, will behave in the same
 * manner, but in an asynchronous fashion, where the API immediately
 * returns to the caller and the actual operation is scheduled to run.
 * After the operation is executed, the callback function will be
 * called to notify the caller of completion.
 *
 * Upon successful completion, the length of the key's value will
 * be stored in the res parameter.
 *
 * @param ark Handle representing the key/value store instance
 * @param klen Length of the key in bytes
 * @param key Key
 * @param res Upon success, size of the key value
 * @param callback Function to call upon completion of the I/O operation
 * @param dt 64bit value to tag an asynchronous API call.
 *
 * @return Upon successful completion, the ark_del and ark_del_async_cb
 * API will return 0.  THe return of ark_del will indicate the status
 * of the operation.  The ark_del_async_cb API return will indicate
 * whether the asynchronous operation was accepted or rejected.  THe
 * true status will be returned in the errcode parameter when the
 * callback function is executed.  If unsuccessful, the ark_del and
 * ark_del_async_cb API will return a non-zero error code:
 *
 * @retval EINVAL Invalid value for one of the parameters
 * @retval ENOSPC Not enough space left in key/value store
 */
int ark_del_async_cb(ARK *ark, uint64_t klen, void *key, 
                     void (*cb)(int errcode, uint64_t dt,int64_t res),
                     uint64_t dt);
int ark_del(ARK *ark, uint64_t klen, void *key, int64_t *res);

/**
 * @brief Query the key/value store to see if a given key is present
 *
 * The ark_exists API will query the key/value store associated with
 * the ark parameter for the given key, key, and if found,
 * return the size of the value in bytes in the res parameter.  The
 * key and it's value will not be altered.  THe API,
 * ark_exists_async_cb, will behave in the same manner, but in an
 * asynchronous fashion, where the API immediately returns to the
 * caller and the actual operation is scheduled to run.  After the
 * operation is executed, the callback function will be called
 * to notify the caller of completion.
 *
 * Upon successful completion, the length of the key's value will
 * be stored in the res parameter.
 *
 * @param ark Handle representing the key/value store instance
 * @param klen Length of the key in bytes
 * @param key Key
 * @param res Upon success, size of the key value
 * @param callback Function to call upon completion of the I/O operation
 * @param dt 64bit value to tag an asynchronous API call.
 *
 * @return Upon successful completion, the ark_exists and
 * ark_exists_async_cb API will return 0.  The return of ark_exists
 * will indicate the status of the operation.  The ark_exists_async_cb
 * API return will indicate whether the asynchronous operation was
 * accepted or rejected.  The true status will be returned in the 
 * errcode parameter when the callback function is executed.  If
 * unsuccessful, the ark_exists and ark_exists_async_cb API will
 * return a non-zero error code:
 *
 * @retval EINVAL Invalid value for one of the parameters
 * @retval ENOSPC Not enough space left in key/value store
 */
int ark_exists_async_cb(ARK *ark, uint64_t klen, void *key,
                        void (*cb)(int errcode, uint64_t dt, int64_t res),
                        uint64_t dt);
int ark_exists(ARK *ark, uint64_t klen, void *key, int64_t *res);


/**
 * @brief Return the first key and handle to iterate through the store
 *
 * The ark_first API will return the first key found in the store
 * in the store in the buffer, kbuf, and the size of the key in klen,
 * as long as the size is less than the size of the kbuf, kbuflen.
 * 
 *
 * Upon successful completion, an iterator handle will be returned
 * to the caller to be used to retrieve the next key in the store
 * by calling the ark_next API.
 *
 * @param ark Handle representing the key/value store instance
 * @param kbuflen Length of the kbuf parameter
 * @param klen Size of the key returned in kbuf
 * @param kbuf Buffer to hold the key
 *
 * @return Upon successful completion, the ark_first API will return
 * a handle to be used to iterate through the store on subsequent
 * calls using the ark_next API.  If unsuccessful, the ark_first API
 * will return NULL with errno set to one of the following:
 *
 * @retval EINVAL Invalid value for one of the parameters
 * @retval ENOSPC Not enough space left in key/value store
 */
ARI *ark_first(ARK *ark, uint64_t kbuflen, int64_t *klen, void *kbuf);

/**
 * @brief Return the next key in the store
 *
 * The ark_next API will return the next key found in the store based
 * on the iterator handle, iter, in the buffer, kbuf, and the size
 * of the key in klen, as long as the size is less than the size of
 * the kbuf, kbuflen.
 * 
 * Upon successful completion, a handle will be returned to the caller
 * to be used to retrieve the next key in the store by calling the
 * ark_next API.  If the end of the store is reached, a NULL, value
 * is returned and errno set to ENOENT. Because of the dynamic nature
 * of the store, some recently written keys may not be returned.
 *
 * @param iter Iterator handle where to begin search in store
 * @param kbuflen Length of the kbuf parameter
 * @param klen Size of the key returned in kbuf
 * @param kbuf Buffer to hold the key
 *
 * @return Upon successful completion, the ark_next API will return a
 * handle to be used to iterate through the store on subsequent calls
 * using the ark_next API.  If unsuccessful, the ark_next API will
 * a return a non-zero value of one of the following errors:
 *
 * @retval EINVAL Invalid value for one of the parameters
 * @retval ENOSPC Not enough space left in key/value store
 */
int ark_next(ARI *iter, uint64_t kbuflen, int64_t *klen, void *kbuf);

/**
 * @brief Return the number of bytes allocated in the store
 *
 * The ark_allocated API will return the number of bytes allocated
 * in the store in the size parameter.
 *
 * @param ark Handle representing the key/value store instance
 * @param size Will hold the size of the store in bytes
 *
 * @return Upon successful completion, the ark_allocated API will return
 * zero.  If unsuccessful, the ark_allocated API will return one
 * of the following error codes:
 *
 * @retval EINVAL Invalid value for one of the parameters
 * @retval ENOSPC Not enough space left in key/value store
 */
int ark_allocated(ARK *ark, uint64_t *size);

/**
 * @brief Return the number of bytes in use in the store
 *
 * The ark_inuse API will return the number of bytes in use in the
 * store in the size parameter.
 *
 * @param ark Handle representing the key/value store instance
 * @param size Will hold the size of the number of blcoks in use. Size
 * will be in bytes
 *
 * @return Upon successful completion, the ark_inuse API will return
 * zero.  If unsuccessful, the ark_allocated API will return one
 * of the following error codes:
 *
 * @retval EINVAL Invalid value for one of the parameters
 */
int ark_inuse(ARK *ark, uint64_t *size);

/**
 * @brief Return the actual number of bytes in use in the store
 *
 * The ark_actual API will return the actual number of bytes in use
 * in the store in the size parameter.  This differs from the ark_inuse
 * API as this takes into account the actual sizes of the individual
 * keys and their values instead of generic allocations based on blocks
 * to store these values.
 *
 * @param ark Handle representing the key/value store instance
 * @param size Will hold the actual number of bytes in use in the store
 *
 * @return Upon successful completion, the ark_actual API will return
 * zero.  If unsuccessful, the ark_actual API will return one of
 * the following error codes:
 *
 * @retval EINVAL Invalid value for one of the parameters
 */
int ark_actual(ARK *ark, uint64_t *size);

/**
 * @brief Return the count of the number of keys in the key/value store
 *
 * The ark_count API will return the total number of keys in the store
 * based on the handle, ark, and store the result in the count
 * parameter.
 *
 * @param ark Handle representing the key/value store instance
 * @param count Number of keys found in the key/value store
 *
 * @return Upon successful completion, ark_count will return zero.
 * Otherwise, a non-zero error code will be returned:
 *
 * @retval EINVAL Invalid value for one of the parameters
 */
int ark_count(ARK *ark, int64_t *count);

/**
 * @brief Return a random key from the key/value store
 *
 * The ark_random API will return a random key found in the store
 * based on the handle, ark, in the buffer, kbuf, and the size of
 * the key in klen, as long as the size is less than the size of
 * the kbuf, kbuflen.
 *
 * @param ark Handle representing the key/value store instance
 * @param kbuflen Length of the kbuf parameter
 * @param klen Size of th ekey returned in kbuf
 * @param kbuf Buffer to hold the key
 *
 * @return Upon successful completion, ark_random will zero.  Otherwise,
 * ark_random will return the following error codes:
 *
 * @retval EINVAL Invalid value for one of the parameters
 */
int ark_random(ARK *ark, uint64_t kbuflen, uint64_t *klen, void *kbuf);

/**
 * @brief Flush the contents of the key/value store
 *
 * The ark_flush API will flush the contents of the key/value store
 * represented by the ark parameter.
 *
 * Note, this will not actually write or clear data off the flash device
 *
 * @param ark Handle representing the key/value store instance
 *
 * @return Upon successful completion, the ark_flush API will return
 * zero.  If unsuccessful, the ark_allocated API will return one
 * of the following error codes:
 *
 * @retval EINVAL Invalid value for one of the parameters
 */
int ark_flush(ARK *ark);

/**
 * @brief Fork a key/value store for archiving purposes
 *
 * The ark_fork and ark_fork_done API's are to be called by the parent
 * key/value store process to prepare the key/value store to be forked
 * fork the child process, and to perform any cleanup once it has been
 * detected the child process has exited.
 *
 * The ark_fork API will fork a child process and upon return, will
 * return the process ID of the child in the parent process, and
 * 0 in the child process.  Once the parent detects the child has
 * exited, a call to ark_fork_done will be needed to clean up any
 * state from the ark_fork call.
 *
 * @param ark Handle representing the key/value store instance
 *
 * @return Upon successful completion, ark_fork and ark_fork_done
 * will return zero, otherwise one of the following errors:
 *
 * @retval EINVAL Invalid value for one of the parameters
 */
pid_t ark_fork(ARK *ark);
int ark_fork_done(ARK *ark);

/**
 * @brief Return the number of key/value ops and number of block ops
 *
 * The ark_stats API will return the total number of key/value
 * operations in the ops parameter, and number of block operations
 * in the ios parameter.
 *
 * @param ark Handle representing the key/value store instance
 * @param ops Number of key/value operations
 * @param ios Number of block operations
 *
 * @return Upon successful completion, the ark_stats API will return
 * zero.  If unsuccessful, the ark_allocated API will return one
 * of the following error codes:
 *
 * @retval EINVAL Invalid value for one of the parameters
 */
int ark_stats(ARK *ark, uint64_t *ops, uint64_t *ios);

#endif

