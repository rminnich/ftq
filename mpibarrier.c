/**
 * ftq.c : Fixed Time Quantum microbenchmark
 *   As modified for MPI.
 *
 * Written by Matthew Sottile (mjsottile@gmail.com)
 *
 * This is a complete rewrite of the original tscbase code by
 * Ron and Matt, in order to use a better set of portable timers,
 * and more flexible argument handling and parameterization.
 *
 * 4/15/2014 : Major rewrite; remove pthreads ifdef. Separate out OS bits.
 *             use --include to pull in whatever OS defines you are using.
 *             New file format with time (in ns) and count in one place;
 *             comments at front are self describing, including sample
 *             frequency. Spits out octave commands to run as well.
 *             Puts cpu info in there as well as uname info.
 *             This makes the files more useful as time goes by.
 * 12/30/2007 : Updated to add pthreads support for FTQ execution on
 *              shared memory multiprocessors (multicore and SMPs).
 *              Also, removed unused TAU support.
 *
 * Licensed under the terms of the GNU Public Licence.  See LICENCE_GPL
 * for details.
 */
#include "mpiftq.h"
#include <sys/mman.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <time.h>
#include <unistd.h>

int ignore_wire_failures = 0;

static int set_realtime = 0;
static int pin_threads = 1;
static int rt_free_cores = 2;
static volatile int hounds = 0;
/* samples: each sample has a timestamp and a work count. */
static struct sample *samples;
static unsigned long long *data;
static size_t numsamples = DEFAULT_COUNT;
static double ticksperns;
static unsigned long long interval = DEFAULT_INTERVAL;
static unsigned long delay_msec;
static int numthreads = 1;
static unsigned long total_count;
static unsigned int max_work;

int k, count;

/* return current time in ns as a 'tick' */
ticks nsec_ticks()
{
	struct timespec t;
	int ret;
	ticks val;
#define TICKCLOCK CLOCK_MONOTONIC_RAW
	ret = clock_gettime(TICKCLOCK, &t);
	if (ret) {
		fprintf(stderr, "%s: clock_gettime failed\n", __func__);
		exit(1);
	}
	val = t.tv_sec * 1000 * 1000 * 1000 + t.tv_nsec;
	return val;
}
double compute_ticksperns(void)
{
	ticks tickstart, tickend;
	unsigned long long timestart, timeend, ns;
	double convert;
	ticks el;

	timestart = nsec_ticks();
	tickstart = getticks();
	sleep(2);
	tickend = getticks();
	timeend = nsec_ticks();
	ns = timeend - timestart;
	el = tickend - tickstart;
	convert = (1.0 * el) / ns;
	return convert;
}

/* do the best you can. */
void osinfo(FILE * f, int core)
{
	int readingcore = -1;
	// no uname on rocky 8. ?

	char buf[8192];

	FILE *cpu = fopen("/proc/cpuinfo", "r");
	if (!cpu)
		return;
	/* note: \n comes for free. */
	/* skip to our core. */
	while (fgets(buf, sizeof(buf), cpu)) {
		if (!strncmp("processor", buf, strlen("processor"))) {
			readingcore++;
		}
		if (readingcore > core)
			break;
		if (core != readingcore)
			continue;
		fprintf(f, "# %s", buf);
	}
}

void usage(char *av0)
{
	fprintf(stderr,
		"usage: %s [-t threads] [-n samples] [-f frequency] [-h] [-o outname] [-s] [-r] [-d delay_msec] "
		"[-T ticks-per-ns-float] "
		"[-w (ignore wire failures -- only do this if there is no option]"
		"\n",
		av0);
	fprintf(stderr, "defaults: %s -t %d -n %d -f %lld -o \"%s\" -d %ld\n", av0, numthreads, (int)numsamples, interval, DEFAULT_OUTNAME, delay_msec);
	exit(1);
}

void header(FILE * f, int thread)
{
	fprintf(f, "# Frequency %f\n", 1e9 / interval);
	fprintf(f, "# Ticks per ns: %g\n", ticksperns);
	fprintf(f, "# octave: pkg load signal\n");
	fprintf(f, "# x = load(<file name>)\n");
	fprintf(f, "# pwelch(x(:,2),[],[],[],%f)\n", 1e9 / interval);
	fprintf(f, "# start delay %lu msec\n", delay_msec);
	fprintf(f, "# Total count is %lu\n", total_count);
	fprintf(f, "# Max possible work is %u\n", max_work);
	fprintf(f, "# Fraction is %g\n", (1.0 * total_count) / max_work);
	if (ignore_wire_failures)
		fprintf(f, "# Warning: not wired to this core; results may be flaky\n");
	osinfo(f, thread);
}

static void ftq_mdelay(unsigned long msec)
{
	ticks start, end, now;

	start = getticks();
	end = start + (ticks)(msec * ticksperns * 1000000);

	do {
		now = getticks();
	} while (now < end || (now > start && end < start));
}

