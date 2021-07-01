#include <ctype.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cctype>
#include <atomic>
#include <thread>

#include "boost/filesystem.hpp"

#define GLOBAL_VALUE_DEFINE

//#include "include/atomic_tool.hh"
#include "include/common.hh"
//#include "include/silo_result.hh"
#include "include/transaction.hh"
#include "include/silo_util.hh"
//#include "include/logger.hh"

//#include "include/atomic_wrapper.hh"
//#include "include/cpu.hh"
#include "include/debug.hh"
//#include "include/fileio.hh"
//#include "include/masstree_wrapper.hh"
#include "include/random.hh"
//#include "include/result.hh"
#include "include/tsc.hh"
//#include "include/util.hh"
#include "include/zipf.hh"

#include "synchronizer.hh"
#include "statistics.hh"

using namespace std;
uint *GlobalLSN;

#if PARTITION_TABLE
#define FLAG_PARTITION true
#else
#define FLAG_PARTITION false
#endif

static Synchronizer synchronizer;
std::vector<Statistics> SiloStats(FLAGS_thread_num);

inline static void makeProcedure(std::vector <Procedure> &pro, Xoroshiro128Plus &rnd,
                                 FastZipf &zipf, size_t tuple_num, size_t max_ope,
                                 size_t thread_num, size_t rratio, bool rmw, bool ycsb,
                                 bool partition, size_t thread_id) {
    pro.clear();
    bool ronly_flag(true), wonly_flag(true);
    for (size_t i = 0; i < max_ope; ++i) {
        uint64_t tmpkey;
        // decide access destination key.
        if (ycsb) {
            if (partition) {
                size_t block_size = tuple_num / thread_num;
                tmpkey = (block_size * thread_id) + (zipf() % block_size);
            } else {
                tmpkey = zipf() % tuple_num;
            }
        } else {
            if (partition) {
                size_t block_size = tuple_num / thread_num;
                tmpkey = (block_size * thread_id) + (rnd.next() % block_size);
            } else {
                tmpkey = rnd.next() % tuple_num;
            }
        }

        // decide operation type.
        if ((rnd.next() % 100) < rratio) {
            wonly_flag = false;
            pro.emplace_back(Ope::READ, tmpkey);
        } else {
            ronly_flag = false;
            if (rmw) {
                pro.emplace_back(Ope::READ_MODIFY_WRITE, tmpkey);
            } else {
                pro.emplace_back(Ope::WRITE, tmpkey);
            }
        }
    }

    (*pro.begin()).ronly_ = ronly_flag;
    (*pro.begin()).wonly_ = wonly_flag;

#if KEY_SORT
    std::sort(pro.begin(), pro.end());
#endif // KEY_SORT
}


void worker(int thid)
{
    Statistics &stats = std::ref(SiloStats[thid]);
    Xoroshiro128Plus rnd;
    rnd.init();
    TxnExecutor trans(thid);
    FastZipf zipf(&rnd, FLAGS_zipf_skew, FLAGS_tuple_num);
    uint64_t epoch_timer_start, epoch_timer_stop;

    synchronizer.wait_start();
    if (thid == 0) epoch_timer_start = rdtscp();

    while (!synchronizer.quit()) {
        makeProcedure(trans.pro_set_, rnd, zipf, FLAGS_tuple_num, FLAGS_max_ope,
                      FLAGS_thread_num, FLAGS_rratio, FLAGS_rmw, FLAGS_ycsb, FLAG_PARTITION,
                      thid);

#if PROCEDURE_SORT
        sort(trans.pro_set_.begin(), trans.pro_set_.end());
#endif

        for (;;) {
            if (thid == 0) {
                leaderWork(epoch_timer_start, epoch_timer_stop);
            }
            if (synchronizer.quit()) return;

            if (trans.transactionWork()) {
                ++stats.commit_counts_;
                break;
            } else {
                ++stats.abort_counts_;
            }
        }
    }
    return;
}


int main(int argc, char *argv[]) {
    try {
        gflags::SetUsageMessage("Silo benchmark.");
        gflags::ParseCommandLineFlags(&argc, &argv, true);
        chkArg();
        makeDB();

        synchronizer.init_count(FLAGS_thread_num);
        std::vector<std::thread> thv;
        for (size_t i = 0; i < FLAGS_thread_num; ++i) {
            thv.emplace_back(worker, i);
        }
        // synchronize
        synchronizer.set_start();
        std::this_thread::sleep_for(std::chrono::seconds(FLAGS_extime));
        synchronizer.set_quit();
        for (auto &th : thv) th.join();
        // stats
        Statistics stats;
        for (auto &st : SiloStats) stats.add(st);
        ShowOptParameters();
        stats.display(); //FLAGS_clocks_per_us, FLAGS_extime, FLAGS_thread_num);
        return 0;
    } catch (std::bad_alloc) {
        ERR;
    }
}
