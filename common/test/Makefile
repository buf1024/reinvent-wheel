CC = cc
CCFLAGS = -Wall -g -ggdb

LOG_TEST=logtest

MYEXE=$(LOG_TEST)

MY_LOG_OBJ=logtest.o ../log.o

EXT_CCFLAGS = 

all:$(MYEXE)

$(LOG_TEST):$(MY_LOG_OBJ)
	$(CC) $(EXT_CCFLAGS) -o $@ $(MY_LOG_OBJ)

clean:
	rm -rf *.o
	rm -rf $(MYEXE)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) -c -o $*.o $<

