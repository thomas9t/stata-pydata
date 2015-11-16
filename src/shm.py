import pandas as pd
import re, os, sys
sys.path.append('../build')
import _py_shm

""" 
    This module is a wrapper for _py_shm.c which takes a real numeric Python list and writes 
    it to System V shared memory. It is designed to be used with the Stata program "shm_use"
    which reads these lists. However, it can in theory be used for other purposes.

    The module defines the following functions:

    1) write_list():  Takes a python list, does final checking and processing and passes it to
                      _py_shm for writing
    2) write_frame(): A utility for passing a Pandas data frame to write_list(). It iterates
                      over columns in the data frame and handles inferring types and incrementing
                      key generator seeds 
    3) deallocate():  A utility which wraps the command line "ipcrm -m" command to remove a shared
                      memory segment 

    Examples:
    >>> integer_list = range(100000) 
    >>> integer_segment = shm.write_list(integer_list, 'int', 'integer_var', 1)
    >>> shm.deallocate(integer_segment[1])

    >>> data = pd.DataFrame(np.random.rand(100000,3), columns = ['var1','var2','var3'])
    >>> allocated_segments = shm.write_frame(data)
    >>> for segment in allocated_segments: shm.deallocate(allocated_segments[segment][1])

    Some important notes:

    1) This module requires the presence of System V shared memory.
    2) The user is expected to manage the seeds used to obtain keys for shared memory segments
       if a duplicate seed is passed the internal writer will fail
    3) Pandas allows inconsistent data types in columns of data frames. For performance reasons
       there is minimal checking on types and it is the users responsibility to ensure consistently
       typed data. Inconsistent types will cause errors in _py_shm or will cause undefined behavior
    4) Casting from Numpy datatypes (which underly Pandas data frames) to Python types is via the
       numpy ndarray.tolist() method. This is assumed to accurately cast types but this is not
       guaranteed. 
    5) Information about allocated segments needed by other programs (e.g. Stata) is written to a 
       tab delimited file which contains:
            segment_key -> segment_id -> data_type -> length -> variable_name 
"""

DTYPE_CODES = {'int' : 0, 'float' : 1, 'long' : 2}

def write_list(data, dtype, varname, key_seed, info_file='segment_info.txt'):
    """ 
        Write a list to a shared memory segment
        
        Arguments:
            data      -- the list to be written. Must be of a constant real numeric data type
            dtype     -- the data's type. String types are mapped to numeric codes in "DTYPE_CODES"
            varname   -- the 'name' of the list. Any arbitrary string.
            key_seed  -- an integer used in the "ftok()" function to obtain a key for shared memory
            info_file -- a path to a file that will contain information about the segment allocated
    """
    try: 
        dtype_key = DTYPE_CODES[dtype]
    except KeyError:
       raise TypeError("Unsupported data type passed")

    # Call the C extension that actually does the writing
    shm_key, segment_id = _py_shm.write(data, dtype_key, key_seed)
    
    numel = len(data)

    # write a text file containing information about the allocated segment
    with open(info_file, mode = 'ab') as fh:
        fh.write(
            str(shm_key)   + '\t' + str(segment_id) + '\t' + 
            str(dtype_key) + '\t' + str(numel)      + '\t' + 
            varname + '\n'
        )
    
    return (shm_key, segment_id)
    
def write_frame(frame, info_file='segment_info.txt', key_seed=1):
    """
        Write a Pandas data frame to shared memory. 
        Calls "write_list()" over each column of the data frame. See note 4 above about data
        type conversion

        Arguments:
            frame     -- the Pandas data frame to be written
            info_file -- a path to the file where information about the segment(s) associated with
                         the frame will be written
            key_seed  -- the "initial" seed that will be passed to "ftok()" subsequent seeds are
                         incremented by one.
    """
    varnames = frame.columns.tolist()
    
    allocated_segments = dict()
    for varname in varnames:
        data = frame.loc[:,varname].values.tolist()

        # infer data type from first element in the list (see note 3 above about data types)
        if isinstance(data[0], int):
            dtype = 'int'
        elif isinstance(data[0], float): 
            dtype = 'float'
        elif isinstance(data[0], long): 
            dtype = 'long'
        else: 
            # if an unsupported type is passed raise exception and clean up existing segments
            if len(allocated_segments) > 0:
                for segment in allocated_segments:
                    deallocate(allocated_segments[segment][1])
            raise TypeError('Column: ' + varname + ' is of an unsupported type')
        
        # call the underlying writer - if an error occurs clean up any existing segments
        try:
            segment_info = write_list(data, dtype, varname, key_seed, info_file)
        except Exception:
            if len(allocated_segments) > 0:
                for segment in allocated_segments:
                    deallocate(allocated_segments[segment][1])
            raise

        allocated_segments[varname] = segment_info
        key_seed += 1  # increment the seed used to obtain segment IDs in C
        
    return allocated_segments

# utility function to remove an allocated segment
def deallocate(segment_id):
    rc = os.system('ipcrm -m ' + str(segment_id))
    if rc != 0: raise OSError("Could not deallocate segment")
