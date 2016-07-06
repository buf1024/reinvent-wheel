CC = cc
CCFLAGS = -I../common -std=gnu99 -O0 -Wall -Wextra -g -ggdb -fPIC -fpic

EXE = simpleproxy

OBJS=$(patsubst %.c,%.o,$(wildcard *.c))


EXT_CCFLAGS = -rdynamic -lpthread -ldl -L../common -lmylib

all:$(EXE)

$(EXE):$(OBJS)
	$(CC) -o $@ $^ $(EXT_CCFLAGS)

clean:
	rm -rf $(OBJS) 
	rm -rf $(EXE)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) -c -o $*.o $<
