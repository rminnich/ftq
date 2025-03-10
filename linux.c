// SPDX-License-Identifier: GPL-2.0-only
#define _GNU_SOURCE

#include "ftq.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <sys/utsname.h>
#include <sys/mman.h>

/* what clock do we use for the OS timer? */
#define TICKCLOCK CLOCK_MONOTONIC_RAW

/* return current time in ns as a 'tick' */
ticks nsec_ticks()
{
	struct timespec t;
	int ret;
	ticks val;

	ret = clock_gettime(TICKCLOCK, &t);
	if (ret) {
		fprintf(stderr, "%s: clock_gettime failed\n", __func__);
		exit(1);
	}
	val = t.tv_sec * 1000 * 1000 * 1000 + t.tv_nsec;
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
	return 0;
}

int wireme(int core)
{
	cpu_set_t *set;
	int numthreads = core + 1;
	int ret;
	size_t size;
	set = CPU_ALLOC(numthreads);
	size = CPU_ALLOC_SIZE(numthreads);
	CPU_ZERO_S(size, set);
	/* lock us down. */
	CPU_SET_S(core, size, set);
	ret = sched_setaffinity(0, size, set);
	/* just blow up. If they ignore this error the numbers will be crap. */
	if ((ret < 0) && (! ignore_wire_failures)) {
		fprintf(stderr, "wireme: pid %d, core %d, %m\n", getpid(), core);
		exit(1);
	}
	CPU_FREE(set);
	return 0;
}

double compute_ticksperns(void)
{
	ticks tickstart, tickend;
	unsigned long long timestart, timeend, ns;
	double convert;
	ticks el;

	timestart = nsec_ticks();
	tickstart = getticks();
	sleep(2);
	tickend = getticks();
	timeend = nsec_ticks();
	ns = timeend - timestart;
	el = tickend - tickstart;
	convert = (1.0 * el) / ns;
	return convert;
}

int get_num_cores(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

int get_coreid(void)
{
	return -1;
}

void set_sched_realtime(void)
{
	const int policy = SCHED_FIFO;
	struct sched_param sp;

	memset(&sp, 0, sizeof(sp));
	sp.sched_priority = sched_get_priority_max(policy);

	if (sched_setscheduler(0, policy, &sp)) {
		perror("sched_setscheduler");
		exit(1);
	}
}

struct sample *allocate_samples(size_t samples_size)
{
	struct sample *samples;

	samples = mmap(0, samples_size, PROT_READ | PROT_WRITE,
	               MAP_ANONYMOUS | MAP_PRIVATE | MAP_POPULATE,
	               -1, 0);
	if (samples != MAP_FAILED) {
		if (mlock(samples, samples_size) < 0)
			perror("Failed to mlock");
	} else {
		perror("Failed to mmap, will just malloc");
		samples = malloc(samples_size);
		assert(samples);
	}
	return samples;
}
