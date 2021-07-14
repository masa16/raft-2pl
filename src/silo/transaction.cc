#include <stdio.h>
#include <algorithm>
#include <string>

//#include "include/atomic_tool.hh"
//#include "include/atomic_wrapper.hh"
//#include "include/log.hh"
#include "include/transaction.hh"
#include "../include/header.h"

extern void displayDB();

TxnExecutor::TxnExecutor(DB &db, int thid) : db_(db), thid_(thid) {
    read_set_.reserve(FLAGS_max_ope);
    write_set_.reserve(FLAGS_max_ope);
    pro_set_.reserve(FLAGS_max_ope);
    max_rset_.obj_ = 0;
    max_wset_.obj_ = 0;

    //genStringRepeatedNumber(write_val_, VAL_SIZE, thid);
    uint digit = sprintf((char*)write_val_, "%d", thid);
    for (uint i = digit; i < VAL_SIZE; ++i) {
        write_val_[i] = '\0';
    }
    //printf("%s\n", write_val_);
}

void TxnExecutor::abort() {
    read_set_.clear();
    write_set_.clear();
}

void TxnExecutor::begin() {
    status_ = TransactionStatus::kInFlight;
    max_wset_.obj_ = 0;
    max_rset_.obj_ = 0;
}

void TxnExecutor::displayWriteSet() {
    printf("display_write_set()\n");
    printf("--------------------\n");
    for (auto &ws : write_set_) {
        printf("key\t:\t%lu\n", ws.key_);
    }
}

void TxnExecutor::insert([[maybe_unused]] std::uint64_t key, [[maybe_unused]] std::string_view val) { // NOLINT
}

void TxnExecutor::lockWriteSet() {
    Tidword expected, desired;

    [[maybe_unused]] retry
        :
        for (auto itr = write_set_.begin(); itr != write_set_.end(); ++itr) {
            expected = itr->rcdptr_->tidword_.load();
            for (;;) {
                if (expected.lock) {
#if NO_WAIT_LOCKING_IN_VALIDATION
                    this->status_ = TransactionStatus::kAborted;
                    if (itr != write_set_.begin()) unlockWriteSet(itr);
                    return;
#elif NO_WAIT_OF_TICTOC
                    if (itr != write_set_.begin()) unlockWriteSet(itr);
                    goto retry;
#endif
                } else {
                    desired = expected;
                    desired.lock = 1;
                    if (itr->rcdptr_->tidword_.compare_exchange_strong(expected, desired))
                        break;
                }
            }

            max_wset_ = std::max(max_wset_, expected);
        }
}

void TxnExecutor::read(std::uint64_t key) {
#if ADD_ANALYSIS
    std::uint64_t start = rdtscp();
#endif

    // these variable cause error (-fpermissive)
    // "crosses initialization of ..."
    // So it locate before first goto instruction.
    Tidword expected, check;

    /**
     * read-own-writes or re-read from local read set.
     */
    if (searchReadSet(key) || searchWriteSet(key)) goto FINISH_READ;

    /**
     * Search tuple from data structure.
     */
    Tuple *tuple;
    tuple = db_.get_tuple(key);

    //(a) reads the TID word, spinning until the lock is clear

    expected = tuple->tidword_.load();
    // check if it is locked.
    // spinning until the lock is clear

    for (;;) {
        while (expected.lock) {
            expected = tuple->tidword_.load();
        }

        //(b) checks whether the record is the latest version
        // omit. because this is implemented by single version

        //(c) reads the data
        memcpy(return_val_, tuple->val_, VAL_SIZE);

        //(d) performs a memory fence
        // don't need.
        // order of load don't exchange.

        //(e) checks the TID word again
        check = tuple->tidword_.load();
        if (expected == check) break;
        expected = check;
#if ADD_ANALYSIS
        ++sres_->local_extra_reads_;
#endif
    }

    read_set_.emplace_back(key, tuple, return_val_, expected);
    // emplace is often better performance than push_back.

#if SLEEP_READ_PHASE
    sleepTics(SLEEP_READ_PHASE);
#endif

 FINISH_READ:

#if ADD_ANALYSIS
    sres_->local_read_latency_ += rdtscp() - start;
#endif
    return;
}


ReadElement<Tuple> *TxnExecutor::searchReadSet(std::uint64_t key) {
    for (auto itr = read_set_.begin(); itr != read_set_.end(); ++itr) {
        if (itr->key_ == key) return &(*itr);
    }

    return nullptr;
}

WriteElement<Tuple> *TxnExecutor::searchWriteSet(std::uint64_t key) {
    for (auto itr = write_set_.begin(); itr != write_set_.end(); ++itr) {
        if (itr->key_ == key) return &(*itr);
    }

    return nullptr;
}

void TxnExecutor::unlockWriteSet() {
    Tidword expected, desired;

    for (auto itr = write_set_.begin(); itr != write_set_.end(); ++itr) {
        expected = itr->rcdptr_->tidword_.load();
        desired = expected;
        desired.lock = 0;
        itr->rcdptr_->tidword_.store(desired);
    }
}

void TxnExecutor::unlockWriteSet(
    std::vector<WriteElement<Tuple>>::iterator end) {
    Tidword expected, desired;

    for (auto itr = write_set_.begin(); itr != end; ++itr) {
        expected = itr->rcdptr_->tidword_.load();
        desired = expected;
        desired.lock = 0;
        itr->rcdptr_->tidword_.store(desired);
    }
}

