
CC := g++
CFLAGS := -g -c -O0 -std=c++17 -I../include -Wall
LDFLAGS := -lm -lgflags -pthread

S1 := kvs.cc twopl.cc
O1 := $(S1:%.cc=%.o)

all: $(O1)

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ $< $(CXX_DEFINES)

run: all
	./raft ../../raft.conf

clean:
	rm -f *~ *.exe *.stackdump $(T1) $(T2) *.o
