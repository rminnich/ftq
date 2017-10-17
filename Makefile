CC = cc
ACC = x86_64-ucb-akaros-gcc
CFLAGS = -Wall -O2
ACFLAGS = -Wall -O2 -Dros
LIBS =
LDFLAGS = $(USER_OPT)

all: linux

linux:
	cc $(CFLAGS) --include linux.h -Wall ftq.c ftqcore.c linux.c -o ftq.linux -lpthread -lrt
	cc $(CFLAGS) --include linux.h -Wall ftq.c ftqcore.c linux.c -o ftq.static.linux -lpthread -lrt -static

akaros:
	$(ACC) $(ACFLAGS) --include akaros.h -Wall ftq.c ftqcore.c akaros.c -o ftq.akaros -lpthread

clean:
	rm -f *.o t_ftq ftq ftq.linux ftq.static.linux ftq.akaros *~
