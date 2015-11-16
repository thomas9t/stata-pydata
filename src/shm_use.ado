version 14.1

/*
    This program provides a Stata interface to shared memory communication. This program is designed
    to be used with Python (see: _py_shm.c and shm.py) but the originator of the segments does not
    matter. This program parses a text file containing information about allocated shared memory
    segments to be read into Stata. The segment information is parsed into Stata matrices which are
    then read by the plugin _st_shm.c to attach and read the shared memory segment. The program
    currently supports reading the following data types:
        [1]: C Long Integer
        [2]: C double

    Important Notes:
        [1]: Allocated segments must be of constant length! Stata contains a single mutable
             rectanuglar data area and so requires that all data be equal length "vectors"
        [2]: This program requires the "pthreads" library and is multithreaded. By default
             the program will allocate one thread per variable being read from shared memory.
             The user can specify a lower number of threads using the "num_threads()" argument.

*/

program shm_use
    syntax using/, [clear deallocate compress]

    quietly insheet using `using', tab `clear' nonames

    mata {
        keys        = st_data(.,1)  // the keys associated with each segment
        segment_ids = st_data(.,2)  // the ID associated with each segment
        dtypes      = st_data(.,3)  // the data type associated with each segment
        numel       = st_data(.,4)  // the number of elements in each segment 
        varnames    = st_sdata(.,5) // the variable name associated with the data in each segment
        
        // set up the Stata data area and check that all segments are of the same size
        stata("clear")
        if (any(numel :!= numel[1])) {
            printf("Error: segments are of variable size\n")
            stata("exit 999")
        }
        st_addobs(numel[1])

        // allocate memory for each variable (create a blank matrix to store data)
        nsegments = length(keys)
        for (s=1; s<=nsegments; s++) {
            if (dtypes[s] == 0) data_type = "long"
            if (dtypes[s] == 1) data_type = "double"
            varname = varnames[s]
            rc = st_addvar(data_type, varnames[s])
        }

        // store information about each segment in a Stata matrix to be read by _st_shm.c
        st_matrix("_shm_dtypes", dtypes) 
        st_matrix("_shm_keys", keys)

        // construct the call to the plugin and invoke the plugin
        varlist = invtokens(varnames', " ")
        call = "plugin call shm_internals " + varlist
        stata(call)
    }

    // optionally compress the data in memory to its lowest type
    if "`compress'" != "" compress

    // optionally deallocate the shared memory segments using the ipcrm Linux command
    if "`deallocate'" != "" {
        mata: st_local("segments", invtokens(strofreal(segment_ids, "%9.0f")'))
        mata: st_local("nsegments", strofreal(nsegments))

        forval s = 1/`nsegments' {
            quietly shell ipcrm -m `: word `s' of `segments''
        }
    }
end

capture program drop shm_internals
program shm_internals, plugin using(../build/_st_shm.plugin)
