#pragma once
#include "tuple.hh"
#include "../../include/header.h"

class DB {
private:
    //void partTableInit(size_t thid, uint64_t start, uint64_t end);
    size_t decideParallelBuildNumber();
public:
    uint64_t tuple_num_;
    void makeDB(uint64_t tuple_num);
    void display();
    Tuple *get_tuple(std::uint64_t key);
    void commitWork(client_request &req);
};
