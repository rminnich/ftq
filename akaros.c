#include <stdio.h>
#include <time.h>
#include <sys/utsname.h>
#include <ros/syscall.h>

/* do what is needed and return the time resolution in nanoseconds. */
int initticks()
{
	/* Right now, on Akaros, not much to do, and it's not very good. */
	/* we'll assume 60 hz. */
	return 16*1024*1024; 
}

/* return current time in ns as a 'tick' */
/* todo: at least implement '#c/bintime!' Then initticks could
 * open it and this would be a simple read(bintimefd, &64bit, 8);
 */
ticks nsec()
{
	struct timeval t;
	ticks val;
	gettimeofday(&t, NULL);
	val = t.tv_sec * 1024 * 1024 * 1024 + t.tv_usec*1024;
	return val;
}

/* do the best you can. */
void osinfo(FILE * f, int core)
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
#ifdef __USE_GNU
		fprintf(f, "# %s\n", utsname.domainname);
#else
		fprintf(f, "# %s\n", utsname.__domainname);
#endif
#endif
	}

	FILE *cpu = fopen("/proc/cpuinfo", "r");
	if (!cpu)
		return;
	/* note: \n comes for free. */
	/* skip to our core. */
	while (fgets(buf, sizeof(buf), cpu)) {
		if (!strncmp("processor", buf, strlen("processor"))) {
			readingcore++;
		}
		if (readingcore > core)
			break;
		if (core != readingcore)
			continue;
		fprintf(f, "# %s", buf);
	}
}

int threadinit(int numthreads)
{
	/* Non-standard way to make sure we never yield a vcore (don't rely on
	 * these interfaces, you should have a good 2LS). */
	pthread_can_vcore_request(FALSE);
	pthread_need_tls(FALSE);
	pthread_lib_init();
	/* we already have 1 from init */
	if (vcore_request(numthreads - 1)) {
		printf("Unable to request %d vcores (got %d, max %d), exiting.\n",
		       numthreads, num_vcores(), max_vcores());
		return -1;
	}
	return 0;
	
}

int wireme(int core)
{
	/* can't do this. */
	return 0;
}
