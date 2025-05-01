CC ?= 
CROSS ?= # e.g. riscv64-linux-gnu-
ACC ?= x86_64-ucb-akaros-gcc
# what a mess.
CFLAGS ?= -Wall -O2 -DHAVE_ARMV8_PMCCNTR_EL0
ACFLAGS ?= -Wall -O2 -Dros
LIBS ?=
LDFLAGS ?= $(USER_OPT)

PHONY = core linux akaros illumos dummy_os clean

all: linux dummy_os

core:
	$(CROSS)$(CC) $(CFLAGS) -O0 -falign-functions=4096 -falign-loops=8 -c ftqcore.c -o ftqcore.o

linux: core
	$(CROSS)$(CC) $(CFLAGS) -Wall ftqcore.o ftq.c linux.c -o ftq.linux -lpthread -lrt

# I hate the fact that so many linux have broken this, but there we are.
static: core
	$(CROSS)$(CC) $(CFLAGS) -Wall ftqcore.o ftq.c linux.c -o ftq.static.linux -lpthread -lrt -static

akaros: core
	$(ACC) $(ACFLAGS) -Wall ftqcore.o ftq.c akaros.c -o ftq.akaros -lpthread

illumos: core
	$(CROSS)$(CC) $(CFLAGS) -Wall ftqcore.o ftq.c illumos.c -o ftq.illumos -lpthread

# Probably won't run: OS stuff is stubbed out
dummy_os: core
	$(CROSS)$(CC) $(CFLAGS) -Wall ftqcore.o ftq.c dummy_os.c -o /dev/null -lpthread

clean:
	rm -f *.o t_ftq ftq ftq.linux ftq.static.linux ftq.akaros ftq.illumos *~

mpiftq:mpiftq.c ftq.h
	mpicc -o mpiftq mpiftq.c

mpibarrier:mpibarrier.c ftq.h
	mpicc -o mpibarrier mpibarrier.c

cudabarrier:cudabarrier.c ftq.h
	mpicxx -o cudabarrier cudabarrier.c  -L /usr/local/cuda/lib64/ -lcudart

.PHONY: $(PHONY)
