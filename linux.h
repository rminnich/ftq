/* yikes! */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include "cycle.h"

/*
 * use cycle timers from FFTW3 (http://www.fftw.org/).  this defines a
 * "ticks" typedef, usually unsigned long long, that is used to store
 * timings.  currently this code will NOT work if the ticks typedef is
 * something other than an unsigned long long.
 */
#include "cycle.h"

/* what clock do we use for the OS timer? */
#define TICKCLOCK CLOCK_MONOTONIC_RAW

#include <pthread.h>

static inline int get_pcoreid(void)
{
	return -1;
}
