// SPDX-License-Identifier: GPL-2.0-only
/**
 * ticks2ns.c, adapted from: 
 * ftq.c : Fixed Time Quantum microbenchmark
 *
 * Licensed under the terms of the GNU Public License.  See LICENSE
 * for details.
 */
#include "ftq.h"
#include <sys/time.h>

/**
 * macros and defines
 */

/** defaults **/
#define DEFAULT_SECONDS 30

void usage(char *av0)
{
	fprintf(stderr,
		"usage: %s [-n samples] [-i bits] [-h] [-o outname] [-s]\n",
		av0);
	exit(EXIT_FAILURE);
}

/**
 * main()
 */
int main(int argc, char **argv)
{
	/* local variables */
	struct timeval x;
	ticks start, end;
	double el;
	unsigned long timestart, timeend;
	double ns;
	double nsperticks;

	gettimeofday(&x, NULL);
	timestart = x.tv_sec;
	start = getticks();
	sleep(30);
	/* how we would do it in Plan 9
	   while (timeend - timestart < 30)
	   timeend = seconds();
	 */
	end = getticks();
	gettimeofday(&x, NULL);
	timeend = x.tv_sec;
	ns = 1e9 * (timeend - timestart);
	el = elapsed(end, start);
	nsperticks = ns / el;
	fprintf(stderr, "elapsed %g\n", el);
	fprintf(stderr, "time %ld seconds; %g ticks; nsperticks %g\n",
			timeend - timestart, el, nsperticks);
	printf("nsperticks = %g\n", nsperticks);
	return 0;
}
