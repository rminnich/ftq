// SPDX-License-Identifier: GPL-2.0-only
#include <u.h>
#include <libc.h>

#define intptr_t int *
#define NULL 0
typedef long long ticks;
#define sprintf sprint
#define fprintf fprint
#define printf print
#define stderr 2
#define stdout 1
#define exit exits
#define EXIT_FAILURE "failure"
#define EXIT_SUCCESS "success"

uvlong getticks(void);

/**
 * main()
 */
int main(int argc, char **argv)
{
	/* local variables */
	char fname_times[1024], fname_counts[1024], buf[32], outname[255];
	int i, j;
	int numthreads = 1;
	int fp;
	int use_stdout = 0;
	int wired = -1;
	int pri = 0;

	/* default output name prefix */
	sprintf(outname, "ftq");

	ARGBEGIN {
case 's':
		use_stdout = 1;
		break;
case 'o':
		{
			char *tmp = ARGF();
			if (tmp == nil)
				usage(argv0);
			sprintf(outname, "%s", tmp);
		}
		break;
case 'i':
		interval_bits = atoi(ARGF());
		break;
case 'n':
		numsamples = atoi(ARGF());
		break;
case 'p':
		pri = atoi(ARGF());
		break;
case 'w':
		wired = atoi(ARGF());
		break;
case 'h':
default:
		usage(argv0);
	}
	ARGEND
		/* sanity check */
		if (numsamples > MAX_SAMPLES) {
		fprintf(stderr, "WARNING: sample count exceeds maximum.\n");
		fprintf(stderr, "         setting count to maximum.\n");
		numsamples = MAX_SAMPLES;
	}
	/* allocate sample storage */
	samples = malloc(sizeof(unsigned long long) * numsamples * 2 * numthreads);
	assert(samples != NULL);

	if (interval_bits > MAX_BITS || interval_bits < MIN_BITS) {
		fprintf(stderr, "WARNING: interval bits invalid.  set to %d.\n",
				MAX_BITS);
		interval_bits = MAX_BITS;
	}
	if (use_threads == 1 && numthreads < 2) {
		fprintf(stderr, "ERROR: >1 threads required for multithread mode.\n");
		exit(EXIT_FAILURE);
	}
	if (use_threads == 1 && use_stdout == 1) {
		fprintf(stderr,
				"ERROR: cannot output to stdout for multithread mode.\n");
		exit(EXIT_FAILURE);
	}
	/*
	 * set up sampling.  first, take a few bogus samples to warm up the
	 * cache and pipeline
	 */
	interval_length = 1 << interval_bits;

#ifdef Plan9
	if (pri || (wired > -1)) {
		int me = getpid();
		char *name = smprint("/proc/%d/ctl", me);
		int fd = open(name, ORDWR);
		char *cmd;
		int amt;
		assert(fd > 0);
		if (wired > -1) {
			print("Wired to %d\n", wired);
			cmd = smprint("wired %d\n", wired);
			amt = write(fd, cmd, strlen(cmd));
			assert(amt >= strlen(cmd));
		}

		if (pri) {
			print("Pri to %d\n", pri);
			cmd = smprint("fixedpri %d\n", pri);
			amt = write(fd, cmd, strlen(cmd));
			assert(amt >= strlen(cmd));
		}
	}
#endif /* Plan 9 */

	if (use_threads == 1) {
#ifdef _WITH_PTHREADS_
		threads = malloc(sizeof(pthread_t) * numthreads);
		assert(threads != NULL);

		for (i = 0; i < numthreads; i++) {
			rc = pthread_create(&threads[i], NULL, ftq_core,
								(void *)(intptr_t) i);
			if (rc) {
				fprintf(stderr, "ERROR: pthread_create() failed.\n");
				exit(EXIT_FAILURE);
			}
		}

		for (i = 0; i < numthreads; i++) {
			rc = pthread_join(threads[i], NULL);
			if (rc) {
				fprintf(stderr, "ERROR: pthread_join() failed.\n");
				exit(EXIT_FAILURE);
			}
		}

		free(threads);
#endif /* _WITH_PTHREADS_ */
	} else {
		ftq_core(0);
	}

	if (use_stdout == 1) {
		for (i = 0; i < numsamples; i++) {
			fprintf(stdout, "%lld %lld\n", samples[i * 2], samples[i * 2 + 1]);
		}
	} else {

		for (j = 0; j < numthreads; j++) {
			sprintf(fname_times, "%s_%d_times.dat", outname, j);
			sprintf(fname_counts, "%s_%d_counts.dat", outname, j);

#ifdef Plan9
			fp = create(fname_times, OWRITE, 700);
#else
			fp = open(fname_times, O_CREAT | O_TRUNC | O_WRONLY, 0644);
#endif
			if (fp < 0) {
				perror("can not create file");
				exit(EXIT_FAILURE);
			}
			for (i = 0; i < numsamples; i++) {
				sprintf(buf, "%lld\n", samples[(i * 2) + (numsamples * j)]);
				write(fp, buf, strlen(buf));
			}
			close(fp);

#ifdef Plan9
			fp = create(fname_counts, OWRITE, 700);
#else
			fp = open(fname_counts, O_CREAT | O_TRUNC | O_WRONLY, 0644);
#endif
			if (fp < 0) {
				perror("can not create file");
				exit(EXIT_FAILURE);
			}
			for (i = 0; i < numsamples; i++) {
				sprintf(buf, "%lld\n", samples[i * 2 + 1 + (numsamples * j)]);
				write(fp, buf, strlen(buf));
			}
			close(fp);
		}
	}

	free(samples);

#ifdef _WITH_PTHREADS_
	pthread_exit(NULL);
#endif

	exit(EXIT_SUCCESS);
}
