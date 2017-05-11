CC = cc
CCFLAGS = -std=gnu99 -DDEBUG -O0 -Wall -Wextra -g -ggdb -fPIC -fpic

AR = ar
ARFLAGS = -r -o

RANDLIB = ranlib

WEBAPP=libwebapp.a

OBJS=$(patsubst %.c,%.o,$(wildcard *.c)) 

EXT_CCFLAGS = -fpic -fPIC -lpthread -ldl -static

all:$(WEBAPP)

$(WEBAPP):$(OBJS)
	$(AR) $(ARFLAGS) -o $@ $^
	$(RANDLIB) $@

clean:
	rm -rf $(OBJS)
	rm -rf $(WEBAPP)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) $(EXT_CCFLAGS) -c -o $*.o $<

