=============================
========= README ============
=============================

=============================
0. Preliminaries
=============================

To use this code, one should first install packages "oce" and "optparse" in R.

=============================
1. About
=============================

* welch.R: R script for plotting the output of pwelch on a particular data set

* welch_dir.pl: Perl script to run welch.R on all files in a specified directory

=============================
2. Samples
=============================

$ Rscript welch.R --help

--- See usage and all optional arguments

$ Rscript welch.R -i ../../fft/ftq_mcp31_30.dat -o ../../fft/ftq_mcp31_30.pdf --xmin 10 --xmax 30 --ymin 0.01 --ymax 1000

--- Collect data from specified dat file and plot to specified pdf file. Note that output file must be a pdf file for now.

$ ./welch_dir.pl

--- See usage information

$ ./welch_dir.pl ../../fft/ 45

--- Plot pwelch plots for every data set in specified directory with maximum frequency value of 45 in each plot



