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
unsigned long long interval_length;
int      interval_bits = DEFAULT_BITS;
unsigned long numsamples = DEFAULT_COUNT;

/*************************************************************************
 * FTQ core: does the measurement                                        *
 *************************************************************************/
void           *
ftq_core(void *arg)
{
	/* thread number, zero based. */
	int             thread_num = (uintptr_t) arg;
	int             i, offset;
	int             k;
	ticks           now, last, endinterval;
	unsigned long   done;
	unsigned long long count;

	offset = thread_num * numsamples * 2;
	done = 0;
	count = 0;

	last = getticks();
	endinterval = (last + interval_length) & (~(interval_length - 1));

	/***************************************************/
	/* first, warm things up with 1000 test iterations */
	/***************************************************/
	for (i = 0; i < 1000; i++) {
		count = 0;

		for (now = last; now < endinterval;) {
			for (k = 0; k < ITERCOUNT; k++)
				count++;
			for (k = 0; k < (ITERCOUNT - 1); k++)
				count--;

			now = getticks();
		}

		samples[(done * 2) + offset] = last;
		samples[(done * 2) + 1 + offset] = count;

		done++;

		if (done >= numsamples)
			break;

		last = getticks();

		endinterval = (last + interval_length) & (~(interval_length - 1));
	}

	/****************************/
	/* now do the real sampling */
	/****************************/
	done = 0;

	while (1) {
		count = 0;

		for (now = last; now < endinterval;) {
			for (k = 0; k < ITERCOUNT; k++)
				count++;
			for (k = 0; k < (ITERCOUNT - 1); k++)
				count--;

			now = getticks();
		}

		samples[(done * 2) + offset] = last;
		samples[(done * 2) + 1 + offset] = count;

		done++;

		if (done >= numsamples)
			break;

		last = getticks();

		endinterval = (last + interval_length) & (~(interval_length - 1));
	}

	return NULL;
}

