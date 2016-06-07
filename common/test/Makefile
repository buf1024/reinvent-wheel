CC = cc
CCFLAGS = -Wall -g -ggdb

LOG_TEST=log_test
CORO_TEST=coro_test

MYEXE=$(LOG_TEST) $(CORO_TEST)

MY_LOG_OBJ=logtest.o ../log.o
MY_CORO_OBJ=corotest.o ../coro.o

EXT_CCFLAGS = 

all:$(MYEXE)

$(LOG_TEST):$(MY_LOG_OBJ)
	$(CC) $(EXT_CCFLAGS) -o $@ $^

$(CORO_TEST):$(MY_CORO_OBJ)
	$(CC) $(EXT_CCFLAGS) -o $@ $^

clean:
	rm -rf *.o
	rm -rf ../*.o
	rm -rf $(MYEXE)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) -c -o $*.o $<

