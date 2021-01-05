
// This file is a part of Simple-XX/SimpleKernel
// (https://github.com/Simple-XX/SimpleKernel).
//
// test.c for Simple-XX/SimpleKernel.

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "stdint.h"
#include "stdbool.h"
#include "assert.h"
#include "test.h"
#include "debug.h"
#include "pmm.h"
#include "vmm.h"
#include "heap.h"

bool test(void) {
    test_libc();
    test_pmm();
    test_heap();
    test_vmm();
    return true;
}

// TODO
bool test_libc(void) {
    printk_test("libc test done.\n");
    return true;
}

// TODO: 完善测试
bool test_pmm(void) {
    uint32_t cd         = 0xCD;
    void *   addr1      = NULL;
    void *   addr2      = NULL;
    void *   addr3      = NULL;
    void *   addr4      = NULL;
    uint32_t free_count = pheap_free_pages_count();
    addr1               = pmm_alloc_page(0x9F);
    assert(pheap_free_pages_count() == free_count - 0x9F,
           "pmm test addr1 alloc.\n");
    *(uint32_t *)addr1 = cd;
    assert((*(uint32_t *)addr1 == cd), "pmm test addr1 assignment.\n");
    pheap_free_page(addr1, 0x9F);
    assert(pheap_free_pages_count() == free_count, "pmm test addr1 free.\n");
    addr2 = pmm_alloc_page(1);
    assert(pheap_free_pages_count() == free_count - 1,
           "pmm test addr2 alloc.\n");
    *(int *)addr2 = cd;
    assert((*(uint32_t *)addr2 == cd), "pmm test addr2 assignment.\n");
    pheap_free_page(addr2, 1);
    assert(pheap_free_pages_count() == free_count, "pmm test addr2 free.\n");
    addr3 = pmm_alloc_page(1024);
    assert(pheap_free_pages_count() == free_count - 1024,
           "pmm test addr3 alloc.\n");
    *(int *)addr3 = cd;
    assert((*(uint32_t *)addr3 == cd), "pmm test addr3 assignment.\n");
    pheap_free_page(addr3, 1024);
    assert(pheap_free_pages_count() == free_count, "pmm test addr3 free.\n");
    addr4 = pmm_alloc_page(4096);
    assert((*(uint32_t *)addr4 == cd), "pmm test addr4 assignment.\n");
    assert(pheap_free_pages_count() == free_count - 4096,
           "pmm test addr4 alloc.\n");
    *(int *)addr4 = cd;
    pheap_free_page(addr4, 4096);
    assert(pheap_free_pages_count() == free_count, "pmm test addr4 free.\n");

    printk_test("pmm test done.\n");
    return true;
}

// TODO: 完善测试
bool test_heap(void) {
    void * addr1 = NULL;
    void * addr2 = NULL;
    void * addr3 = NULL;
    void * addr4 = NULL;
    size_t free  = heap_get_free();
    size_t total = heap_get_total();
    printk_test("free: 0x%X, total: 0x%X\n", free, total);
    addr1 = kmalloc(1);
    printk_test("free1: 0x%X, total1: 0x%X\n", heap_get_free(),
                heap_get_total());
    // assert(total == heap_get_total(), "heap test addr1 heap_get_total().\n");
    // assert(heap_get_free() == (free - 0xFF - 1),
    //        "heap test addr1 heap_get_free().\n");
    printk_test("kmalloc heap addr1: 0x%X\n", addr1);
    addr2 = kmalloc(9000);
    printk_test("kmalloc heap addr2: 0x%X\n", addr2);
    addr3 = kmalloc(4095);
    printk_test("kmalloc heap addr3: 0x%X\n", addr3);
    addr4 = kmalloc(12);
    printk_test("kmalloc heap addr4: 0x%X\n", addr4);
    printk_test("Test Heap kfree1: 0x%X\n", addr1);
    kfree(addr1);
    printk_test("Test Heap kfree2: 0x%X\n", addr2);
    kfree(addr2);
    printk_test("Test Heap kfree3: 0x%X\n", addr3);
    kfree(addr3);
    printk_test("Test Heap kfree4: 0x%X\n", addr4);
    kfree(addr4);
    void *new_addr = kmalloc(9000);
    printk_test("New kmalloc heap addr new: 0x%X\n", new_addr);

    printk_test("heap test done.\n");
    return true;
}

// TODO: 完善测试
bool test_vmm(void) {
    uint32_t *pg = 0xA0000000;
    void *    pa = kmalloc(4);
    addr_t    p;
    mmap(curr_dir, pg, pa, VMM_PAGE_PRESENT | VMM_PAGE_RW | VMM_PAGE_KERNEL);
    get_mmap(curr_dir, pg, &p);
    printk_test("p: 0x%X\n", p);
    unmmap(curr_dir, pg);
    *pg = 0xCD;
    printk_test("*pg: 0x%X\n", *pg);
    printk_test("vmm test done.\n");
    return true;
}

#ifdef __cplusplus
}
#endif
