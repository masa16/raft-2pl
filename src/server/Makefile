
T1 := raft
T2 := raft-follower
T3 := raft-silo
T4 := raft-silo-follower
T := $(T1) $(T2) $(T3) $(T4)
CC := g++
CFLAGS := -g -c -O0 -std=c++17 -I../include -Wall
LDFLAGS := -lm -lgflags -pthread

CXX_DEFINES = -DADD_ANALYSIS=0 -DBACK_OFF=0 -DBOOST_ALL_NO_LIB -DBOOST_FILESYSTEM_DYN_LINK -DDEQ_MIN_EPOCH=0 -DDURABLE_EPOCH=0 -DKEY_SIZE=8 -DMASSTREE_USE=0 -DMAX_EPOCH_DIFF=0 -DNOLOG=0 -DNO_WAIT_LOCKING_IN_VALIDATION=1 -DNO_WAIT_OF_TICTOC=0 -DPARTITION_TABLE=0 -DPMEMCPY=0 -DPROCEDURE_SORT=0 -DSLEEP_READ_PHASE=0 -DVAL_SIZE=4 -DWAL=0 -DWALMEM=0

S1 := clientnode.cc config.cc filehandler.cc functions.cc log.cc node.cc raftnode.cc rpc.cc status.cc worker.cc logging.cc
O1 := $(S1:%.cc=%.o)
SILO_OBJS=../silo/transaction.o ../silo/silo_util.o ../silo/db.o
2PL_OBJS=../2pl/kvs.o ../2pl/twopl.o
DBCC=TWOPL

all: $(T)

$(T1): $(O1) $(T1).o main.o
	$(CC) -o $@ $^ $(2PL_OBJS) $(LDFLAGS)

$(T2): $(O1) $(T2).o main.o
	$(CC) -o $@ $^ $(2PL_OBJS) $(LDFLAGS)

$(T3): $(O1) $(T3).o main-silo.o
	$(CC) -o $@ $^ $(SILO_OBJS) $(LDFLAGS)

$(T4): $(O1) $(T4).o main-silo.o
	$(CC) -o $@ $^ $(SILO_OBJS) $(LDFLAGS)

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ $< $(CXX_DEFINES) -DTWOPL

$(T2).o: raft.cc
	$(CC) $(CFLAGS) -o $@ $< $(CXX_DEFINES) -DTWOPL -DCAN_BE_CANDIDACY=false

$(T3).o: raft.cc
	$(CC) $(CFLAGS) -o $@ $< $(CXX_DEFINES) -DSILO

$(T4).o: raft.cc
	$(CC) $(CFLAGS) -o $@ $< $(CXX_DEFINES) -DSILO -DCAN_BE_CANDIDACY=false

main-silo.o: main.cc
	$(CC) $(CFLAGS) -o $@ $< $(CXX_DEFINES) -DSILO

run: all
	./raft ../../raft.conf

clean:
	rm -f *~ *.exe *.stackdump $(T) *.o
