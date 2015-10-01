/**
 * ftq.c : Fixed Time Quantum microbenchmark
 *
 * Written by Matthew Sottile (matt@cs.uoregon.edu)
 *
 * This is a complete rewrite of the original tscbase code by
 * Ron and Matt, in order to use a better set of portable timers,
 * and more flexible argument handling and parameterization.
 *
 * 12/30/2007 : Updated to add pthreads support for FTQ execution on
 *              shared memory multiprocessors (multicore and SMPs).
 *              Also, removed unused TAU support.
 *
 * Licensed under the terms of the GNU Public Licence.  See LICENCE_GPL
 * for details.
 */
#include "ftq.h"

/* samples: each sample has a timestamp and a work count. */
unsigned long long *samples;
unsigned long long interval = DEFAULT_INTERVAL;
unsigned long numsamples = DEFAULT_COUNT;
double ticksperns;
int rt_free_cores = 2;
int hounds = 0;

/*************************************************************************
 * FTQ core: does the measurement                                        *
 * All time base here is in ticks; computation to ns is done elsewhere   *
 * as needed.                                                            *
 *************************************************************************/
void *ftq_core(void *arg)
{
	/* thread number, zero based. */
	int thread_num = (uintptr_t) arg;
	int i, offset;
	int k;
	ticks tickstart, ticknow, ticklast, tickend, tickinterval;
	unsigned long done;
	volatile unsigned long long count;

	/* core # is thread # */
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

	offset = thread_num * numsamples * 2;
	done = 0;
	count = 0;
	tickinterval = interval * ticksperns;

	while (!hounds) ;
	tickstart = getticks();
	tickend = tickstart + (done + 1) * tickinterval;

	/***************************************************/
	/* first, warm things up with 1000 test iterations */
	/***************************************************/
	for (i = 0; i < 1000; i++) {
		count = 0;

		for (ticknow = ticklast = getticks();
			 ticknow < tickend; ticknow = getticks()) {
			for (k = 0; k < ITERCOUNT; k++)
				count++;
			for (k = 0; k < (ITERCOUNT - 1); k++)
				count--;

		}

		samples[(done * 2) + offset] = ticklast;
		samples[(done * 2) + 1 + offset] = count;

		done++;

		if (done >= numsamples)
			break;

		ticklast = getticks();

		tickend = tickstart + (done + 1) * tickinterval;
	}

	/****************************/
	/* now do the real sampling */
	/****************************/
	done = 0;
	tickstart = getticks();
	tickend = tickstart + (done + 1) * tickinterval;

	while (1) {
		count = 0;

		for (ticknow = ticklast = getticks();
			 ticknow < tickend; ticknow = getticks()) {
			for (k = 0; k < ITERCOUNT; k++)
				count++;
			for (k = 0; k < (ITERCOUNT - 1); k++)
				count--;
		}

		samples[(done * 2) + offset] = ticklast;
		samples[(done * 2) + 1 + offset] = count;

		done++;

		if (done >= numsamples)
			break;

		ticklast = getticks();

		tickend = tickstart + (done + 1) * tickinterval;
	}

	return NULL;
}
