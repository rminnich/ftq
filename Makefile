CC = cc
# Note the -O0. You should *never* optimize this benchmark!
CFLAGS = -Wall -O0
LIBS =
LDFLAGS = $(USER_OPT)

ftq:
	cc $(CFLAGS) -Wall ftq.c ftqcore.c -o ftq

clean:
	rm -f *.o t_ftq ftq
