# stata-pydata: Shared memory communication between Python and Stata

# Description
Stata-PyData provides routines for efficiently passing numeric data between Python and Stata. It aims to use interfaces similar to traditional Stata and Python I/O tools to provide the speed of shared memory communication with the transparency of file I/O.

# Background
Stata-PyData was designed primarily for use in a project I worked on a Brown in which we found oursleves using Python/Pandas for low level data cleaning and manipulation and Stata for stastical analysis. We were impressed by the speed and flexibility of Python/Pandas for handling data manipulation and the easy of use of Stata for analysis but were frustrated by the painfully slow process of transferring data between the two using flat files on the disk. Stata-PyData aims to make that process slightly less painful. While the specific constraints (and lack thereof) of our project at Brown inform many design considerations I hope this code will be generally useful.

**Features**
Stata-PyData is deisgned to do one thing: pass real valued numeric data between Python and Stata quickly. We think it does this pretty well. Passing 15gb of floating point values (approx xx million observations on yy variables) takes zz seconds. Compare this to aa minutes to do the same thing using CSV files on the disk. For some demonstrations see /demo. Code is relatively simple and usage should be clear from comments and demo files.

**Technical Notes**
	# This code was developed for use on Linux OS. It *may* work on other Unix-like systems like OSX but this has not been tested. It certainly will not work on Windows.
	# 