CC = cc
CCFLAGS = -I../../ -std=gnu99 -O0 -Wall -DDEBUG -g -ggdb -fPIC -fpic

EXE = webapp-unittest

OBJS=$(patsubst %.c,%.o,$(wildcard *.c))


EXT_CCFLAGS = -rdynamic -L../../ -lwebapp -lpthread -ldl 

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
