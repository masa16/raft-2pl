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

#include "include/config.hh"
#include "include/cache_line_size.hh"
#include "include/db.hh"
#include "../silo/include/debug.hh"

using std::cout;
using std::endl;

static alignas(CACHE_LINE_SIZE) Tuple *Table;

void DB::display() {
    Tuple *tuple;
    for (unsigned int i = 0; i < tuple_num_; ++i) {
        tuple = &Table[i];
        cout << "------------------------------" << endl;  //-は30個
        cout << "key: " << i << endl;
        cout << "val: " << tuple->val_ << endl;
        cout << "TIDword: " << tuple->tidword_.load(std::memory_order_relaxed).obj_ << endl;
        cout << "bit: " << tuple->tidword_.load(std::memory_order_relaxed).obj_ << endl;
        cout << endl;
    }
}

static void partTableInit([[maybe_unused]] size_t thid, uint64_t start, uint64_t end) {
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

size_t DB::decideParallelBuildNumber() {
    // if table size is very small, it builds by single thread.
    if (tuple_num_ < 1000) return 1;

    // else
    for (size_t i = std::thread::hardware_concurrency(); i > 0; --i) {
        if (tuple_num_ % i == 0) {
            return i;
        }
        if (i == 1) ERR;
    }

    return 0;
}

void DB::makeDB(uint64_t tuple_num) {
    tuple_num_ = tuple_num;
    if (posix_memalign((void **) &Table, PAGE_SIZE,
                       (tuple_num_) * sizeof(Tuple)) != 0)
        ERR;
#if dbs11
    if (madvise((void *)Table, (tuple_num_) * sizeof(Tuple),
                MADV_HUGEPAGE) != 0)
        ERR;
#endif

    size_t maxthread = decideParallelBuildNumber();

    std::vector<std::thread> thv;
    for (size_t i = 0; i < maxthread; ++i)
        thv.emplace_back(partTableInit, i, i * (tuple_num_ / maxthread),
                         (i + 1) * (tuple_num_ / maxthread) - 1);
    for (auto &th : thv) th.join();
}


Tuple *DB::get_tuple(std::uint64_t key) {
    return &Table[key];
}


void DB::commitWork(client_request &req) {
}
