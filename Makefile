CC ?= 
CROSS ?= # e.g. riscv64-linux-gnu-
ACC ?= x86_64-ucb-akaros-gcc
# what a mess.
CFLAGS ?= -Wall -O2 -DHAVE_ARMV8_PMCCNTR_EL0
ACFLAGS ?= -Wall -O2 -Dros
LIBS ?=
LDFLAGS ?= $(USER_OPT)

# make with this set for extra perf msr recording.
# X86_PERF_MSR 

PHONY = core linux akaros illumos clean

all: linux

core:
	$(CROSS)$(CC) $(CFLAGS) -falign-functions=4096 -falign-loops=8 -c ftqcore.c -o ftqcore.o

linuxcore:
	$(CROSS)$(CC) $(CFLAGS) -include linux.h -falign-functions=4096 -falign-loops=8 -c ftqcore.c -o ftqcore.o

linux: linuxcore
	$(CROSS)$(CC) $(CFLAGS) --include linux.h -Wall ftqcore.o ftq.c linux.c -o ftq.linux -lpthread -lrt

# I hate the fact that so many linux have broken this, but there we are.
static: linuxcore
	$(CROSS)$(CC) $(CFLAGS) --include linux.h -Wall ftqcore.o ftq.c linux.c -o ftq.static.linux -lpthread -lrt -static

akaros: core
	$(ACC) $(ACFLAGS) --include akaros.h -Wall ftqcore.o ftq.c akaros.c -o ftq.akaros -lpthread

illumos: core
	$(CROSS)$(CC) $(CFLAGS) --include illumos.h -Wall ftqcore.o ftq.c illumos.c -o ftq.illumos -lpthread

clean:
	rm -f *.o t_ftq ftq ftq.linux ftq.static.linux ftq.akaros ftq.illumos *~

mpiftq:mpiftq.c ftq.h
	mpicc -o mpiftq mpiftq.c

mpibarrier:mpibarrier.c ftq.h
	mpicc -o mpibarrier mpibarrier.c

cudabarrier:cudabarrier.c ftq.h
	mpicxx -o cudabarrier cudabarrier.c  -L /usr/local/cuda/lib64/ -lcudart

.PHONY: $(PHONY)
