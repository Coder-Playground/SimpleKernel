
// This file is a part of SimpleXX/SimpleKernel
// (https://github.com/SimpleXX/SimpleKernel).
//
// vmm.c for SimpleXX/SimpleKernel.

#ifdef __cplusplus
extern "C" {
#endif

#include "stdio.h"
#include "string.h"
#include "cpu.hpp"
#include "vmm.h"

page_dir_t curr_dir = NULL;

// 内核页目录区域，内容为页表
page_dir_t pgd_kernel = NULL;

// 内核页表区域，内容为页表项
// page_table_entry_t
// pte_kernel[VMM_PAGE_TABLES_KERNEL][VMM_PAGES_PRE_PAGE_TABLE]
//     __attribute__((aligned(VMM_PAGE_SIZE)));
page_table_t *pte_kernel = NULL;

// TODO: 完善缺页处理
void page_fault(pt_regs_t *pt_regs) {
    addr_t pg_addr = 0x00;
#ifdef __x86_64__
    __asm__ volatile("movq %%cr2,%0" : "=r"(pg_addr));
#else
    __asm__ volatile("mov %%cr2,%0" : "=r"(pg_addr));
#endif
    printk("Page fault at 0x%X, virtual faulting address 0x%X\n", pt_regs->eip,
           pg_addr);
    printk_err("Error code: 0x%X\n", pt_regs->err_code);

    // bit 0 为 0 指页面不存在内存里
    if ((pt_regs->err_code & 0x1) == 0x1) {
        printk_err("Page wasn't present.\n");
    }
    // bit 1 为 0 表示读错误，为 1 为写错误
    if ((pt_regs->err_code & 0x2) == 0x2) {
        printk_err("Write error.\n");
    }
    else {
        printk_err("Read error.\n");
    }
    // bit 2 为 0 是在内核模式打断的，为 1 表示在用户模式打断的，
    if ((pt_regs->err_code & 0x4) == 0x4) {
        printk_err("In user mode.\n");
    }
    else {
        printk_err("In kernel mode.\n");
    }
    // bit 3 为 1 表示错误是由保留位覆盖造成的
    if ((pt_regs->err_code & 0x8) == 0x8) {
        printk_err("Reserved bits being overwritten.\n");
    }
    // bit 4 为 1 表示错误发生在取指令的时候
    if ((pt_regs->err_code & 0x10) == 0x10) {
        printk_err("The fault occurred during an instruction fetch.\n");
    }
    while (1) {
        ;
    }
    return;
}

// 执行完成后，开启虚拟内存
// 内核可访问所有内存
void vmm_init(void) {
    register_interrupt_handler(INT_PAGE_FAULT, &page_fault);
    // TODO: 将物理地址前 0～1GB 映射到虚拟地址 0xC0000000～0xF0000000
    addr_t skip = pmm_alloc_page(1);
    printk_debug("skip: 0x%X\n", skip);
    pgd_kernel = (page_dir_t)pmm_alloc_page(1);
    printk_debug("pgd_kernel: 0x%X\n", pgd_kernel);
    pte_kernel = (page_table_t *)pmm_alloc_page(8);
    printk_debug("pte_kernel: 0x%X\n", pte_kernel[1023]);
    printk_debug("pgd_kernel[0] = 0x%X\n", (addr_t)pte_kernel[0] |
                                               VMM_PAGE_PRESENT | VMM_PAGE_RW |
                                               VMM_PAGE_KERNEL);
    printk_debug("pgd_kernel[1] = 0x%X\n", (addr_t)pte_kernel[1] |
                                               VMM_PAGE_PRESENT | VMM_PAGE_RW |
                                               VMM_PAGE_KERNEL);
    for (uint32_t i = VMM_PGD_INDEX(KERNEL_BASE), j = 0;
         j < VMM_PAGE_TABLES_KERNEL; i++, j++) {
        pgd_kernel[i] = ((addr_t)&pte_kernel[VMM_PAGES_PRE_PAGE_TABLE * j]) |
                        VMM_PAGE_PRESENT | VMM_PAGE_RW | VMM_PAGE_KERNEL;
    }
    for (uint32_t i = 0; i < VMM_PAGES_KERNEL; i++) {
        pte_kernel[i] =
            (i << 12) | VMM_PAGE_PRESENT | VMM_PAGE_RW | VMM_PAGE_KERNEL;
    }
    set_pgd(pgd_kernel);
    enable_page();

    printk_info("vmm_init\n");
    return;
}

void enable_page(void) {
    uint32_t cr3 = 0;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    if (cr3 == NULL) {
        printk_err("cr3 not set!\n");
    }
    uint32_t cr0 = 0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    // 最高位 PG 位置 1，分页开启
    cr0 |= (1u << 31);
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0));
    return;
}

void set_pgd(page_dir_t pgd) {
    curr_dir = pgd;
    __asm__ volatile("mov %0, %%cr3" : : "r"(pgd));
    return;
}

#ifdef __cplusplus
}
#endif
