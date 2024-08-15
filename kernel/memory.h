#ifndef __MEMORY_KERNEL_H
#define __MEMORY_KERNEL_H
#include "stdint.h" 
#include "bitmap.h"

struct virtual_addr {
    struct bitmap vaddr_bitmap;
    uint32_t vaddr_start;
};

/*  内存池标记, 用于标记内存池的类型 */
enum pool_flags {
    PF_KERNEL = 1,
    PF_USER = 2
};

#define PG_P_1  1   // 页表项或页目录项存在属性位
#define PG_P_0  0   
#define PG_RW_R 0   // rw属性位值, 读/执行
#define PG_RW_W 2   // 读/写/执行
#define PG_US_S 0   // U/S 属性位值, 系统级
#define PG_US_U 4   // U/S 属性位置, 用户级

extern struct pool kernel_pool, user_pool;
void mem_init(void);
void* get_kernel_pages(uint32_t);
void* get_user_pages(uint32_t pg_cnt);
void* get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);

#endif 