CC = cc
CCFLAGS = -I../ -Wall -g

TEST=test

TEST_MY_OBJ=main.o

TEST_OBJ=../log.o log_test.o ../dict.o ../util.o ../iniconf.o iniconf_test.o \
		$(TEST_MY_OBJ)

EXT_CCFLAGS = 

all:$(TEST)

$(TEST):$(TEST_OBJ)
	$(CC) $(EXT_CCFLAGS) -o $@ $(TEST_OBJ)

install:all
	cp -f $(TEST) ./bin

clean:
	rm -rf *.o
	rm -rf $(TEST)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) -c -o $*.o $<

