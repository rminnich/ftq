#include <stdio.h>
#include <time.h>

/* do what is needed and return the time resolution in nanoseconds. */
int
initticks()
{
	struct timespec t;
	int ret;

	ret = clock_getres(TICKCLOCK, &t);
	if (ret) {
		fprintf(stderr, "%s: clock_getres failed\n", __func__);
		exit(1);
	}
	return (int)t.tv_nsec;
}

/* return current time in ns as a 'tick' */
ticks
nsec()
{
	struct timespec t;
	int ret;
	ticks val;

	ret = clock_gettime(TICKCLOCK, &t);
	if (ret) {
		fprintf(stderr, "%s: clock_gettime failed\n", __func__);
		exit(1);
	}
	val = t.tv_sec * 1024 * 1024 * 1024 + t.tv_nsec;
	return val;
}





