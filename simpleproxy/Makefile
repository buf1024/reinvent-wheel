CC = cc
CCFLAGS = -I../common -O0 -Wall -Wextra -g -ggdb -fPIC -fpic

EXE = simpleproxy

OBJS=$(patsubst %.c,%.o,$(wildcard *.c))


EXT_CCFLAGS = -lpthread -L../common -lmylib

all:$(EXE)

$(EXE):$(OBJS)
	$(CC) $(EXT_CCFLAGS) -o $@ $^

clean:
	rm -rf $(OBJS) 
	rm -rf $(EXE)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) -c -o $*.o $<
