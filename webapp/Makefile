CC = cc
CCFLAGS = -std=gnu99 -DDEBUG -O0 -Wall -Wextra -g -ggdb -fPIC -fpic

AR = ar
ARFLAGS = -r -o

RANDLIB = ranlib

WEBAPP=libwebapp.a
DISTOBJ=$(WEBAPP) webapp.h
DISTTAR=webapp.tar
DISTGZ=$(DISTTAR).gz

OBJS=$(patsubst %.c,%.o,$(wildcard *.c)) 

EXT_CCFLAGS = -fpic -fPIC -lpthread -ldl -static

all:$(WEBAPP)

$(WEBAPP):$(OBJS)
	$(AR) $(ARFLAGS) -o $@ $^
	$(RANDLIB) $@

dist: $(WEBAPP)
	tar cvf $(DISTTAR) $(DISTOBJ)
	gzip $(DISTTAR)

clean:
	rm -rf $(OBJS)
	rm -rf $(WEBAPP)
	
distclean:
	rm -rf $(OBJS)
	rm -rf $(WEBAPP)
	rm -rf $(DISTGZ)


.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) $(EXT_CCFLAGS) -c -o $*.o $<

