#pragma once

class Statistics {
public:
    uint64_t commit_counts_ = 0;
    uint64_t abort_counts_ = 0;
    void add(Statistics other) {};
    void display() {};
};
