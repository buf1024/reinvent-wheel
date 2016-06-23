CC = cc
CCFLAGS = -Wall -Wextra -g -ggdb -I../common

PROXY = lazyproxyd

OBJS=$(patsubst %.c,%.o,$(wildcard *.c))

EXT_CCFLAGS = -L../common -lmylib -lpthread

all:$(PROXY)

$(PROXY):$(OBJS)
	$(CC) -o $@ $^ $(EXT_CCFLAGS) 

clean:
	rm -rf $(OBJS) 
	rm -rf $(PROXY)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) -c -o $*.o $<
