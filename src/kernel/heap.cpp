
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
    return slab.alloc(byte);
}

void HEAP::free(void *addr) {
    slab.free(addr);
    return;
}

size_t HEAP::get_total(void) {
    return slab.get_total();
}

size_t HEAP::get_free(void) {
    return slab.get_free();
}