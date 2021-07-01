#include <stdio.h>
#include <sys/syscall.h>  // syscall(SYS_gettid),
#include <sys/time.h>
#include <sys/types.h>  // syscall(SYS_gettid),
#include <unistd.h>     // syscall(SYS_gettid),

#include <bitset>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <thread>
#include <vector>

//#include "include/atomic_tool.hh"
#include "include/common.hh"
#include "include/transaction.hh"
#include "include/tuple.hh"
#include "include/silo_util.hh"

#include "include/cache_line_size.hh"
#include "include/config.hh"
#include "include/debug.hh"
//#include "include/masstree_wrapper.hh"
#include "include/procedure.hh"
#include "include/random.hh"
#include "include/tsc.hh"
//#include "include/util.hh"
//#include "include/zipf.hh"

/*
alignas(CACHE_LINE_SIZE) Tuple *Table;
alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> *ThLocalEpoch;
alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> *CTIDW;
alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> *ThLocalDurableEpoch;
alignas(CACHE_LINE_SIZE) std::atomic<uint64_t> DurableEpoch;

int FLAGS_thread_num = 9;
int FLAGS_tuple_num = 10;
int FLAGS_epoch_time = 40;
int FLAGS_clocks_per_us = 2000;
*/

void chkArg() {
    displayParameter();

    if (FLAGS_rratio > 100) {
        ERR;
    }

    if (FLAGS_zipf_skew >= 1) {
        cout << "FLAGS_zipf_skew must be 0 ~ 0.999..." << endl;
        ERR;
    }
#if DURABLE_EPOCH
    if (FLAGS_thread_num < FLAGS_logger_num) {
        cout << "FLAGS_logger_num must be <= FLAGS_thread_num..." << endl;
        ERR;
    }
#endif

    if (posix_memalign((void **) &ThLocalEpoch, CACHE_LINE_SIZE,
                       FLAGS_thread_num * sizeof(std::atomic<uint64_t>)) != 0)
        ERR;
    if (posix_memalign((void **) &CTIDW, CACHE_LINE_SIZE,
                       FLAGS_thread_num * sizeof(std::atomic<uint64_t>)) != 0)
        ERR;
#if DURABLE_EPOCH
    if (posix_memalign((void **) &ThLocalDurableEpoch, CACHE_LINE_SIZE,
                       FLAGS_logger_num * sizeof(uint64_t_64byte)) != 0)
        ERR;
#endif

    // init
    for (unsigned int i = 0; i < FLAGS_thread_num; ++i) {
        ThLocalEpoch[i].store(0);
        CTIDW[i].store(~(uint64_t)0);
    }
#if DURABLE_EPOCH
    for (unsigned int i = 0; i < FLAGS_logger_num; ++i) {
        ThLocalDurableEpoch[i].obj_ = 0;
    }
    DurableEpoch.obj_ = 0;
#endif
}

void displayDB() {
    Tuple *tuple;
    for (unsigned int i = 0; i < FLAGS_tuple_num; ++i) {
        tuple = &Table[i];
        cout << "------------------------------" << endl;  //-は30個
        cout << "key: " << i << endl;
        cout << "val: " << tuple->val_ << endl;
        cout << "TIDword: " << tuple->tidword_.load(std::memory_order_relaxed).obj_ << endl;
        cout << "bit: " << tuple->tidword_.load(std::memory_order_relaxed).obj_ << endl;
        cout << endl;
    }
}

void displayParameter() {
    cout << "#FLAGS_clocks_per_us:\t" << FLAGS_clocks_per_us << endl;
    cout << "#FLAGS_epoch_time:\t" << FLAGS_epoch_time << endl;
    cout << "#FLAGS_extime:\t\t" << FLAGS_extime << endl;
    cout << "#FLAGS_max_ope:\t\t" << FLAGS_max_ope << endl;
    cout << "#FLAGS_rmw:\t\t" << FLAGS_rmw << endl;
    cout << "#FLAGS_rratio:\t\t" << FLAGS_rratio << endl;
    cout << "#FLAGS_thread_num:\t" << FLAGS_thread_num << endl;
#if DURABLE_EPOCH
    cout << "#FLAGS_logger_num:\t" << FLAGS_logger_num << endl;
    cout << "#FLAGS_buffer_num:\t" << FLAGS_buffer_num << endl;
    cout << "#FLAGS_buffer_size:\t" << FLAGS_buffer_size << endl;
    cout << "#FLAGS_epoch_diff:\t" << FLAGS_epoch_diff << endl;
#endif
    cout << "#FLAGS_tuple_num:\t" << FLAGS_tuple_num << endl;
    cout << "#FLAGS_ycsb:\t\t" << FLAGS_ycsb << endl;
    cout << "#FLAGS_zipf_skew:\t" << FLAGS_zipf_skew << endl;
}