bool TxnExecutor::validationPhase() {
#if ADD_ANALYSIS
    std::uint64_t start = rdtscp();
#endif

    /* Phase 1
     * lock write_set_ sorted.*/
    sort(write_set_.begin(), write_set_.end());
    lockWriteSet();
#if NO_WAIT_LOCKING_IN_VALIDATION
    if (this->status_ == TransactionStatus::kAborted) return false;
#endif

    asm volatile("":: : "memory");
    ThLocalEpoch[thid_].store(GlobalEpoch.load(std::memory_order_acquire), std::memory_order_release);
    asm volatile("":: : "memory");

    /* Phase 2 abort if any condition of below is satisfied.
     * 1. tid of read_set_ changed from it that was got in Read Phase.
     * 2. not latest version
     * 3. the tuple is locked and it isn't included by its write set.*/

    Tidword check;
    for (auto itr = read_set_.begin(); itr != read_set_.end(); ++itr) {
        // 1
        check = itr->rcdptr_->tidword_.load();
        if (itr->get_tidword().epoch != check.epoch ||
            itr->get_tidword().tid != check.tid) {
#if ADD_ANALYSIS
            sres_->local_vali_latency_ += rdtscp() - start;
#endif
            this->status_ = TransactionStatus::kAborted;
            unlockWriteSet();
            return false;
        }
        // 2
        // if (!check.latest) return false;

        // 3
        if (check.lock && !searchWriteSet(itr->key_)) {
#if ADD_ANALYSIS
            sres_->local_vali_latency_ += rdtscp() - start;
#endif
            this->status_ = TransactionStatus::kAborted;
            unlockWriteSet();
            return false;
        }
        max_rset_ = std::max(max_rset_, check);
    }

    // goto Phase 3
#if ADD_ANALYSIS
    sres_->local_vali_latency_ += rdtscp() - start;
#endif
    this->status_ = TransactionStatus::kCommitted;
    return true;
}


void TxnExecutor::write(std::uint64_t key, std::string_view val) {

    if (searchWriteSet(key)) return;

    /**
     * Search tuple from data structure.
     */
    Tuple *tuple;
    ReadElement<Tuple> *re;
    re = searchReadSet(key);
    if (re) {
        tuple = re->rcdptr_;
    } else {
        tuple = db_.get_tuple(key);
    }

    write_set_.emplace_back(key, tuple, val);
}

void TxnExecutor::writePhase() {
    // It calculates the smallest number that is
    //(a) larger than the TID of any record read or written by the transaction,
    //(b) larger than the worker's most recently chosen TID,
    // and (C) in the current global epoch.

    Tidword tid_a, tid_b, tid_c;

    // calculates (a)
    // about read_set_
    tid_a = std::max(max_wset_, max_rset_);
    tid_a.tid++;

    // calculates (b)
    // larger than the worker's most recently chosen TID,
    tid_b = mrctid_;
    tid_b.tid++;

    // calculates (c)
    tid_c.epoch = ThLocalEpoch[thid_].load();

    // compare a, b, c
    Tidword maxtid = std::max({tid_a, tid_b, tid_c});
    maxtid.lock = 0;
    maxtid.latest = 1;
    mrctid_ = maxtid;

    // write(record, commit-tid)
    for (auto itr = write_set_.begin(); itr != write_set_.end(); ++itr) {
        // update and unlock
        if (itr->get_val_length() == 0) {
            // fast approach for benchmark
            memcpy(itr->rcdptr_->val_, write_val_, VAL_SIZE);
        } else {
            memcpy(itr->rcdptr_->val_, itr->get_val_ptr(), itr->get_val_length());
        }
        itr->rcdptr_->tidword_.store(maxtid);
    }

    read_set_.clear();
    write_set_.clear();
}


/*
bool TxnExecutor::transactionWork()
{
    begin();
    for (auto itr = pro_set_.begin(); itr != pro_set_.end(); ++itr) {
        if (itr->ope_ == Ope::READ) {
            read(itr->key_);
        } else if (itr->ope_ == Ope::WRITE) {
            write(itr->key_);
        } else if (itr->ope_ == Ope::READ_MODIFY_WRITE) {
            read(itr->key_);
            write(itr->key_);
        } else {
            //ERR;
        }
    }
    if (validationPhase()) {
        writePhase();
        return true;
    } else {
        abort();
        return false;
    }
}
*/

bool TxnExecutor::transferWork(client_request &req)
{
    union {
        char val[VAL_SIZE];
        int32_t amount;
    } from, to;

    assert(req.from != req.to);

    // sort
    int tmp;
    if(req.from > req.to){
        tmp = req.from;
        req.from = req.to;
        req.to = tmp;
        req.diff = -req.diff;
    }

    begin();

    // transfer
    read(req.from);
    read(req.to);
    memcpy(from.val, read_set_[0].val_, VAL_SIZE);
    memcpy(to.val, read_set_[1].val_, VAL_SIZE);
    from.amount -= req.diff;
    to.amount += req.diff;
    write(req.from, from.val);
    write(req.to, to.val);

    if (validationPhase()) {
        writePhase();
        return true;
    } else {
        abort();
        return false;
    }
}
