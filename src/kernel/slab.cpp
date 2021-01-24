
// This file is a part of Simple-XX/SimpleKernel
// (https://github.com/Simple-XX/SimpleKernel).
//
// slab.cpp for Simple-XX/SimpleKernel.

#include "io.h"
#include "stdint.h"
#include "string.h"
#include "pmm.h"
#include "slab.h"

// 初始化
static void list_init(slab_list_entry_t *list) {
    list->next = list;
    list->prev = list;
    return;
}

// 在中间添加元素
static void list_add_middle(slab_list_entry_t *prev, slab_list_entry_t *next,
                            slab_list_entry_t *new_entry) {
    next->prev      = new_entry;
    new_entry->next = next;
    new_entry->prev = prev;
    prev->next      = new_entry;
}

// 在 prev 后添加项
static void list_add_after(slab_list_entry_t *prev,
                           slab_list_entry_t *new_entry) {
    list_add_middle(prev, prev->next, new_entry);
    return;
}

// 删除元素
static void list_del(slab_list_entry_t *list) {
    list->next->prev = list->prev;
    list->prev->next = list->next;
    return;
}

// 返回前面的元素
static slab_list_entry_t *list_prev(slab_list_entry_t *list) {
    return list->prev;
}

// 返回后面的的元素
static slab_list_entry_t *list_next(slab_list_entry_t *list) {
    return list->next;
}

// 返回 chunk_info
static slab_block_t *list_slab_block(slab_list_entry_t *list) {
    return &(list->slab_block);
}

// 将 entry 设置为已使用
static void set_used(slab_list_entry_t *entry) {
    list_slab_block(entry)->allocated = SLAB_USED;
    return;
}

// 将 entry 设置为未使用
static void set_unused(slab_list_entry_t *entry) {
    list_slab_block(entry)->allocated = SLAB_UNUSED;
    return;
}

SLAB::SLAB(PMM &pmm) : pmm(pmm) {
    return;
}

SLAB::~SLAB(void) {
    return;
}

int32_t SLAB::init(void) {
    // 设置第一块内存的信息
    // 首先给链表中添加一个大小为 1 页的块
    slab_list = (slab_list_entry_t *)pmm.alloc_page(1);
    bzero(slab_list, PMM_PAGE_SIZE);
    // 填充管理信息
    addr_start  = NULL;
    addr_end    = NULL;
    heap_total  = PMM_PAGE_SIZE;
    heap_free   = PMM_PAGE_SIZE - sizeof(slab_list_entry_t);
    block_count = 1;
    list_init(slab_list);
    // 设置第一块内存的相关信息
    list_slab_block(slab_list)->allocated = SLAB_UNUSED;
    list_slab_block(slab_list)->len = PMM_PAGE_SIZE - sizeof(slab_list_entry_t);
    io.printf("slab init.\n");
    return 0;
}

// 切分内存块，len 为调用者申请的大小，不包括头大小
// 返回分割出来的管理头地址
slab_list_entry_t *SLAB::slab_split(slab_list_entry_t *entry, size_t len) {
    // 如果剩余内存大于内存头的长度+设定的最小长度
    if ((list_slab_block(entry)->len - len >
         sizeof(slab_list_entry_t) + SLAB_MIN)) {
        // 添加新的链表项，位于旧表项开始地址+旧表项长度
        slab_list_entry_t *new_entry =
            (slab_list_entry_t *)((uint8_t *)entry + sizeof(slab_list_entry_t) +
                                  len);

        bzero((void *)new_entry, list_slab_block(entry)->len - len);
        list_init(new_entry);
        // 新表项的长度为：list->len（总大小）- 头大小 - 要求分割的大小
        list_slab_block(new_entry)->len =
            list_slab_block(entry)->len - len - sizeof(slab_list_entry_t);
        set_unused(new_entry);
        list_add_after(entry, new_entry);
        // 重新设置旧链表信息
        list_slab_block(entry)->len = len;
        return new_entry;
    }
    return (slab_list_entry_t *)NULL;
}

// 合并内存块
void SLAB::slab_merge(slab_list_entry_t *list) {
    slab_list_entry_t *entry = list;
    // 合并后面的
    if (entry->next != list &&
        list_slab_block(entry->next)->allocated == SLAB_UNUSED) {
        slab_list_entry_t *next = entry->next;
        list_slab_block(entry)->len +=
            list_slab_block(entry->next)->len + sizeof(slab_list_entry_t);
        list_del(next);
    }
    // 合并前面的
    if (entry->prev != list &&
        list_slab_block(entry->prev)->allocated == SLAB_UNUSED) {
        slab_list_entry_t *prev = entry->prev;
        list_slab_block(prev)->len +=
            (list_slab_block(entry)->len + sizeof(slab_list_entry_t));
        list_del(entry);
    }
    return;
}

// 寻找长度符合的项
slab_list_entry_t *SLAB::find_entry(size_t len) {
    slab_list_entry_t *entry = slab_list;
    do {
        // 查找符合长度且未使用，符合对齐要求的内存
        if ((list_slab_block(entry)->len >= len) &&
            (list_slab_block(entry)->allocated == SLAB_UNUSED)) {
            // 进行分割，这个函数会同时设置 entry 的信息
            slab_split(entry, len);
            return entry;
        }
    } while ((entry = list_next(entry)) != slab_list);
    return (slab_list_entry_t *)NULL;
}

void *SLAB::alloc(size_t byte) {
    // 所有申请的内存长度(限制最小大小)加上管理头的长度
    size_t             len   = (byte > SLAB_MIN) ? byte : SLAB_MIN;
    slab_list_entry_t *entry = find_entry(len);
    if (entry != NULL) {
        set_used(entry);
        return (void *)((uint8_t *)entry + sizeof(slab_list_entry_t));
    }
    entry = list_prev(slab_list);
    // 如果执行到这里，说明没有可用空间了，那么申请新的内存页
    len += sizeof(slab_list_entry_t);
    size_t pages = (len % PMM_PAGE_SIZE == 0) ? (len / PMM_PAGE_SIZE)
                                              : ((len / PMM_PAGE_SIZE) + 1);
    slab_list_entry_t *new_entry = (slab_list_entry_t *)pmm.alloc_page(pages);
    if (new_entry == NULL) {
        io.printf("Error at slab.c void *alloc(): no enough physical memory\n");
        return NULL;
    }
    list_init(new_entry);
    // 新表项的可用长度为减去头的大小
    list_slab_block(new_entry)->len =
        pages * PMM_PAGE_SIZE - sizeof(slab_list_entry_t);
    list_add_after(entry, new_entry);
    // 进行分割
    slab_split(new_entry, len);
    set_used(new_entry);
    return (void *)((uint8_t *)new_entry + sizeof(slab_list_entry_t));
}

void SLAB::free(void *addr) {
    // 获取实际开始地址
    slab_list_entry_t *entry =
        (slab_list_entry_t *)((uint8_t *)addr - sizeof(slab_list_entry_t));
    if (list_slab_block(entry)->allocated != SLAB_USED) {
        io.printf("Error at slab.c void free(void *)\n");
        return;
    }
    list_slab_block(entry)->allocated = SLAB_UNUSED;
    slab_merge(entry);
    return;
}

// TODO: 这两个数据没有进行处理
size_t SLAB::get_total(void) {
    return heap_total;
}

size_t SLAB::get_free(void) {
    return heap_free;
}
