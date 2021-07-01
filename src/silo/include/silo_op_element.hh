#pragma once

#include <memory>
#include <atomic>
#include <string_view>

template<typename T> class OpElement {
public:
    uint64_t key_;
    T *rcdptr_;

    OpElement() : key_(0), rcdptr_(nullptr) {}
    OpElement(uint64_t key) : key_(key) {}
    OpElement(uint64_t key, T *rcdptr) : key_(key), rcdptr_(rcdptr) {}
};


template<typename T> class ReadElement : public OpElement<T> {
private:
    Tidword tidword_;

public:
    char val_[VAL_SIZE];
    using OpElement<T>::OpElement;

    ReadElement(uint64_t key, T *rcdptr, char *val, Tidword tidword)
        : OpElement<T>::OpElement(key, rcdptr) {
        tidword_.obj_ = tidword.obj_;
        memcpy(this->val_, val, VAL_SIZE);
    }

    bool operator<(const ReadElement &right) const {
        return this->key_ < right.key_;
    }

    Tidword get_tidword() {
        return tidword_;
    }
};


template<typename T> class WriteElement : public OpElement<T> {
private:
    std::unique_ptr<char[]> val_ptr_; // NOLINT
    std::size_t val_length_{};

public:
    using OpElement<T>::OpElement;

    WriteElement(uint64_t key, T *rcdptr, std::string_view val)
        : OpElement<T>::OpElement(key, rcdptr) {
        static_assert(std::string_view("").size() == 0, "Expected behavior was broken.");
        if (val.size() != 0) {
            val_ptr_ = std::make_unique<char[]>(val.size());
            memcpy(val_ptr_.get(), val.data(), val.size());
            val_length_ = val.size();
        } else {
            val_length_ = 0;
        }
        // else : fast approach for benchmark
    }

    bool operator<(const WriteElement &right) const {
        return this->key_ < right.key_;
    }

    char *get_val_ptr() {
        return val_ptr_.get();
    }

    std::size_t get_val_length() {
        return val_length_;
    }
};
