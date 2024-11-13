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

// there's too much variance in how this is define
#define MAXPATH 256

// Architecture defines that may or may not exist on your machine.
// But a define does no harm.
//
// minimise dependencies on include hell ...
#define MPERF                 0x000000e7
#define APERF                 0x000000e8

/*
 * use cycle timers from FFTW3 (http://www.fftw.org/).  this defines a
 * "ticks" typedef, usually unsigned long long, that is used to store
 * timings.  currently this code will NOT work if the ticks typedef is
 * something other than an unsigned long long.
 */
#include "cycle.h"

/* what clock do we use for the OS timer? */
#define TICKCLOCK CLOCK_MONOTONIC_RAW

#include <sched.h>

static inline int get_pcoreid(void)
{
	return sched_getcpu();
}

static inline int msrfd(int core)
{
  char path[MAXPATH];
  snprintf(path, sizeof(path), "/dev/cpu/%d/msr", core);
  return open(path, 0);
}

static unsigned long rdmsr(int fd, uint32_t msr)
{
  unsigned long long value;
      if (pread(fd, &value, sizeof(value), msr) != sizeof(value)) {
        perror("pread");
	exit(2);
    }
      return value;
}
  
static inline unsigned long long aperf(int fd) {
  return rdmsr(fd, APERF);
}

static inline unsigned long long mperf(int fd) {
  return rdmsr(fd, MPERF);
}

static inline unsigned long long smicount(int fd) {
  return rdmsr(fd, 0x34);
}

static inline unsigned long long modulation(int fd) {
  return rdmsr(fd, 0x19a);
}
