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

void usage(char *av0)
{
	fprintf(stderr,
			"usage: %s [-t threads] [-n samples] [-i bits] [-h] [-o outname] [-s]\n",
			av0);
	exit(EXIT_FAILURE);
}

void header(FILE *f, float nspercycle, int core)
{
	fprintf(f,"# Frequency %f\n", nspercycle * interval_length);
	fprintf(f,"# octave: pkg load signal");
	fprintf(f,"# x = load(<file name>)\n");
	fprintf(f,"# pwelch(x(:,2),[],[],[],%f)\n", nspercycle * interval_length);
	fprintf(f,"# core %d\n", core);
	osinfo(f);
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
	cpu_set_t *set;

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
			{"interval", 0, 0, 'i'},
			{"outname", 0, 0, 'o'},
			{"stdout", 0, 0, 's'},
			{"threads", 0, 0, 't'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "n:hsi:o:t:", long_options, &option_index);
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
			case 'i':
				interval_bits = atoi(optarg);
				break;
			case 'n':
				numsamples = atoi(optarg);
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
	samples = malloc(sizeof(unsigned long long) * numsamples * 2 * numthreads);
	assert(samples != NULL);

	if (interval_bits > MAX_BITS || interval_bits < MIN_BITS) {
		fprintf(stderr, "WARNING: interval bits invalid.  set to %d.\n",
				MAX_BITS);
		interval_bits = MAX_BITS;
	}
	if (use_threads == 1 && numthreads < 2) {
		fprintf(stderr, "ERROR: >1 threads required for multithread mode.\n");
		exit(EXIT_FAILURE);
	}
	if (use_threads == 1 && use_stdout == 1) {
		fprintf(stderr,
				"ERROR: cannot output to stdout for multithread mode.\n");
		exit(EXIT_FAILURE);
	}

	set = CPU_ALLOC(numthreads);
	/* lock us down. */
	CPU_SET(0, set);
	sched_setaffinity(0, numthreads, set);

	/*
	 * set up sampling.  first, take a few bogus samples to warm up the
	 * cache and pipeline
	 */
	interval_length = 1 << interval_bits;

	if (use_threads == 1) {
		threads = malloc(sizeof(pthread_t) * numthreads);
		assert(threads != NULL);
		start = nsec();
		cyclestart = getticks();
		/* TODO: put a barrier in place, and threads don't start until we
		 * enable the barrier.
		 */
		for (i = 0; i < numthreads; i++) {
			CPU_SET(i, set);
			rc = pthread_create(&threads[i], NULL, ftq_core,
								(void *)(intptr_t) i);
			if (rc) {
				fprintf(stderr, "ERROR: pthread_create() failed.\n");
				exit(EXIT_FAILURE);
			}
			rc = pthread_setaffinity_np(threads[i], numthreads, set);
			if (rc) {
				fprintf(stderr, "ERROR: pthread_setaffinity_np() failed.\n");
				exit(EXIT_FAILURE);
			}
			CPU_CLR(i, set);
		}

		for (i = 0; i < numthreads; i++) {
			rc = pthread_join(threads[i], NULL);
			if (rc) {
				fprintf(stderr, "ERROR: pthread_join() failed.\n");
				exit(EXIT_FAILURE);
			}
		}
		cycleend = getticks();
		end = nsec();

	} else {
		start = nsec();
		cyclestart = getticks();
		ftq_core(0);
		cycleend = getticks();
		end = nsec();
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
	fprintf(stderr, "Cycles per ns. is %f; nspercycle is %f\n", (1.0*cycles)/ns,
		nspercycle);
	fprintf(stderr, "Sample frequency is %f\n", nspercycle * interval_length);
	if (use_stdout == 1) {
		header(stdout, nspercycle, 0);
		for (i = 0, base = samples[0]; i < numsamples; i++) {
			fprintf(stdout, "%f %lld\n",
				nspercycle * (samples[i * 2] - base), samples[i * 2 + 1]);
		}
	} else {

		for (j = 0; j < numthreads; j++) {
			sprintf(fname, "%s_%d.dat", outname, j);
			fp = fopen(fname, "w");
			if (fp < 0) {
				perror("can not create file");
				exit(EXIT_FAILURE);
			}
			header(fp, nspercycle, j);
			for (i = 0, base = samples[numsamples * j];
			     i < numsamples; i++) {
				fprintf(fp, "%f %lld\n",
					nspercycle * (samples[j*numsamples + i * 2] - base),
					samples[j*numsamples + i * 2 + 1]);
			}
			fclose(fp);
		}
	}

	pthread_exit(NULL);

	exit(EXIT_SUCCESS);
}