#define V(...)
// The MPI version has one big difference from the standard FTQ.
// Computation ends, and starts, on direction from rank 0.
// Rank 0 starts the nodes, waits for interval end, stops the nodes,
// gathers the result. Lots of slop in this.
int main(int argc, char **argv)
{
	/* local variables */
	static char fname[8192], outname[255];
	int i, j;
	int use_threads = 0;
	FILE *fp;
	int use_stdout = 0;
	int rc;
	pthread_t *threads;
	ticks base;
	size_t samples_size, data_size;

	MPI_Init(&argc,&argv);
	/*
	 * getopt_long to parse command line options.
	 * basically the code from the getopt man page.
	 */
	while (1) {
		int c;
		c = getopt(argc, argv, "n:hsf:o:t:T:wrd:");
		if (c == -1)
			break;

		switch (c) {
		case 't':
			numthreads = atoi(optarg);
			use_threads = 1;
			break;
		case 's':
			use_stdout = 1;
			break;
		case 'o':
			sprintf(outname, "%s", optarg);
			break;
		case 'f':
			/* the interval units are ns. */
			interval = (unsigned long long)
				(1e9 / atoi(optarg));
			break;
		case 'n':
			numsamples = atoi(optarg);
			break;
		case 'w':
			ignore_wire_failures++;
			break;
		case 'T':
			sscanf(optarg, "%lg", &ticksperns);
			break;
		case 'r':
			set_realtime = 1;
			break;
		case 'd':
			delay_msec = strtoul(optarg, NULL, 0);
			if ((long)delay_msec < 0) {
				fprintf(stderr, "delay_msec must not be negative\n");
				exit(-1);
			}
			break;
		case 'h':
		default:
			usage(argv[0]);
			break;
		}
	}
	/* sanity check */
	if (numsamples > MAX_SAMPLES) {
		fprintf(stderr, "WARNING: sample count exceeds maximum.\n");
		fprintf(stderr, "         setting count to maximum.\n");
		numsamples = MAX_SAMPLES;
	}
	if (ticksperns == 0.0)
		ticksperns = compute_ticksperns();

	MPI_Comm comm = MPI_COMM_WORLD;
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int size;
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	/* allocate sample storage */
	/* the first part is the structs, the second part the data arrays */
	samples_size = sizeof(*samples) * numsamples;
	samples = mmap(0, samples_size, PROT_READ | PROT_WRITE,
	               MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE,
	               -1, 0);
	if (samples == MAP_FAILED) {
		printf("can not mmap %d bytes", samples_size);
		perror("die");
		exit(1);
	}
	if (mlock(samples, samples_size) < 0)
		perror("Failed to mlock");

	data_size = size * numsamples * sizeof(*data);
	data = mmap(0, data_size, PROT_READ | PROT_WRITE,
		    MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE,
		    -1, 0);
	if (mlock(data, data_size) < 0)
		perror("Failed to mlock");
	/* set up the data arrays. */
	for(int i = 0; i < numsamples; i++) {
		samples[i].count = &data[i*size];
	}
	// Run for 10M ns.
	unsigned long long tickinterval = (unsigned long long)(ticksperns * interval);

	int done = 0;
	V("RANK %d BCAST...", rank);
	MPI_Bcast(&tickinterval, 1, MPI_INT, 0, comm);
	V("%d DONE\n", rank);
	ticks tickend;
	tickend = base = getticks();
	// Rank 0 does a full barrier, and times it, for a baseline.
	for (int sample = 0; sample < numsamples; sample++) {
		count = 0;
		ticks ticknow, ticklast;
		ticknow = ticklast = getticks();
		tickend += tickinterval;
		V("Rank %d: ticknow %lld ticklast %lld tickinterval %lld tickend %lld\n", rank, ticknow, ticklast, tickinterval, tickend);
		MPI_Barrier(comm);
		samples[sample].ticklast = ticklast;
		samples[sample].count[0] = getticks() - ticklast;
		if (rank == 0)
		for (;ticknow < tickend; ticknow = getticks()) {
			const struct timespec t;
			if (nanosleep(&t, NULL) < 0) {
				perror("nanosleep");
			}
		}
	}

	if (rank == 0) {
		fprintf(stderr, "Ticks per ns: %f\n", ticksperns);
		fprintf(stderr, "Sample frequency is %f\n", 1e9 / interval);
		fprintf(stderr, "Total count is %lu\n", total_count);
		header(stdout, 0);
		base = samples[0].ticklast;
		for (i = 0; i < numsamples; i++) {
			fprintf(stdout, "%lld,", samples[i].ticklast - base);
			fprintf(stdout, "%lld\n",samples[i].count[0]);
		}
	}
	MPI_Finalize();
}
