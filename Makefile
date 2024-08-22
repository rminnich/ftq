CC = gcc
CXX= # e.g. riscv64-linux-gnu-
ACC = x86_64-ucb-akaros-gcc
CFLAGS = -Wall -O2
ACFLAGS = -Wall -O2 -Dros
LIBS =
LDFLAGS = $(USER_OPT)

PHONY = core linux akaros illumos clean

all: linux

core:
	$(CXX)$(CC) $(CFLAGS) -falign-functions=4096 -falign-loops=8 -c ftqcore.c -o ftqcore.o

linux: core
	$(CXX)$(CC) $(CFLAGS) --include linux.h -Wall ftqcore.o ftq.c linux.c -o ftq.linux -lpthread -lrt

# I hate the fact that so many linux have broken this, but there we are.
static: core
	$(CXX)$(CC) $(CFLAGS) --include linux.h -Wall ftqcore.o ftq.c linux.c -o ftq.static.linux -lpthread -lrt -static

akaros: core
	$(ACC) $(ACFLAGS) --include akaros.h -Wall ftqcore.o ftq.c akaros.c -o ftq.akaros -lpthread

illumos: core
	$(CXX)$(CC) $(CFLAGS) --include illumos.h -Wall ftqcore.o ftq.c illumos.c -o ftq.illumos -lpthread

clean:
	rm -f *.o t_ftq ftq ftq.linux ftq.static.linux ftq.akaros ftq.illumos *~

mpiftq:mpiftq.c ftq.h
	mpicc -o mpiftq mpiftq.c

mpibarrier:mpibarrier.c ftq.h
	mpicc -o mpibarrier mpibarrier.c

.PHONY: $(PHONY)
