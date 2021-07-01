#pragma once

#include "kvs.h"

bool transactionWork(KVS *kvs, client_request &req);
void commitWork(KVS *kvs, client_request &req);
