CC = cc
# Note the -O0. You should *never* optimize this benchmark!
CFLAGS = -Wall -O0
LIBS =
LDFLAGS = $(USER_OPT)

all: linux

linux:
	cc $(CFLAGS) --include linux.h -Wall ftq.c ftqcore.c linux.c -o ftq -lpthread

clean:
	rm -f *.o t_ftq ftq
