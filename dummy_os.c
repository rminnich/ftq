// SPDX-License-Identifier: GPL-2.0-only
/*
 * Not a real OS.  The dummy's job is to catch when you add an OS-specific
 * function forget to add it to the other OSes.
 */

#define _GNU_SOURCE

#include "ftq.h"

ticks nsec_ticks()
{
	return 0;
}

void osinfo(FILE *f, int core)
{
}

int threadinit(int numthreads)
{
	return 0;
}

int wireme(int core)
{
	return 0;
}

double compute_ticksperns(void)
{
	return 0.0;
}

int get_num_cores(void)
{
	return 0;
}

int get_coreid(void)
{
	return -1;
}

void set_sched_realtime(void)
{
}

struct sample *allocate_samples(size_t samples_size)
{
	return NULL;
}
