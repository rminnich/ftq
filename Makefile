CC = cc
ACC = x86_64-ucb-akaros-gcc
CFLAGS = -Wall -O2
ACFLAGS = -Wall -O2 -Dros
LIBS =
LDFLAGS = $(USER_OPT)

PHONY = linux core akaros

all: linux

core:
	cc $(CFLAGS) -falign-functions=4096 -c ftqcore.c -o ftqcore.o

linux: core
	cc $(CFLAGS) --include linux.h -Wall ftqcore.o ftq.c linux.c -o ftq.linux -lpthread -lrt
	cc $(CFLAGS) --include linux.h -Wall ftqcore.o ftq.c linux.c -o ftq.static.linux -lpthread -lrt -static

akaros: core
	$(ACC) $(ACFLAGS) --include akaros.h -Wall ftqcore.o ftq.c akaros.c -o ftq.akaros -lpthread

clean:
	rm -f *.o t_ftq ftq ftq.linux ftq.static.linux ftq.akaros *~

.PHONY: $(PHONY)