/*
  void genLogFile(std::string &logpath, const int thid) {
  genLogFileName(logpath, thid);
  createEmptyFile(logpath);
  }
*/

void partTableInit([[maybe_unused]] size_t thid, uint64_t start, uint64_t end) {
#if MASSTREE_USE
    MasstreeWrapper<Tuple>::thread_init(thid);
#endif

    for (auto i = start; i <= end; ++i) {
        Tuple *tmp;
        Tidword tidw;
        tidw.epoch = 1;
        tidw.latest = 1;
        tidw.lock = 0;
        tmp = &Table[i];
        tmp->tidword_.store(tidw, std::memory_order_relaxed);
        tmp->val_[0] = 'a';
        tmp->val_[1] = '\0';

#if MASSTREE_USE
        MT.insert_value(i, tmp);
#endif
    }
}

size_t decideParallelBuildNumber(size_t tuple_num) {
    // if table size is very small, it builds by single thread.
    if (tuple_num < 1000) return 1;

    // else
    for (size_t i = std::thread::hardware_concurrency(); i > 0; --i) {
        if (tuple_num % i == 0) {
            return i;
        }
        if (i == 1) ERR;
    }

    return 0;
}

void makeDB() {
    if (posix_memalign((void **) &Table, PAGE_SIZE,
                       (FLAGS_tuple_num) * sizeof(Tuple)) != 0)
        ERR;
#if dbs11
    if (madvise((void *)Table, (FLAGS_tuple_num) * sizeof(Tuple),
                MADV_HUGEPAGE) != 0)
        ERR;
#endif

    size_t maxthread = decideParallelBuildNumber(FLAGS_tuple_num);

    std::vector<std::thread> thv;
    for (size_t i = 0; i < maxthread; ++i)
        thv.emplace_back(partTableInit, i, i * (FLAGS_tuple_num / maxthread),
                         (i + 1) * (FLAGS_tuple_num / maxthread) - 1);
    for (auto &th : thv) th.join();
}

bool chkEpochLoaded() {
    uint64_t nowepoch = GlobalEpoch.load(std::memory_order_acquire);
    //全てのワーカースレッドが最新エポックを読み込んだか確認する．
    for (unsigned int i = 1; i < FLAGS_thread_num; ++i) {
        if (ThLocalEpoch[i].load(std::memory_order_acquire) != nowepoch)
            return false;
    }

    return true;
}

void leaderWork(uint64_t &epoch_timer_start, uint64_t &epoch_timer_stop) {
    epoch_timer_stop = rdtscp();
    if (epoch_timer_stop - epoch_timer_start > FLAGS_epoch_time * FLAGS_clocks_per_us * 1000 &&
        chkEpochLoaded()) {
        //atomicAddGE();
        GlobalEpoch.fetch_add(1);
        epoch_timer_start = epoch_timer_stop;
    }
}

void ShowOptParameters() {
    cout << "#ShowOptParameters()"
         << ": ADD_ANALYSIS " << ADD_ANALYSIS << ": BACK_OFF " << BACK_OFF
         << ": KEY_SIZE " << KEY_SIZE << ": MASSTREE_USE " << MASSTREE_USE
         << ": NO_WAIT_LOCKING_IN_VALIDATION " << NO_WAIT_LOCKING_IN_VALIDATION
         << ": PARTITION_TABLE " << PARTITION_TABLE << ": PROCEDURE_SORT "
         << PROCEDURE_SORT << ": SLEEP_READ_PHASE " << SLEEP_READ_PHASE
         << ": VAL_SIZE " << VAL_SIZE << ": WAL " << WAL << endl;
}
