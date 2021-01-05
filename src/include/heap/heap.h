
// This file is a part of Simple-XX/SimpleKernel
// (https://github.com/Simple-XX/SimpleKernel).
//
// heap.h for Simple-XX/SimpleKernel.

#ifndef _HEAP_H_
#define _HEAP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stddef.h"
#include "pmm.h"

// 堆最大容量 4MB
#define HEAP_MAX_SIZE (0x400000UL)

// 堆管理结构体
typedef struct heap_manage {
    // 管理算法的名称
    const char *name;
    // 初始化
    void (*heap_manage_init)();
    // 内存申请，单位为 Byte，align 为对齐大小
    void *(*heap_manage_malloc)(size_t byte);
    // 释放内存
    void (*heap_manage_free)(void *addr);
    // 获取当前管理的内存页数
    size_t (*heap_manage_get_total)(void);
    // 获取空闲内存大小
    size_t (*heap_manage_get_free)(void);
} heap_manage_t;

// 初始化堆
void heap_init(void);

// 内存申请，单位为 Byte
void *kmalloc(size_t byte);

// 内存释放
void kfree(void *p);

// 获取管理的内存大小，包括管理信息
size_t heap_get_total(void);

// 获取空闲内存数量 单位为 byte
size_t heap_get_free(void);

#ifdef __cplusplus
}
#endif

#endif /* _HEAP_H_ */
