# stata-pydata: Shared memory communication between Python and Stata

# Description
Stata-PyData provides routines for efficiently passing numeric data between Python and Stata. It aims to use interfaces similar to traditional Stata and Python I/O tools to provide the speed of shared memory communication with the transparency of file I/O.

# Background
<<<<<<< HEAD
Stata-PyData was designed primarily for use in a project I worked on a Brown in which we found oursleves using Python/Pandas for low level data cleaning and manipulation and Stata for stastical analysis. We were impressed by the speed and flexibility of Python/Pandas for handling data manipulation and the easy of use of Stata for analysis but were frustrated by the painfully slow process of transferring data between the two using flat files on the disk. Stata-PyData aims to make that process slightly less painful. While the specific constraints (and lack thereof) of our project at Brown inform many design considerations I hope this code will be generally useful.

**Features**
Stata-PyData is deisgned to do one thing: pass real valued numeric data between Python and Stata quickly. We think it does this pretty well. Passing 15gb of floating point values (approx xx million observations on yy variables) takes zz seconds. Compare this to aa minutes to do the same thing using CSV files on the disk. For some demonstrations see /demo. Code is relatively simple and usage should be clear from comments and demo files.

**Technical Notes**
	# This code was developed for use on Linux OS. It *may* work on other Unix-like systems like OSX but this has not been tested. It certainly will not work on Windows.
	# 
=======
Stata-PyData was designed primarily for use in a project I worked on a Brown in which we found ourselves using Python/Pandas for low level data cleaning and manipulation and Stata for stastical analysis. We were impressed by the speed and flexibility of Python/Pandas for handling data manipulation and the easy of use of Stata for analysis but were frustrated by the painfully slow process of transferring data between the two using flat files on the disk. Stata-PyData aims to make that process slightly less painful. While the specific constraints (and lack thereof) of our project at Brown inform many design considerations I hope this code will be generally useful.

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

# Function and command syntax:

**Python** - Defined in shm.py

    shm.write_list(data, dtype, key_seed, info_file='segment_info.txt')

This function writes the list defined in `data` to a shared memory segment. `data` must be a list else an exception will be thrown from C. `dtype` is a string equal to `int`, `float` or `long` which described the data type of `data`. Important note: lists are expected to be of constant type. Inconsistently typed lists will result in errors or inconsistent behavior. `key_seed` is an integer used in a call to `ftok('/tmp', key_seed)` to obtain a key for the shared memory segment. `info_file` is a text file containing information about the shared memory segment needed by other programs to attach and read the segment.

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

>>>>>>> origin/master
