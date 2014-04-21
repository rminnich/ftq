# library that includes the pwelch function
library(oce)

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
plot_jelly <- function(wout, max_freq, out_filename) {
	# get rid of extreme outliers by taking quantiles
	min_y = quantile(wout$spec,0.001)
	max_y = quantile(wout$spec,0.999)
	# but leave a little wiggle room in the plot
	max_y = max_y * 10
	min_y = min_y / 10

	# don't overstep on the max frequency
	max_freq = min(max_freq, max(wout$freq))
	
	# plot to pdf
	pdf(out_filename)
	# note logarithmic spectrum (vertical) scaling
	plot(c(0,max_freq), c(min_y,max_y), log="y", type="n",
		xlab="frequency", ylab="spectrum")
	lines(wout$freq, wout$spec)	
	dev.off()
}

######################################
### Main
######################################

# collect command line arguments
args = commandArgs(trailingOnly = TRUE)
if(length(args) < 3) {
	stop("Usage: Rscript welch.R infile_name.dat outfile_name.pdf max_frequency\n")
}

filename_in = as.character(args[1])
filename_out = as.character(args[2])
max_freq = as.numeric(as.character(args[3]))

# read in data
mydata = extract_data(filename_in)

# make welch plot
wout = make_jelly(mydata$work_amt, mydata$avg_time_step_ns)
plot_jelly(wout, max_freq, filename_out)

