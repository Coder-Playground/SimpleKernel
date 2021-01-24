
// This file is a part of Simple-XX/SimpleKernel
// (https://github.com/Simple-XX/SimpleKernel).
//
// heap.h for Simple-XX/SimpleKernel.

#ifndef _HEAP_H_
#define _HEAP_H_

#include "stdint.h"
#include "stddef.h"
#include "slab.h"

class HEAP {
private:
    // 堆最大容量 4MB
    static constexpr const uint32_t HEAP_MAX_SIZE = 0x400000;
    // 管理算法的名称
    const char *name;
    SLAB        slab;

protected:
public:
    HEAP(PMM &pmm);
    ~HEAP(void);
    // 初始化
    int32_t init(void);
    //
    int32_t manage_init(void);
    // 内存申请，单位为 Byte
    void *malloc(size_t byte);
    // 内存释放
    void free(void *p);
    // 获取管理的内存大小，包括管理信息
    size_t get_total(void);
    // 获取空闲内存数量 单位为 byte
    size_t get_free(void);
};

#endif /* _HEAP_H_ */
