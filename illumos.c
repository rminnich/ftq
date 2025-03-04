// SPDX-License-Identifier: GPL-2.0-only
#include <sys/types.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <sys/utsname.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include "ftq.h"

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
	processor_info_t pinfo;
	struct utsname utsname;

	if (uname(&utsname) == 0) {
		fprintf(f, "# %s\n", utsname.sysname);
		fprintf(f, "# %s\n", utsname.nodename);
		fprintf(f, "# %s\n", utsname.release);
		fprintf(f, "# %s\n", utsname.version);
		fprintf(f, "# %s\n", utsname.machine);
	}

	if (processor_info(core, &pinfo) < 0) {
		perror("processor_info");
		return;
	}
	fprintf(f, "# processor %d\n", core);
	fprintf(f, "# %s\n", pinfo.pi_processor_type);
	fprintf(f, "# %s\n", pinfo.pi_fputypes);
	fprintf(f, "# %d MHz\n", pinfo.pi_clock);
}

int threadinit(int numthreads)
{
	return 0;
}

int wireme(int core)
{
	int ret;

	ret = processor_bind(P_LWPID, P_MYID, core, NULL);
	if (ret < 0 && !ignore_wire_failures) {
		fprintf(stderr, "wireme: pid %d, core %d, %m\n", getpid(), core);
		exit(1);
	}

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

int get_pcoreid(void)
{
	processorid_t core;

	if (processor_bind(P_LWPID, P_MYID, PBIND_QUERY, &core) < 0) {
		perror("processor_bind");
		return -1;
	}

	return (int)core;
}
