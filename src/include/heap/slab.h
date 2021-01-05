
// This file is a part of Simple-XX/SimpleKernel
// (https://github.com/Simple-XX/SimpleKernel).
//
// slab.h for Simple-XX/SimpleKernel.

#ifndef _SLAB_H_
#define _SLAB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "heap.h"

typedef struct slab_block {
    // 该内存块是否已经被申请
    size_t allocated;
    // 当前内存块的长度，不包括头长度
    size_t len;
} slab_block_t;

// 一个仅在这里使用的简单循环链表
typedef struct list_entry {
    slab_block_t       slab_block;
    struct list_entry *next;
    struct list_entry *prev;
} list_entry_t;

// 管理结构
typedef struct slab_manage {
    // 管理的内存起始地址，包括头的位置
    void *addr_start;
    // 管理内存结束地址
    void *addr_end;
    // 堆管理的内存大小
    size_t heap_total;
    // 堆所有空闲内存大小
    size_t heap_free;
    // 堆中 block 的数量
    size_t block_count;
    // 堆节点链表
    list_entry_t *slab_list;
} slab_manage_t;

// 用于管理堆物理地址
extern heap_manage_t slab_manage;

#ifdef __cplusplus
}
#endif

#endif /* _SLAB_H_ */
