#include <stdio.h>
#include <time.h>
#include <sys/utsname.h>
#include <ros/syscall.h>
#include <parlib/timing.h>

/* do what is needed and return the time resolution in nanoseconds. */
int initticks()
{
	/* Right now, on Akaros, not much to do, and it's not very good. */
	/* we'll assume 60 hz. */
	return 16 * 1024 * 1024;
}

/* return current time in ns as a 'tick' */
/* On akaros, this only returns the amount of time since boot, not since 1970.
 * This is fine for now, since the only user of nsec subtracts end - start.
 *
 * TODO: at least implement '#c/bintime!' Then initticks could
 * open it and this would be a simple read(bintimefd, &64bit, 8);
 */
ticks nsec_ticks()
{
	return (ticks)nsec();
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
	/* Make sure we never yield a vcore, nor ask for more */
	parlib_never_yield = TRUE;
	pthread_mcp_init();
	vcore_request_total(numthreads);
	parlib_never_vc_request = TRUE;
	return 0;

}

int wireme(int core)
{
	/* can't do this. */
	return 0;
}

double compute_ticksperns(void)
{
	return 1e-9 * get_tsc_freq();
}

int get_num_cores(void)
{
	return 1;
}

void set_sched_realtime(void)
{

}

