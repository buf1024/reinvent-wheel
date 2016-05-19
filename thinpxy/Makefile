CC = cc
CCFLAGS = -I./src -O0 -Wall -ggdb -fPIC -fpic

THINHTTPD = pxy

THIN_OBJS = src/main.o src/thinconf.o


EXT_CCFLAGS = -lpthread

all:$(THINHTTPD)

$(THINHTTPD):$(THIN_OBJS)
	$(CC) $(EXT_CCFLAGS) -o $@ $^

clean:
	rm -rf $(THIN_OBJS) 
	rm -rf $(THINHTTPD)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) -c -o $*.o $<
