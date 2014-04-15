#include <stdio.h>
#include <time.h>
#include <sys/utsname.h>
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

/* do the best you can. */
void osinfo(FILE *f, int core)
{
	int readingcore = -1;
	struct utsname utsname;
	char buf[8192];
	if (uname(&utsname) == 0) {
		fprintf(f, "# %s\n", utsname.sysname);
		fprintf(f, "# %s\n", utsname.nodename);
		fprintf(f, "# %s\n", utsname.release);
		fprintf(f, "# %s\n", utsname.version);
		fprintf(f, "# %s\n", utsname.machine);
		/* who writes this stuff? */
#if _UTSNAME_DOMAIN_LENGTH - 0
# ifdef __USE_GNU
		fprintf(f, "# %s\n", utsname.domainname);
# else
		fprintf(f, "# %s\n", utsname.__domainname);
# endif
#endif
	}

	FILE *cpu = fopen("/proc/cpuinfo", "r");
	if (! cpu)
		return;
	/* note: \n comes for free. */
	/* skip to our core. */
	while (fgets(buf, sizeof(buf), cpu)) {
		if (! strncmp("processor", buf, strlen("processor"))) {
			readingcore++;
		}
		if (readingcore > core)
			break;
		if (core != readingcore)
			continue;
		fprintf(f, "# %s", buf);
	}
}




