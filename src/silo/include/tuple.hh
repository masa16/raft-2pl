#pragma once

#include <atomic>
#include <cstdint>

struct Tidword {
    union {
        uint64_t obj_;
        struct {
            bool lock: 1;
            bool latest: 1;
            bool absent: 1;
            uint64_t tid: 29;
            uint64_t epoch: 32;
        };
    };

    Tidword() : obj_(0) {};

    bool operator==(const Tidword &right) const { return obj_ == right.obj_; }
    bool operator!=(const Tidword &right) const { return !operator==(right); }
    bool operator<(const Tidword &right) const { return obj_ < right.obj_; }
    Tidword &operator=(const uint64_t &n) { obj_ = n; return *this; }
};


class Tuple {
public:
    std::atomic<Tidword> tidword_;
    char val_[VAL_SIZE];
};
