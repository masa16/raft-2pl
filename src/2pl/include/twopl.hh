#pragma once

#include "kvs.h"
#include "../include/header.h"

class DB {
public:
    KVS *kvs_;
    uint64_t tuple_num_;
    void makeDB(uint64_t tuple_num);
    void commitWork(client_request &req);
};


class TxnExecutor {
public:
    DB &db_;
    unsigned int thid_;
    TxnExecutor(DB &db, int thid) : db_(db), thid_(thid) {}
    bool transferWork(client_request &req);
};
