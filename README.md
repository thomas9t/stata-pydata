# stata-pydata: Shared memory communication between Python and Stata

# Description
Stata-PyData provides routines for efficiently passing numeric data between Python and Stata. It aims to use interfaces similar to traditional Stata and Python I/O tools to provide the speed of shared memory communication with the transparency of file I/O.

# Background
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
