
T1 := raft
T2 := raft-follower

CC := g++
CFLAGS := -g -c -O0 -std=c++17 -I../include -Wall
LDFLAGS := -lm -lgflags -pthread

CXX_DEFINES = -DADD_ANALYSIS=0 -DBACK_OFF=0 -DBOOST_ALL_NO_LIB -DBOOST_FILESYSTEM_DYN_LINK -DDEQ_MIN_EPOCH=0 -DDURABLE_EPOCH=0 -DKEY_SIZE=8 -DMASSTREE_USE=0 -DMAX_EPOCH_DIFF=0 -DNOLOG=0 -DNO_WAIT_LOCKING_IN_VALIDATION=1 -DNO_WAIT_OF_TICTOC=0 -DPARTITION_TABLE=0 -DPMEMCPY=0 -DPROCEDURE_SORT=0 -DSLEEP_READ_PHASE=0 -DVAL_SIZE=4 -DWAL=0 -DWALMEM=0

S1 := clientnode.cc config.cc filehandler.cc functions.cc kvs.cc log.cc main.cc node.cc raftnode.cc rpc.cc status.cc worker.cc logging.cc
O1 := $(S1:%.cc=%.o)
OBJS=../silo/transaction.o ../silo/silo_util.o ../silo/db.o

all: $(T1) $(T2)

$(T1): $(O1) $(T1).o
	$(CC) -o $@ $^ $(OBJS) $(LDFLAGS)

$(T2): $(O1) $(T2).o
	$(CC) -o $@ $^ $(OBJS) $(LDFLAGS)

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ $< $(CXX_DEFINES)

$(T2).o: raft.cc
	$(CC) $(CFLAGS) -o $@ $< $(CXX_DEFINES) -DCAN_BE_CANDIDACY=false

run: all
	./raft ../../raft.conf

clean:
	rm -f *~ *.exe *.stackdump $(T1) $(T2) *.o
