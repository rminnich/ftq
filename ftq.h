/** defaults **/
#define MAX_SAMPLES    2000000
#define DEFAULT_COUNT  524288
#define DEFAULT_INTERVAL 100000
#define MAX_BITS       30
#define MIN_BITS       3

/**
 * Work grain, which is now fixed. 
 */
#define ITERCOUNT      32

extern unsigned long long *samples;
extern unsigned long long interval;
extern unsigned long numsamples;
extern int hounds;
extern double ticksperns;

/* ftqcore.c */
void *ftq_core(void *arg);

/* must be provided by OS code */
/* Sorry, Plan 9; don't know how to manage FILE yet */
ticks nsec(void);
void osinfo(FILE * f, int core);
int threadinit(int numthreads);
double compute_ticksperns(void);
/* known to be doable in Plan 9 and Linux. Akaros? not sure. */
int wireme(int core);
