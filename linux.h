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
#include "cycle.h"

/*
 * use cycle timers from FFTW3 (http://www.fftw.org/).  this defines a
 * "ticks" typedef, usually unsigned long long, that is used to store
 * timings.  currently this code will NOT work if the ticks typedef is
 * something other than an unsigned long long.
 */
#include "cycle.h"

#include <pthread.h>


