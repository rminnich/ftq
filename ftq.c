// SPDX-License-Identifier: GPL-2.0-only
/**
 * ftq.c : Fixed Time Quantum microbenchmark
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
 * Licensed under the terms of the GNU Public License.  See LICENSE
 * for details.
 */
#include "ftq.h"
#include <sys/mman.h>
#include <sys/param.h>
#include <getopt.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

int ignore_wire_failures = 0;

static int set_realtime = 0;
static int pin_threads = 1;
static int rt_free_cores = 2;
static volatile int hounds = 0;
/* samples: each sample has a timestamp and a work count. */
static struct sample *samples;
static size_t numsamples = DEFAULT_COUNT;
static double ticksperns;
static unsigned long long interval = DEFAULT_INTERVAL;
static unsigned long delay_msec;
static int numthreads = 1;
static unsigned long long total_count;
static unsigned long long max_work;

void usage(char *av0)
{
	fprintf(stderr,
			"usage: %s [-t threads] [-n samples] [-f frequency] [-h] [-o outname] [-s] [-r] [-d delay_msec] "
			"[-T ticks-per-ns-float] "
			"[-w (ignore wire failures -- only do this if there is no option]"
			"\n",
			av0);
	fprintf(stderr, "defaults: %s -t %d -n %d -f %lld -o \"%s\" -d %ld\n", av0, numthreads, (int)numsamples, interval, DEFAULT_OUTNAME, delay_msec);
	exit(EXIT_FAILURE);
}

