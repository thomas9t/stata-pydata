#include <Python.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <errno.h>

#define INT_CONVERT_FAILURE    -999  
#define GET_FAILURE            -998 
#define ATT_FAILURE            -997 
#define GET_ITEM_FAILURE       -996 
#define FLOAT_CONVERT_FAILURE  -995 
#define PYLONG_CONVERT_FAILURE -994 

typedef enum DTYPE_CODE {INTEGER, DOUBLE, PYLONG} DTYPE;

/* access functions - these get an element of a Python list and
   convert it to a C type. If an error is encountered they set "err_flag" to 1
   and return a numeric code indicating the source of the error */
static long   get_long_elt(PyObject   *list, int idx, int *err_flag);
static double get_double_elt(PyObject *list, int idx, int *err_flag);
static double get_PyLong_elt(PyObject *list, int idx, int *err_flag);

/* writers - these take a Python list and write it to shared memory
   upon success they return the segment_id to which data was written
   upon error the return a negative integer indicating the source of 
   the error */
static int write_integer_list(PyObject *list, key_t key);
static int write_double_list(PyObject  *list, key_t key);
static int write_PyLong_list(PyObject  *list, key_t key);

// utility functions
static int len(PyObject *list);

// main function: calls writers, handles exceptions
static PyObject *_py_shm(PyObject *self, PyObject *args)
{
    PyObject *datalist, *outlist;
    key_t key;
    int segment_id, exit_status;
    long dtype, key_seed;

    /* interpret arguments passed from Python. Explanation:
           [0]: O!: Pointer to a Python object (a list) to be written to shared memory
           [1]: l:  Python integer -> C long with the data type of 0
           [2]: l:  Python integer -> C long with the byte used to seed ftok */
    
    if (!PyArg_ParseTuple(args, "O!ll", &PyList_Type, &datalist, &dtype, &key_seed))
        return NULL;

    // obtain a key to generate a shared memory segment
    if ((key = ftok("/tmp", (int) key_seed)) == (key_t) -1)
        return PyErr_SetFromErrno(PyExc_OSError);

    segment_id = 0;
    
    /* call the appropriate writer for the passed data. Unsupported datatypes should have been
       caught in Python. */
    switch ((int) dtype) {
        case INTEGER:
            exit_status = write_integer_list(datalist, key);
            break;
        case DOUBLE:
            exit_status = write_double_list(datalist, key);
            break;
        case PYLONG:
            exit_status = write_PyLong_list(datalist, key);
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "Unsupported datatype passed");
            return NULL;
    }

    /* handle the exit codes from writer functions. The exit status is either a 
       code indicating which function call failed or the segment ID of allocated memory */
    switch (exit_status) {
        case INT_CONVERT_FAILURE:
            PyErr_SetString(PyExc_TypeError, "Could not cast to integer");
            return NULL;
        case FLOAT_CONVERT_FAILURE:
            PyErr_SetString(PyExc_TypeError, "Could not cast to double from PyFloat");
            return NULL;
        case PYLONG_CONVERT_FAILURE:
            PyErr_SetString(PyExc_TypeError, "Could not cast to double from PyLong");
            return NULL;
        case GET_FAILURE:
            PyErr_Format(PyExc_OSError, 
                "Could not create segment. OS Returned Error %d: %s", errno, strerror(errno));
            return NULL;
        case ATT_FAILURE:
            PyErr_Format(PyExc_OSError,
                "Could not attach segment. OS Returned Error %d: %s", errno, strerror(errno));
            return NULL;
        case GET_ITEM_FAILURE:
            PyErr_SetString(PyExc_StandardError, "Error extracting item");
            return NULL;
    }
    if (exit_status < 0) {
        PyErr_SetString(PyExc_StandardError, "Undefined error occurred");
        return NULL;
    }
    segment_id = exit_status; // functions return segment IDs upon success
    
    /* build the return value of the program. The function will return a list
       containing:
           [0] the key associated with the written segment and 
           [2] the segment ID associated with the written segment  */

    outlist = PyList_New(2);
    if (PyList_SetItem(outlist, 0, PyInt_FromLong((long) key))) {
        PyErr_SetString(PyExc_StandardError, "Error setting item");
        Py_DECREF(outlist);
        return NULL;
    }
    if (PyList_SetItem(outlist, 1, PyInt_FromLong((long) segment_id))) {
        PyErr_SetString(PyExc_StandardError, "Error setting item");
        Py_DECREF(outlist);
        return NULL;
    }
    return Py_BuildValue("O", outlist);
}

// initialization routines needed by Python
static PyMethodDef _shm_methods[] = {
    {"write", _py_shm, METH_VARARGS, "Write a list to shared memory"},
    {NULL,NULL,0,NULL}
};

PyMODINIT_FUNC init_py_shm(void)
{
    (void) Py_InitModule("_py_shm", _shm_methods);
}

// function to compute the length of a Python object as a C integer
static int len(PyObject *list)
{
    Py_ssize_t size;
    PyObject *size_asPyInt;
    int length;

    size = PyList_Size(list);
    size_asPyInt = PyInt_FromSsize_t(size);

    length = (int) PyInt_AsLong(size_asPyInt);
    if (length == -1) {
        Py_DECREF(size_asPyInt);
        return INT_CONVERT_FAILURE;
    }
    Py_DECREF(size_asPyInt);
    return length;
}

