
// This file is a part of Simple-XX/SimpleKernel
// (https://github.com/Simple-XX/SimpleKernel).
//
// heap.cpp for Simple-XX/SimpleKernel.

#include "io.h"
#include "cpu.hpp"
#include "slab.h"
#include "heap.h"

HEAP::HEAP(PMM &pmm) : slab(SLAB(pmm)) {
    return;
}

HEAP::~HEAP(void) {
    return;
}

int32_t HEAP::init(void) {
    manage_init();
    io.printf("heap_init\n");
    return 0;
}

int32_t HEAP::manage_init(void) {
    slab.init();
    return 0;
}

void *HEAP::malloc(size_t byte) {
    void *addr = NULL;
    addr       = slab.alloc(byte);
    return addr;
}

void HEAP::free(void *addr) {
    slab.free(addr);
    return;
}

size_t HEAP::get_total(void) {
    uint32_t pages = 0;
    pages          = slab.get_total();
    return pages;
}

size_t HEAP::get_free(void) {
    uint32_t bytes = 0;
    bytes          = slab.get_free();
    return bytes;
}