void header(FILE * f, int thread)
{
	fprintf(f, "# Frequency %f\n", 1e9 / interval);
	fprintf(f, "# Ticks per ns: %g\n", ticksperns);
	fprintf(f, "# octave: pkg load signal\n");
	fprintf(f, "# x = load(<file name>)\n");
	fprintf(f, "# pwelch(x(:,2),[],[],[],%f)\n", 1e9 / interval);
	fprintf(f, "# thread %d, core %d\n", thread, get_coreid());
	fprintf(f, "# start delay %lu msec\n", delay_msec);
	fprintf(f, "# Total count is %llu\n", total_count);
	fprintf(f, "# Max possible work is %llu\n", max_work);
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

static void *ftq_thread(void *arg)
{
	/* thread number, zero based. */
	int thread_num = (uintptr_t) arg;
	int offset;
	ticks tickinterval;
	unsigned long total_count = 0;

	/* core # is thread # for some OSs (not Akaros pth 2LS) */
	if (pin_threads)
		wireme(thread_num);

	if (set_realtime) {
		int cores = get_num_cores();

		/*
		 * Leave at least rt_free_cores cores to the OS to run things
		 * while the test runs.
		 */
		if (thread_num + rt_free_cores < cores)
			set_sched_realtime();
	}

	offset = thread_num * numsamples;
	tickinterval = interval * ticksperns;

	while (!hounds) ;

	ftq_mdelay(delay_msec);

	total_count = main_loops(samples, numsamples, tickinterval, offset);

	return (void*)total_count;
}

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
	size_t samples_size;

	/* default output name prefix */
	sprintf(outname, DEFAULT_OUTNAME);

	/*
	 * getopt_long to parse command line options.
	 * basically the code from the getopt man page.
	 */
	while (1) {
		int c;
		int option_index = 0;
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			{"numsamples", 0, 0, 'n'},
			{"frequency", 0, 0, 'f'},
			{"outname", 0, 0, 'o'},
			{"stdout", 0, 0, 's'},
			{"threads", 0, 0, 't'},
			{"ticksperns", 0, 0, 'T'},
			{"ignore_wire_failures", 0, 0, 'w'},
			{"realtime", 0, 0, 'r'},
			{"delay", 0, 0, 'd'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "n:hsf:o:t:T:wrd:", long_options,
						&option_index);
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
	/* allocate sample storage */
	samples_size = sizeof(struct sample) * numsamples * numthreads;
	samples = mmap(0, samples_size, PROT_READ | PROT_WRITE,
	               MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE,
	               -1, 0);
	if (samples != MAP_FAILED) {
		if (mlock(samples, samples_size) < 0)
			perror("Failed to mlock");
	} else {
		perror("Failed to mmap, will just malloc");
		samples = malloc(samples_size);
		assert(samples);
	}
	/* in case mmap failed or MAP_POPULATE didn't populate */
	memset(samples, 0, samples_size);

	if (use_stdout == 1 && numthreads > 1) {
		fprintf(stderr, "ERROR: cannot output to stdout for more than one thread.\n");
		exit(EXIT_FAILURE);
	}

	if (ticksperns == 0.0)
		ticksperns = compute_ticksperns();

	/*
	 * set up sampling.  first, take a few bogus samples to warm up the
	 * cache and pipeline
	 */
	if (use_threads == 1) {
		if (threadinit(numthreads) < 0) {
			fprintf(stderr, "threadinit failed\n");
			assert(0);
		}
		threads = malloc(sizeof(pthread_t) * numthreads);
		/* fault in the array, o/w we'd take the faults after 'start' */
		memset(threads, 0, sizeof(pthread_t) * numthreads);
		assert(threads != NULL);
		/* TODO: abstract this nonsense into a call in
		 * linux.c/akaros.c/etc */
		for (i = 0; i < numthreads; i++) {
			rc = pthread_create(&threads[i], NULL, ftq_thread,
					    (void *)(intptr_t) i);
			if (rc) {
				fprintf(stderr,
					"ERROR: pthread_create() failed.\n");
				exit(EXIT_FAILURE);
			}
		}

		hounds = 1;
		/* TODO: abstract this nonsense into a call in
		 * linux.c/akaros.c/etc */
		for (i = 0; i < numthreads; i++) {
			void *retval;

			rc = pthread_join(threads[i], &retval);
			if (rc) {
				fprintf(stderr,
					"ERROR: pthread_join() failed.\n");
				exit(EXIT_FAILURE);
			}
			total_count += (unsigned long)retval;
		}
	} else {
		pin_threads = 0;
		hounds = 1;
		total_count = (unsigned long)ftq_thread(0);
	}

	fprintf(stderr, "Ticks per ns: %f\n", ticksperns);
	fprintf(stderr, "Sample frequency is %f\n", 1e9 / interval);
	fprintf(stderr, "Total count is %llu\n", total_count);
	for (j = 0; j < numthreads; j++) {
		for (i = 0; i < numsamples; i++) {
			int ix = j * numsamples + i;
			if (samples[ix].count > max_work) {
				max_work = samples[ix].count;
			}
		}
	}
	max_work *= numthreads * numsamples;
	fprintf(stderr, "Max possible work is %llu\n", max_work);
	fprintf(stderr, "Fraction is %g\n", (1.0 * total_count) / max_work);

	if (use_stdout == 1) {
		header(stdout, 0);
		base = samples[0].ticklast;
		for (i = 0; i < numsamples; i++) {
			fprintf(stdout, "%lld %lld\n",
				(ticks)((samples[i].ticklast - base)
					/ ticksperns), samples[i].count);
		}
	} else {

		for (j = 0; j < numthreads; j++) {
			sprintf(fname, "%s_%d.dat", outname, j);
			fp = fopen(fname, "w");
			if (!fp) {
				perror("can not create file");
				exit(EXIT_FAILURE);
			}
			header(fp, j);
			base = samples[numsamples * j].ticklast;
			for (i = 0; i < numsamples; i++) {
				int ix = j * numsamples + i;
				fprintf(fp, "%lld %lld\n",
					(ticks)((samples[ix].ticklast - base) / ticksperns),
					samples[ix].count);
			}
			fclose(fp);
		}
	}

	if (use_threads)
		pthread_exit(NULL);

	exit(EXIT_SUCCESS);
}