// function to extract an element of a Python integer list and return a C long
static long get_long_elt(PyObject *list, int idx, int *err_flag)
{
    PyObject *tmp;
    long elt;

    if ((tmp = PyList_GetItem(list, idx)) == NULL) {
        *err_flag = 1;
        return GET_ITEM_FAILURE;
    }
    if (((elt = PyInt_AsLong(tmp)) == -1L) && 
         (PyErr_Occurred() != NULL)) {
        *err_flag = 1;
        return INT_CONVERT_FAILURE;
    }
    return elt;
}

// function to extract an element of a Python float list and return a C double
static double get_double_elt(PyObject *list, int idx, int *err_flag)
{
    PyObject *tmp;
    double elt;

    if ((tmp = PyList_GetItem(list,idx)) == NULL) {
        *err_flag = 1;
        return GET_ITEM_FAILURE;
    }
    if (((elt = PyFloat_AsDouble(tmp)) == -1.0) && 
        (PyErr_Occurred() != NULL)) {
        *err_flag = 1;
        return FLOAT_CONVERT_FAILURE;
    }
    return elt;
}

// function to extract an element of a Python Long Integer list and return a C double
static double get_PyLong_elt(PyObject *list, int idx, int *err_flag)
{
    PyObject *tmp;
    double elt;

    if ((tmp = PyList_GetItem(list, idx)) == NULL) {
        *err_flag = 1;
        return GET_ITEM_FAILURE;
    }
    if (((elt = PyLong_AsDouble(tmp)) == -1.0) && 
         (PyErr_Occurred() != NULL)) {
        *err_flag = 1;
        return PYLONG_CONVERT_FAILURE;
    }
    return elt;
}

// function to write a Python integer list to shared memory
static int write_integer_list(PyObject *list, key_t key)
{
    int numel, segment_id, segment_size, idx, error_flag;
    long *shm, elt;

    // allocate the shared memory segment
    if ((numel = len(list)) == INT_CONVERT_FAILURE)
        return INT_CONVERT_FAILURE;
    segment_size = numel * sizeof(long);
    segment_id = shmget(key, segment_size, IPC_CREAT | IPC_EXCL | S_IWUSR | S_IRUSR);
    if (segment_id == -1)
        return GET_FAILURE;
    if ((shm = shmat(segment_id, 0, 0)) == NULL) {
        shmctl(segment_id, IPC_RMID, 0);
        return ATT_FAILURE;
    }

    // write the list to the allocated segment
    numel -= 1;
    error_flag = 0;
    for (idx = 0; idx <= numel; idx++) {
        elt = get_long_elt(list, idx, &error_flag);
        if (error_flag == 1) {
            shmdt(shm);
            shmctl(segment_id, IPC_RMID, 0);
            return elt;
        }
        shm[idx] = elt; 
    }

    // detach (but not deallocate) the segment
    shmdt(shm);
    return segment_id;
}

// function to write a Python float list to shared memory
static int write_double_list(PyObject *list, key_t key)
{
    int numel, segment_id, segment_size, idx, error_flag;
    double *shm, elt;

    // allocate the shared memory segment
    if ((numel = len(list)) == INT_CONVERT_FAILURE)
        return INT_CONVERT_FAILURE;
    segment_size = numel * sizeof(double);
    segment_id = shmget(key, segment_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (segment_id == -1)
        return GET_FAILURE;
    if ((shm = shmat(segment_id, 0, 0)) == NULL){
        shmctl(segment_id, IPC_RMID, 0);
        return ATT_FAILURE;
    }

    // write the list to the allocated segment
    numel -= 1;
    error_flag = 0;
    for (idx = 0; idx <= numel; idx++) {
        elt = get_double_elt(list, idx, &error_flag);
        if (error_flag == 1) {
            shmdt(shm);
            shmctl(segment_id, IPC_RMID, 0);
            return elt;
        }
        shm[idx] = elt;
    }

    // detach (but do not deallocate) the segment
    shmdt(shm);
    return segment_id; 
}

// function to write a Python Long Integer list shared memory
static int write_PyLong_list(PyObject *list, key_t key)
{
    int numel, segment_id, segment_size, idx, error_flag;
    double *shm, elt;

    // allocate the segment as type double
    if ((numel = len(list)) == INT_CONVERT_FAILURE)
        return INT_CONVERT_FAILURE;
    segment_size = numel * sizeof(double);
    segment_id = shmget(key, segment_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    if (segment_id == -1)
        return GET_FAILURE;
    if ((shm = shmat(segment_id, 0, 0)) == NULL) {
        shmctl(segment_id, IPC_RMID, 0);
        return ATT_FAILURE;
    }

    // write the list to the allocated segment
    numel -= 1;
    error_flag = 0;
    for (idx = 0; idx <= numel; idx++) {
        elt = get_PyLong_elt(list, idx, &error_flag);
        if (error_flag == 1) {
            shmdt(shm);
            shmctl(segment_id, IPC_RMID, 0);
            return elt;
        }
        shm[idx] = elt;
    }

    // detach (but do not deallocate) the segment
    shmdt(shm);
    return segment_id; 
}