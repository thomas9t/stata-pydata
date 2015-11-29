# Stata-PyData: Shared memory communication between Python and Stata

# Description
Stata-PyData provides routines for efficiently passing numeric data between Python and Stata. It aims to use interfaces similar to traditional Stata and Python I/O tools to provide the speed of shared memory communication with the transparency of file I/O.

# Background
Stata-PyData was designed primarily for use in a project I worked on a Brown in which we found ourselves using Python (in particular the Pandas library built on Numpy) for low level data cleaning and manipulation, and Stata for stastical analysis. We were impressed by the speed and flexibility of Python for handling data manipulation and the ease-of-use of Stata for analysis but were frustrated by the painfully slow process of transferring data between the two using files on the disk. Stata-PyData aims to make that process slightly less painful. While the specific constraints (and lack thereof) of our project at Brown inform many design considerations I hope this code will be generally useful.

# Function and command syntax:

**Python** - Defined in shm.py

    shm.write_list(data, dtype, key_seed, info_file='segment_info.txt')

This function writes the list defined in `data` to a shared memory segment. `data` must be a list else an exception will be thrown from C. `dtype` is a string equal to `int`, `float` or `long` which described the data type of `data`. Important note: lists are expected to be of consistant type. Inconsistently typed lists will result in errors or undefined behavior. `key_seed` is an integer used in a call to `ftok('/tmp', key_seed)` to obtain a key for the shared memory segment. `info_file` is a text file containing information about the shared memory segment needed by other programs to attach and read the segment.

    shm.write_frame(frame, info_file='segment_info.txt', key_seed=1)

This is a utility function which calls `shm.write_list` repeatedly over the columns of a Pandas data frame. Data types are inferred from the first element of each column in the data frame. The value of `key_seed` is incremented by one each time a new column is written to shared memory.

**Stata** - Defined in shm_use.ado

    shm_use using filename [, clear deallocate compress]

`shm_use` parses the information contained in `filename` and reads the corresponding data from shared memory into the Stata data area. The underlying C program is multithreaded using pthreads. A new thread is created for each segment listed in `filename`.

    options              description
    -----------------------------------------------------------------------------------
    clear                replace data currently in memory
    deallocate           deallocate the shared memory segments after import
    compress             compress data in memory to the lowest possible storage type


# Examples of use:

Create data in Python and write it to shared memory using shm.write_frame()

    import pandas as pd
    import numpy as np
    import shm

    data = pd.DataFrame(np.random.rand(10000000,3), columns = ['var1', 'var2', 'var3'])
    print data.describe()
    data.loc[1:10,:]
    allocated_segments = shm.write_frame(data, info_file = 'random_integers.txt')

Read the data into Stata

	shm_use using random_integers.txt, clear compress deallocate
	summarize
	list in 1/10
