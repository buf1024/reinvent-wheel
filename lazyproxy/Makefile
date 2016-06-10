CC = cc
CCFLAGS = -Wall -Wextra -g -ggdb -I../

PROXY = lazyproxyd

OBJS = main.o


EXT_CCFLAGS = -lpthread

all:$(PROXY)

$(PROXY):$(OBJS)
	$(CC) $(EXT_CCFLAGS) -o $@ $^

clean:
	rm -rf $(OBJS) 
	rm -rf $(PROXY)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) -c -o $*.o $<
