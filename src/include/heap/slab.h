
// This file is a part of Simple-XX/SimpleKernel
// (https://github.com/Simple-XX/SimpleKernel).
//
// slab.h for Simple-XX/SimpleKernel.

#ifndef _SLAB_H_
#define _SLAB_H_

#include "pmm.h"

typedef struct slab_block {
    // 该内存块是否已经被申请
    size_t allocated;
    // 当前内存块的长度，不包括头长度
    size_t len;
} slab_block_t;

// 一个仅在这里使用的简单循环链表
typedef struct slab_list_entry {
    slab_block_t            slab_block;
    struct slab_list_entry *next;
    struct slab_list_entry *prev;
} slab_list_entry_t;

static constexpr const uint32_t SLAB_USED   = 0x00;
static constexpr const uint32_t SLAB_UNUSED = 0x01;

class SLAB {
private:
    // 最小空间
    static constexpr const uint32_t SLAB_MIN = 0xFF;

    // 管理的内存起始地址，包括头的位置
    void *addr_start;
    // 管理内存结束地址
    void *addr_end;
    // 堆管理的内存大小 单位为 bytes
    size_t heap_total;
    // 堆所有空闲内存大小 单位为 bytes
    size_t heap_free;
    // 堆中 block 的数量
    size_t block_count;
    // 堆节点链表
    slab_list_entry_t *slab_list;
    PMM &              pmm;

    slab_list_entry_t *slab_split(slab_list_entry_t *entry, size_t len);
    void               slab_merge(slab_list_entry_t *list);
    slab_list_entry_t *find_entry(size_t len);

protected:
public:
    SLAB(PMM &pmm);
    ~SLAB(void);
    int32_t init(void);
    // 内存申请，单位为 Byte
    void *alloc(size_t byte);
    // 内存释放
    void free(void *p);
    // 获取管理的内存大小，包括管理信息
    size_t get_total(void);
    // 获取空闲内存数量 单位为 byte
    size_t get_free(void);
};

#endif /* _SLAB_H_ */
