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
