#pragma once

#include <xmmintrin.h>

class Synchronizer {
public:
    std::atomic<bool> start_{false};
    std::atomic<bool> quit_{false};
    std::atomic<int> count_{0};

    void init_count(int n) {
        start_.store(false);
        quit_.store(false);
        count_.store(n);
    }
    void set_start() {
        while (count_.load() > 0) _mm_pause();
        start_.store(true);
    }
    void wait_start() {
        count_.fetch_add(-1);
        while (!start_.load()) _mm_pause();
    }
    void set_quit() {
        quit_.store(true);
    }
    bool quit() {
        return quit_.load();
    }
};
