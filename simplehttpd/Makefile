CC = cc
CCFLAGS = -I../common -I./ -I./mod -std=gnu99 -O0 -Wall -Wextra -Werror -g -ggdb -fPIC -fpic

EXE = simplehttp

OBJS=$(patsubst %.c,%.o,$(wildcard *.c)) \
      $(patsubst %.c,%.o,$(wildcard mod/*.c))


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
