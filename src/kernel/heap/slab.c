
// This file is a part of Simple-XX/SimpleKernel
// (https://github.com/Simple-XX/SimpleKernel).
//
// slab.c for Simple-XX/SimpleKernel.

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "stdint.h"
#include "string.h"
#include "assert.h"
#include "pmm.h"
#include "slab.h"

#define SLAB_USED (0x00)
#define SLAB_UNUSED (0x01)

// slab 管理的内存页数
#define SLAB_PAGE_COUNT (pheap_free_pages_count())
// 最小空间
#define SLAB_MIN (0xFF)

static void   init(void);
static void * alloc(size_t byte);
static void   free(void *addr);
static size_t get_total(void);
static size_t get_free(void);

heap_manage_t slab_manage = {"Slab", &init,      &alloc,
                             &free,  &get_total, &get_free};

// 管理信息
static slab_manage_t sb_manage;
// 初始化
static void list_init(list_entry_t *list) {
    list->next = list;
    list->prev = list;
    return;
}

// 在中间添加元素
static void list_add_middle(list_entry_t *prev, list_entry_t *next,
                            list_entry_t *new) {
    next->prev = new;
    new->next  = next;
    new->prev  = prev;
    prev->next = new;
}

// 在 prev 后添加项
static void list_add_after(list_entry_t *prev, list_entry_t *new) {
    list_add_middle(prev, prev->next, new);
    return;
}

// 删除元素
static void list_del(list_entry_t *list) {
    list->next->prev = list->prev;
    list->prev->next = list->next;
    return;
}

// 返回前面的元素
static list_entry_t *list_prev(list_entry_t *list) {
    return list->prev;
}

// 返回后面的的元素
static list_entry_t *list_next(list_entry_t *list) {
    return list->next;
}

// 返回 chunk_info
static slab_block_t *list_slab_block(list_entry_t *list) {
    return &(list->slab_block);
}

// 将 entry 设置为已使用
static void set_used(list_entry_t *entry) {
    list_slab_block(entry)->allocated = SLAB_USED;
    return;
}

// 将 entry 设置为未使用
static void set_unused(list_entry_t *entry) {
    list_slab_block(entry)->allocated = SLAB_UNUSED;
    return;
}

void init(void) {
    // 设置第一块内存的信息
    // 首先给链表中添加一个大小为 1 页的块
    list_entry_t *sb_list = (list_entry_t *)pmm_alloc_page(1);
    bzero((void *)sb_list, PMM_PAGE_SIZE);
    // 填充管理信息
    sb_manage.addr_start  = NULL;
    sb_manage.addr_end    = NULL;
    sb_manage.heap_total  = PMM_PAGE_SIZE;
    sb_manage.heap_free   = PMM_PAGE_SIZE - sizeof(list_entry_t);
    sb_manage.block_count = 1;
    sb_manage.slab_list   = sb_list;
    list_init(sb_manage.slab_list);
    // 设置第一块内存的相关信息
    list_slab_block(sb_manage.slab_list)->allocated = SLAB_UNUSED;
    list_slab_block(sb_manage.slab_list)->len =
        PMM_PAGE_SIZE - sizeof(list_entry_t);
    printk_info("slab init.\n");
    return;
}

// 切分内存块，len 为调用者申请的大小，不包括头大小
// 返回分割出来的管理头地址
static list_entry_t *slab_split(list_entry_t *entry, size_t len) {
    // 如果剩余内存大于内存头的长度+设定的最小长度
    if ((list_slab_block(entry)->len - len > sizeof(list_entry_t) + SLAB_MIN)) {
        // 添加新的链表项，位于旧表项开始地址+旧表项长度
        list_entry_t *new_entry =
            (list_entry_t *)((void *)entry + sizeof(list_entry_t) + len);

        bzero((void *)new_entry, list_slab_block(entry)->len - len);
        list_init(new_entry);
        // 新表项的长度为：list->len（总大小）- 头大小 - 要求分割的大小
        list_slab_block(new_entry)->len =
            list_slab_block(entry)->len - len - sizeof(list_entry_t);
        set_unused(new_entry);
        list_add_after(entry, new_entry);
        // 重新设置旧链表信息
        list_slab_block(entry)->len = len;
        return new_entry;
    }
    return (list_entry_t *)NULL;
}

// 合并内存块
static void slab_merge(list_entry_t *list) {
    list_entry_t *entry = list;
    // 合并后面的
    if (entry->next != list &&
        list_slab_block(entry->next)->allocated == SLAB_UNUSED) {
        list_entry_t *next = entry->next;
        list_slab_block(entry)->len +=
            list_slab_block(entry->next)->len + sizeof(list_entry_t);
        list_del(next);
    }
    // 合并前面的
    if (entry->prev != list &&
        list_slab_block(entry->prev)->allocated == SLAB_UNUSED) {
        list_entry_t *prev = entry->prev;
        list_slab_block(prev)->len +=
            (list_slab_block(entry)->len + sizeof(list_entry_t));
        list_del(entry);
    }
    return;
}

// 寻找长度符合的项
static list_entry_t *find_entry(size_t len) {
    list_entry_t *entry = sb_manage.slab_list;
    do {
        // 查找符合长度且未使用，符合对齐要求的内存
        if ((list_slab_block(entry)->len >= len) &&
            (list_slab_block(entry)->allocated == SLAB_UNUSED)) {
            // 进行分割，这个函数会同时设置 entry 的信息
            slab_split(entry, len);
            return entry;
        }
    } while ((entry = list_next(entry)) != sb_manage.slab_list);
    return (list_entry_t *)NULL;
}

void *alloc(size_t byte) {
    // 所有申请的内存长度(限制最小大小)加上管理头的长度
    size_t        len   = (byte > SLAB_MIN) ? byte : SLAB_MIN;
    list_entry_t *entry = find_entry(len);
    if (entry != NULL) {
        set_used(entry);
        return ((void *)entry + sizeof(list_entry_t));
    }
    entry = list_prev(sb_manage.slab_list);
    // 如果执行到这里，说明没有可用空间了，那么申请新的内存页
    len += sizeof(list_entry_t);
    size_t        pages     = (len % PMM_PAGE_SIZE == 0) ? (len / PMM_PAGE_SIZE)
                                                         : ((len / PMM_PAGE_SIZE) + 1);
    list_entry_t *new_entry = (list_entry_t *)pmm_alloc_page(pages);
    if (new_entry == NULL) {
        printk_err("Error at slab.c void *alloc(): no enough physical "
                   "memory\n");
        return NULL;
    }
    list_init(new_entry);
    // 新表项的可用长度为减去头的大小
    list_slab_block(new_entry)->len =
        pages * PMM_PAGE_SIZE - sizeof(list_entry_t);
    list_add_after(entry, new_entry);
    // 进行分割
    slab_split(new_entry, len);
    set_used(new_entry);
    return ((void *)new_entry + sizeof(list_entry_t));
}

void free(void *addr) {
    // 获取实际开始地址
    list_entry_t *entry = (list_entry_t *)(addr - sizeof(list_entry_t));
    if (list_slab_block(entry)->allocated != SLAB_USED) {
        printk_err("Error at slab.c void free(void *)\n");
        return;
    }
    list_slab_block(entry)->allocated = SLAB_UNUSED;
    slab_merge(entry);
    return;
}

// TODO: 这两个数据没有进行处理
size_t get_total(void) {
    return sb_manage.heap_total;
}

size_t get_free(void) {
    return sb_manage.heap_free;
}

#ifdef __cplusplus
}
#endif
