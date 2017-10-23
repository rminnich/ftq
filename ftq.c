/**
 * ftq.c : Fixed Time Quantum microbenchmark
 *
 * Written by Matthew Sottile (matt@cs.uoregon.edu)
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
#include "ftq.h"
#include <sys/mman.h>
#include <sys/param.h>

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

void usage(char *av0)
{
	fprintf(stderr,
			"usage: %s [-t threads] [-n samples] [-f frequency] [-h] [-o outname] [-s] [-r] [-d delay_msec] "
			"[-T ticks-per-ns-float] "
			"[-w (ignore wire failures -- only do this if there is no option"
			"\n",
			av0);
	exit(EXIT_FAILURE);
}

void header(FILE * f, float nspercycle, int thread)
{
	fprintf(f, "# Frequency %f\n", 1e9 / interval);
	fprintf(f, "# Ticks per ns: %g\n", ticksperns);
	fprintf(f, "# octave: pkg load signal\n");
	fprintf(f, "# x = load(<file name>)\n");
	fprintf(f, "# pwelch(x(:,2),[],[],[],%f)\n", 1e9 / interval);
	fprintf(f, "# thread %d, core %d\n", thread, get_pcoreid());
	fprintf(f, "# start delay %lu msec\n", delay_msec);
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
	int numthreads = 1, use_threads = 0;
	FILE *fp;
	int use_stdout = 0;
	int rc;
	pthread_t *threads;
	ticks start, end, ns;
	ticks cyclestart, cycleend, cycles, base;
	float nspercycle;
	size_t samples_size;
	unsigned long total_count = 0;

	/* default output name prefix */
	sprintf(outname, "ftq");

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
	               MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE | MAP_LOCKED,
	               -1, 0);
	if (samples == MAP_FAILED) {
		perror("Failed to mmap, will just malloc");
		samples = malloc(samples_size);
		assert(samples);
	}
	/* in case mmap failed or MAP_POPULATE didn't populate */
	memset(samples, 0, samples_size);

	if (use_stdout == 1 && numthreads > 1) {
		fprintf(stderr,
				"ERROR: cannot output to stdout for more than one thread.\n");
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
		start = nsec_ticks();
		cyclestart = getticks();
		/* TODO: abstract this nonsense into a call in linux.c/akaros.c/etc */
		for (i = 0; i < numthreads; i++) {
			rc = pthread_create(&threads[i], NULL, ftq_thread,
								(void *)(intptr_t) i);
			if (rc) {
				fprintf(stderr, "ERROR: pthread_create() failed.\n");
				exit(EXIT_FAILURE);
			}
		}

		hounds = 1;
		/* TODO: abstract this nonsense into a call in linux.c/akaros.c/etc */
		for (i = 0; i < numthreads; i++) {
			void *retval;

			rc = pthread_join(threads[i], &retval);
			if (rc) {
				fprintf(stderr, "ERROR: pthread_join() failed.\n");
				exit(EXIT_FAILURE);
			}
			total_count += (unsigned long)retval;
		}
		cycleend = getticks();
		end = nsec_ticks();
	} else {
		pin_threads = 0;
		hounds = 1;
		start = nsec_ticks();
		cyclestart = getticks();
		total_count = (unsigned long)ftq_thread(0);
		cycleend = getticks();
		end = nsec_ticks();
	}

	/* We now have the total ns used, and the total ticks used. */
	ns = end - start;
	cycles = cycleend - cyclestart;
	/* TODO: move IO to linux.c, assuming we ever need Plan 9 again.
	 * but IFDEF is NOT ALLOWED
	 */
	fprintf(stderr, "Start %lld end %lld elapsed %lld\n", start, end, ns);
	fprintf(stderr, "cyclestart %lld cycleend %lld elapsed %lld\n",
			cyclestart, cycleend, cycles);
	nspercycle = (1.0 * ns) / cycles;
	fprintf(stderr, "Avg Cycles(ticks) per ns. is %f; nspercycle is %f\n",
			(1.0 * cycles) / ns, nspercycle);
	fprintf(stderr, "Pre-computed ticks per ns: %f\n", ticksperns);
	fprintf(stderr, "Sample frequency is %f\n", 1e9 / interval);
	fprintf(stderr, "Total count is %lu\n", total_count);
	if (use_stdout == 1) {
		header(stdout, nspercycle, 0);
		base = samples[0].ticklast;
		for (i = 0; i < numsamples; i++) {
			fprintf(stdout, "%lld %lld\n",
					(ticks) (nspercycle * (samples[i].ticklast - base)),
					samples[i].count);
		}
	} else {

		for (j = 0; j < numthreads; j++) {
			sprintf(fname, "%s_%d.dat", outname, j);
			fp = fopen(fname, "w");
			if (!fp) {
				perror("can not create file");
				exit(EXIT_FAILURE);
			}
			header(fp, nspercycle, j);
			base = samples[numsamples * j].ticklast;
			for (i = 0; i < numsamples; i++) {
				fprintf(fp, "%lld %lld\n",
						(ticks) (nspercycle *
								 (samples[j * numsamples + i].ticklast - base)),
						samples[j * numsamples + i].count);
			}
			fclose(fp);
		}
	}

	if (use_threads)
		pthread_exit(NULL);

	exit(EXIT_SUCCESS);
}
