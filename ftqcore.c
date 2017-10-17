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
volatile int hounds = 0;

/*************************************************************************
 * FTQ core: does the measurement                                        *
 * All time base here is in ticks; computation to ns is done elsewhere   *
 * as needed.                                                            *
 *************************************************************************/

static unsigned long main_loops(unsigned int numsamples, ticks tickinterval,
                                int offset)
{
	int k;
	unsigned long done;
	volatile unsigned long long count;
	unsigned long total_count = 0;
	ticks ticknow, ticklast, tickend;

	tickend = getticks();

	for (done = 0; done < numsamples; done++) {
		count = 0;
		tickend += tickinterval;

		for (ticknow = ticklast = getticks();
			 ticknow < tickend; ticknow = getticks()) {
			for (k = 0; k < ITERCOUNT; k++)
				count++;
			for (k = 0; k < (ITERCOUNT - 1); k++)
				count--;
		}

		samples[(done * 2) + offset] = ticklast;
		samples[(done * 2) + 1 + offset] = count;
		total_count += count;
	}
	return total_count;
}


void *ftq_core(void *arg)
{
	/* thread number, zero based. */
	int thread_num = (uintptr_t) arg;
	int offset;
	ticks tickinterval;
	unsigned long total_count = 0;

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
	tickinterval = interval * ticksperns;

	while (!hounds) ;


	/***************************************************/
	/* first, warm things up with 1000 test iterations */
	/***************************************************/
	main_loops(1000, tickinterval, offset);

	/****************************/
	/* now do the real sampling */
	/****************************/
	total_count = main_loops(numsamples, tickinterval, offset);

	return (void*)total_count;
}
