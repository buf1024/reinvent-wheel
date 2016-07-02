CC = cc
CCFLAGS = -Wall -Wextra -g -ggdb -DHAVE_EPOLL

AR = ar
ARFLAGS = -r -o

RANDLIB = ranlib

MYLIB=libmylib.a

OBJS=$(patsubst %.c,%.o,$(wildcard *.c)) \
	 $(patsubst %.c,%.o,$(wildcard ftp/*.c)) \
	 $(patsubst %.c,%.o,$(wildcard sftp/*.c)) \
	 $(patsubst %.c,%.o,$(wildcard crypt/*.c))

EXT_CCFLAGS = -fpic -fPIC -lpthread -lcrypto

all:$(MYLIB)

$(MYLIB):$(OBJS)
	$(AR) $(ARFLAGS) -o $@ $^
	$(RANDLIB) $@

clean:
	rm -rf $(OBJS)
	rm -rf $(MYLIB)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) $(EXT_CCFLAGS) -c -o $*.o $<

