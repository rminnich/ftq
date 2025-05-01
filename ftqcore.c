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
 * 12/30/2007 : Updated to add pthreads support for FTQ execution on
 *              shared memory multiprocessors (multicore and SMPs).
 *              Also, removed unused TAU support.
 *
 * Licensed under the terms of the GNU Public License.  See LICENSE
 * for details.
 *
 * Keep this file OS-independent.
 */
#include "ftq.h"

/*************************************************************************
 * All time base here is in ticks; computation to ns is done elsewhere   *
 * as needed.                                                            *
 *************************************************************************/

/* Global to prevent the compiler from throwing away the work. */
float work_keep;

unsigned long main_loops(struct sample *samples, size_t numsamples,
                         ticks tickinterval, int offset)
{
	unsigned long done;
	unsigned long count;
	float work = 133.7;
	unsigned long total_count = 0;
	ticks ticknow, ticklast, tickend;

	tickend = getticks();

	for (done = 0; done < numsamples; done++) {
		count = 0;
		tickend += tickinterval;

		for (ticknow = ticklast = getticks();
			 ticknow < tickend; ticknow = getticks()) {

			/*
			 * This is the older style.  The problem is that even
			 * without any OS/platform interference, this code will
			 * not take a constant amount of time due to
			 * microarchitectural details.  The IPC can vary wildly
			 * by factors of 3-4x.  See the writeup elsewhere for
			 * details.
			 *
			 * But let's keep the code around for the hell of it.
			 */
#if 0
			for (int k = 0; k < ITERCOUNT; k++)
				count++;
			for (int k = 0; k < (ITERCOUNT - 1); k++)
				count--;
#endif

			/*
			 * not the prettiest, but it's the stablest workload
			 * i've found on x86, especially SPR/Golden Cove.
			 *
			 * you don't want the work loop to be so short that it
			 * hammers rdtsc, but also not obnoxiously long.
			 */
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;
			work = work * work;

			count++;
		}

		samples[done + offset].ticklast = ticklast;
		samples[done + offset].count = count;
		total_count += count;
	}

	work_keep = work;
	return total_count;
}
