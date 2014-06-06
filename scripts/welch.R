# library that includes the pwelch function
suppressPackageStartupMessages(library(oce))
# library for command line option parsing
suppressPackageStartupMessages(library(optparse))

######################################
### Functions
######################################

# extract useful information from the raw data file
extract_data <- function(filename) {
	mydata = read.table(filename, comment.char="#")
	work_amt = mydata$V2

	# calculate time steps and mean time step (all in ns)
	times = as.numeric(as.character(mydata$V1))
	N_entries = length(times)
	time_steps_ns = times[2:N_entries] - times[1:(N_entries-1)]
	avg_time_step_ns = mean(time_steps_ns)

	return(list(work_amt=work_amt, time_steps_ns=time_steps_ns,
		N_entries=N_entries, avg_time_step_ns=avg_time_step_ns))
}

# get it? welch's? har har
# inputs:
# ---- * work_amt: work amount at each time step
# ---- * avg_time_diff_ns: we assume there is the same time step length (in ns) between each step
make_jelly <- function(work_amt, avg_time_diff_ns) {
	
	# the magic
	w <- pwelch(work_amt,fs=1.0/(avg_time_diff_ns*10**(-9)),plot=FALSE)

	return(w)
}

# plot pwelch output
plot_jelly <- function(wout, out_filename,
                       min_freq, max_freq,
                       min_spec, max_spec) {
  ### find boundaries of plot
  max_possible_freq = max(wout$freq)
  if(missing(min_freq)) {
    min_freq = 0
  }
  if(missing(max_freq) | (max_freq <= min_freq) | (max_freq >= max_possible_freq)) {
    max_freq = max_possible_freq
  }
  if(missing(min_spec) | min_spec <= 10^(-10)) {
    # get rid of extreme outliers by taking quantiles
    # but divide to leave a little wiggle room in the plot
    min_spec = as.numeric(quantile(wout$spec,0.001)) / 10
  }
  if(missing(max_spec) | max_spec <= min_spec) {
    max_spec = as.numeric(quantile(wout$spec,0.999)) * 10
  }
	
	# plot to pdf
	pdf(out_filename)
	# note logarithmic spectrum (vertical) scaling
	plot(c(min_freq,max_freq), c(min_spec,max_spec),
       log="y", type="n",
       xlab="frequency", ylab="spectrum")
	lines(wout$freq, wout$spec)	
	invisible(dev.off())
}

######################################
### Main
######################################

### collect command line arguments
# establish optional arguments
# "-h" and "--help" are automatically in the list
option_list <- list(
  make_option(c("-i", "--input"), type="character",
    default="welch_input.dat",
    help="Input data file"),
  make_option(c("-o", "--output"), type="character",
    default="welch_plot.pdf",
    help="Output file for plotting"),
  make_option("--xmin", type="double", default=0,
    help=paste("Minimum frequency (horizontal axis) ",
      "in output plot [default %default]",sep="")),
  make_option("--xmax", type="double", default=40,
    help=paste("Maximum frequency (horizontal axis) ",
      "in output plot [default %default]",sep="")),
  make_option("--ymin", type="double", default=-1,
    help=paste("Minimum spectrum (vertical axis) ",
      "in output plot [default adaptive]",sep="")),
  make_option("--ymax", type="double", default=-1,
    help=paste("Maximum spectrum (vertical axis) ",
      "in output plot [default adaptive]",sep=""))
)

# read command line
opt <- parse_args(OptionParser(option_list=option_list))
  
#max_freq = as.numeric(as.character(args[3]))

### read in data
mydata = extract_data(opt$input)

### make welch plot
wout = make_jelly(mydata$work_amt, mydata$avg_time_step_ns)
plot_jelly(wout, opt$output, opt$xmin, opt$xmax,
           opt$ymin, opt$ymax)

