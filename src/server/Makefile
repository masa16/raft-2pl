
T1 := raft
T2 := raft-follower

CC := g++
CFLAGS := -g -c -O0 -std=c++11 -I../include -Wall
LDFLAGS := -lm -pthread

S1 := clientnode.cc config.cc filehandler.cc functions.cc kvs.cc log.cc main.cc node.cc raftnode.cc rpc.cc status.cc worker.cc
O1 := $(S1:%.cc=%.o)

all: $(T1) $(T2)

$(T1): $(O1) $(T1).o
	$(CC) $(LDFLAGS) -o $@ $^

$(T2): $(O1) $(T2).o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ $<

$(T2).o: raft.cc
	$(CC) $(CFLAGS) -o $@ $< -DCAN_BE_CANDIDACY=false

run: all
	./raft ../../raft.conf

clean:
	rm -f *~ *.exe *.stackdump $(T1) $(T2) *.o
