CC = cc
ACC = x86_64-ros-gcc
# Note the -O0. You should *never* optimize this benchmark!
CFLAGS = -Wall -O0
ACFLAGS = -Wall -O0 -Dros
LIBS =
LDFLAGS = $(USER_OPT)

all: linux

linux:
	cc $(CFLAGS) --include linux.h -Wall ftq.c ftqcore.c linux.c -o ftq -lpthread -lrt

akaros:
	$(ACC) $(ACFLAGS) --include linux.h -Wall ftq.c ftqcore.c linux.c -o ftq -lpthread -lrt

clean:
	rm -f *.o t_ftq ftq
