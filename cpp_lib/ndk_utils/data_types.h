#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <format>

struct Buf {
    void* bits = nullptr;
    size_t size = 0;
    static Buf alloc(size_t size) {
        Buf buf;
        buf.bits = ::malloc(size);
        buf.size = size;
        return buf;
    }
    void free() {
        ::free(bits);
        bits = nullptr;
        size = 0;
    }
    virtual std::string toString() {
        return std::format("data: {}, size: {}", bits, size);
    }
};

