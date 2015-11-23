#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "stplugin.h"

#define GET_FAILURE    -998 // return code for failure of shmget function
#define ATT_FAILURE    -997 // return code for failure of shmat function
#define THREAD_FAILURE -996 // return code for failure of threading function (pthread_*())

typedef enum DTYPE_CODES {LONG, DOUBLE} DTYPE;
typedef struct Segment {
    ST_int key;                   // the key associated with the shared memory
    ST_int dtype;                 // the data type associated with the shared memory
    ST_int varindex;              // the varindex in Stata to which data will be written
} Segment;

// reading functions. These take a list in shared memory and write them to the Stata data array
static ST_retcode read_long_list(key_t key, ST_int varindex);
static ST_retcode read_double_list(key_t key, ST_int varindex);
static void *thread_mgr(void *thread_args);

// main function. Calls readers and returns exit statuses
STDLL stata_call(int argc, char *argv[]) 
{
    int num_threads, thread_rc, ix;
    pthread_t *thread_ids;
    ST_retcode rc;
    ST_double key, dtype;
    int *thread_exit_codes;
    Segment *segments;

    // create an array of Segment structs to pass arguments to threads
    num_threads = SF_nvars();
    segments = malloc(num_threads * sizeof(Segment));
    if (segments == NULL) {
        SF_display("Operating system would not allocate memory\n");
        return 909;
    }

    // allocate an array of thread IDs
    thread_ids = malloc(num_threads * sizeof(pthread_t));
    if (thread_ids == NULL) {
        SF_display("Operating system would not allocate memory\n");
        free(segments);
        return 909;
    }

    // extract parameters from Stata matrices and load them into structs to pass to threads
    for (ix = 0; ix < num_threads; ix++) {
        if ((rc = SF_mat_el("_shm_keys", ix+1, 1, &key))) {
            SF_display("Error accessing shared memory keys\n");
            free(segments);
            free(thread_ids);
            return rc;
        }

        if ((rc = SF_mat_el("_shm_dtypes", ix+1, 1, &dtype))) {
            SF_display("Error accessing shared memory data types\n");
            free(segments);
            free(thread_ids);
            return rc;
        }

        // load the parameters into structs that will be passed to threads
        segments[ix].key = (ST_int) key;
        segments[ix].dtype = (ST_int) dtype;
        segments[ix].varindex = (ST_int) ix+1;
    }

    /* start the threads - if any thread fails to start the program will exit and threads will be
       canceled */
    for (ix = 0; ix < num_threads; ix++) {
        thread_rc = pthread_create(&thread_ids[ix], NULL, &thread_mgr, &segments[ix]);
        if (thread_rc != 0) {
            SF_display("OS would not create new thread\n");
            free(segments);
            free(thread_ids);
            return (ST_retcode) THREAD_FAILURE;
        }
    }

    // join the threads stared above to capture their return status.
    thread_exit_codes = malloc(num_threads * sizeof(int));
    for (ix = 0; ix < num_threads; ix++) {
        thread_rc = pthread_join(thread_ids[ix], (void **) &thread_exit_codes[ix]);
        if (thread_rc != 0) {
            SF_display("Could not join thread\n");
            free(segments);
            free(thread_exit_codes);
            free(thread_ids);
            return (ST_retcode) THREAD_FAILURE;
        }
    }
    
    // test the return status of threads. If any thread returns an error terminate the program
    for (ix = 0; ix < num_threads; ix++) {
        if (thread_exit_codes[ix] != 0) {
            SF_display("Thread returned error\n");
            free(segments);
            free(thread_exit_codes);
            free(thread_ids);
            return (ST_retcode) rc;
        }
    }

    free(segments);
    free(thread_exit_codes);
    free(thread_ids);
    return 0;
}

/* function to handle arguments passed to threads. This takes a Struct Segment as an argument
   and calls the appropriate reading functions based on the passed data type */
static void *thread_mgr(void *thread_args) 
{
    Segment *segment_info;
    key_t key;
    DTYPE dtype;
    ST_int varindex;
    ST_retcode rc;

    segment_info = (Segment *) thread_args; // cast the void struct to the appropriate type
    key   = (key_t) segment_info->key;
    dtype = (DTYPE) segment_info->dtype;
    varindex = segment_info->varindex;
 
    // call the reader appropriate for the data type passed
    switch (dtype) {
        case LONG:
            rc = read_long_list(key, varindex);
            break;
        case DOUBLE:
            rc = read_double_list(key, varindex);
            break;
    }
    return (void *) rc;
}

// function to read a list of long integers from shared memory into Stata
static ST_retcode read_long_list(key_t key, ST_int varindex) 
{
    ST_retcode rc;
    int segment_id, numel, idx;
    size_t segment_size;
    long *shm, elt;

    // allocate the shared memory segment. Note that length of Stata vector is equal to length
    // of shared memory segment
    numel = SF_nobs();
    segment_size = numel * sizeof(long);
    segment_id = shmget(key, segment_size, S_IRUSR | S_IWUSR);
    if (segment_id == -1) {
        SF_display("Could not get segment\n");
        return (ST_retcode) GET_FAILURE; 
    }
    if ((shm = shmat(segment_id, 0, 0)) == NULL) {
        SF_display("Could not attach segment\n");
        return (ST_retcode) ATT_FAILURE;
    }

    for (idx = SF_in1(); idx <= SF_in2(); idx++) {
        elt = (ST_double) shm[idx-1];
        if ((rc = SF_vstore(varindex, idx, elt)) != 0) {
            return rc;
        }
    }
    shmdt(shm);
    return (ST_retcode) 0;
}

// function to read a list of floating point values from shared memory into Stata
static ST_retcode read_double_list(key_t key, ST_int varindex) {
    ST_retcode rc;
    int segment_id, numel, idx;
    size_t segment_size;
    double *shm, elt;

    numel = SF_nobs();
    segment_size = numel * sizeof(double);
    segment_id = shmget(key, segment_size, S_IRUSR | S_IWUSR);
    if (segment_id == -1) {
        SF_display("Could not get segment\n");
        return (ST_retcode) GET_FAILURE; 
    }
    if ((shm = shmat(segment_id,0,0)) == NULL) {
        SF_display("Could not attach segment\n");
        return (ST_retcode) ATT_FAILURE;
    }

    for (idx = SF_in1(); idx <= SF_in2(); idx++) {
        elt = (ST_double) shm[idx-1];
        if ((rc = SF_vstore(varindex, idx, elt)) != 0) {
            return rc;
        }
    }
    shmdt(shm);
    return (ST_retcode) 0;
}
