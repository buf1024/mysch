CC = cc
CCFLAGS = -Wall -g

MYSCH=mysch

MYSCH_MY_OBJ=mysch.o

MYSCH_OBJ=dict.o dlist.o iniconf.o log.o util.o anet.o \
		$(MYSCH_MY_OBJ)

EXT_CCFLAGS = 

all:$(MYSCH)

$(MYSCH):$(MYSCH_OBJ)
	$(CC) $(EXT_CCFLAGS) -o $@ $(MYSCH_OBJ)

install:all
	cp -f $(MYSCH) ./bin

clean:
	rm -rf *.o
	rm -rf $(MYSCH)

.PRECIOUS:%.c
.SUFFIXES:
.SUFFIXES:.c .o
.c.o:
	$(CC) $(CCFLAGS) -c -o $*.o $<

