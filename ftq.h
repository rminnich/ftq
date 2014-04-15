/** defaults **/
#define MAX_SAMPLES    2000000
#define DEFAULT_COUNT  524288
#define DEFAULT_BITS   20
#define MAX_BITS       30
#define MIN_BITS       3

/**
 * Work grain, which is now fixed. 
 */
#define ITERCOUNT      32

extern unsigned long long *samples;
extern unsigned long long interval_length;
extern int interval_bits;
extern unsigned long numsamples;

/* ftqcore.c */
void *ftq_core(void *arg);
