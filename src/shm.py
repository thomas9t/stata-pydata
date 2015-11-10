import pandas as pd
import re, os, sys
sys.path.append('../build')
import _py_shm

# this module is a wrapper for _shm.c where most of the
# action happens. It is the users responsibility to ensure that a new
# unique seed for the key is passed to on each call to this function. The function
# will abort if an already used key is generated.

# IMPORTANT NOTE - It is the user's responsibility to make sure data is of a constant type
# data types are inferred based on the first element in the list passed from a dataframe.
# Pandas supports inconsistent types in columns of data frames. This will cause errors.

# The shm_info argument can be used to specify a filename which contains information
# other programs need to understand the shared memory segment. It is tab delimited
# and formatted according to:
# segment_key -> dtype -> length -> variable name 

DTYPE_CODES = {'int' : 0, 'float' : 1, 'long' : 2}

def write_list(data, dtype, varname, key_seed, info_file = None):
    try: 
        dtype_key = DTYPE_CODES[dtype]
    except KeyError:
       print "Invalid argument passed for data type"
       raise

    # Call the C extension that actually does the writing
    shm_key, segment_id = _py_shm.write(data, dtype_key, key_seed)

    if info_file is None: fname = 'segment_info.txt'
    else: fname = info_file
    
    numel = len(data)

    # write a text file containing information about the allocated segment
    with open(fname, mode = 'ab') as fh:
        fh.write(
            str(shm_key)   + '\t' + str(segment_id) + '\t' + 
            str(dtype_key) + '\t' + str(numel)      + '\t' + 
            varname + '\n'
        )
    
    return (shm_key, segment_id)
    
def write_frame(frame, info_file = None, key_seed = 1):
    varnames = frame.columns.tolist()
    
    allocated_segments = dict()
    for varname in varnames:
        data = frame.loc[:,varname].values.tolist()
        if isinstance(data[0], int):
            dtype = 'int'
        elif isinstance(data[0], float): 
            dtype = 'float'
        elif isinstance(data[0], long): 
            dtype = 'long'
        else: 
            raise TypeError('Column: ' + varname + ' is of an unsupported type')
        
        segment_info = write_list(data, dtype, varname, key_seed, info_file)
        allocated_segments[varname] = segment_info
        key_seed += 1 # increment the seed used to obtain segment IDs in C
        
    return allocated_segments

# utility function to remove an allocated segment
def deallocate_segment(segment_id):
    os.system('ipcrm -m ' + str(segment_id))
        
