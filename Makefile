CC = cc
# Note the -O0. You should *never* optimize this benchmark!
CFLAGS = -Wall -O0
LIBS =
LDFLAGS = $(USER_OPT)

all: basic 

basic:
	cc $(CFLAGS) -Wall ftq.c -o ftq
threads:
	cc $(CFLAGS) -funroll-loops ftq.c -D_WITH_PTHREADS_ -o t_ftq -lpthread
clean:
	rm -f ftq.o ftq ftq15 ftq31 ftq63 t_ftq*
