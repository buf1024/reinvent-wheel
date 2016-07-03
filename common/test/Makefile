CC = cc
CCFLAGS = -Wall -g -ggdb -I../

LOG_TEST=log_test
CORO_TEST=coro_test
SOCK_TEST=sock_test
SOCKEXT_TEST=sockext_test
EVT_TEST=evt_test
TSON_TEST=tson_test

MYEXE=$(LOG_TEST) $(CORO_TEST) $(SOCK_TEST) \
	  $(SOCKEXT_TEST) $(EVT_TEST) $(TSON_TEST)

MY_LOG_OBJ=logtest.o
MY_CORO_OBJ=corotest.o
MY_SOCK_OBJ=socktest.o
MY_SOCKEXT_OBJ=sock-exttest.o
MY_EVT_OBJ=evttest.o
MY_TSON_OBJ=tsontest.o



EXT_CCFLAGS =  -L../ -lmylib -lpthread -lm

all:$(MYEXE)

$(LOG_TEST):$(MY_LOG_OBJ)
	$(CC) -o $@ $^ $(EXT_CCFLAGS) 

$(CORO_TEST):$(MY_CORO_OBJ)
	$(CC) -o $@ $^  $(EXT_CCFLAGS) 
	
$(SOCK_TEST):$(MY_SOCK_OBJ)
	$(CC) -o $@ $^ $(EXT_CCFLAGS) 

$(SOCKEXT_TEST):$(MY_SOCKEXT_OBJ)
	$(CC) -o $@ $^ $(EXT_CCFLAGS) 

$(EVT_TEST):$(MY_EVT_OBJ)
	$(CC) -o $@ $^ $(EXT_CCFLAGS) 
	
$(TSON_TEST):$(MY_TSON_OBJ)
	$(CC) -o $@ $^ $(EXT_CCFLAGS) 

clean:
	rm -rf *.o
	rm -rf $(MYEXE)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) -c -o $*.o $<

