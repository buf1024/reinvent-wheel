CPPFLAGS=-Wall -Wextra -Werror -ggdb -MD
LDFLAGS=-lcrypto -lssh2

EXE=mysftp
OBJS=../mysftp.o mysftp_test.o 

all: $(OBJS)
	$(CC) -o $(EXE) $^ $(LDFLAGS)

clean:
	rm -rf *.o
	rm -rf *.d
	rm $(EXE)
