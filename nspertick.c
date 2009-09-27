/**
 * ticks2ns.c, adapted from: 
 * ftq.c : Fixed Time Quantum microbenchmark
 *
 * Licensed under the terms of the GNU Public Licence.  See LICENCE_GPL
 * for details.
 */
#include "ftq.h"

/**
 * macros and defines
 */

/** defaults **/
#define DEFAULT_SECONDS 30

void 
usage(char *av0)
{
	fprintf(stderr, "usage: %s [-n samples] [-i bits] [-h] [-o outname] [-s]\n",
		av0);
	exit(EXIT_FAILURE);
}

/**
 * main()
 */
int 
main(int argc, char **argv)
{
	/* local variables */
	unsigned long start, end, timestart, timeend;
	float ns;
	float nsperticks;

	start = getticks();
	timeend = timestart = 0;
	sleep(30);
	/* how we would do it in Plan 9
	while (timeend - timestart < 30)
		timeend = seconds();
	 */
	timeend = 30;
	end = getticks();
	ns = 1e9 * (timeend - timestart); 
	nsperticks = ns / (end - start);
	fprintf(stderr, "time %ld seconds; %ld ticks; nsperticks %g\n", 
			timeend - timestart, end - start, nsperticks);
	printf("nsperticks = %f\n", nsperticks);
	return 0;
}
