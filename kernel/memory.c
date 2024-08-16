#include "memory.h"
#include "bitmap.h"
#include "stdint.h"
#include "print.h" 
#include "global.h"
#include "string.h" 
#include "debug.h" 
#include "sync.h"
#include "thread.h"



#define PG_SIZE 4096    // 页尺寸 4kb

/**************位图地址*************
*   0xc009f000 是内核主线程地址, 0xc009e000是内核主线程的pcb
*   一个页框大小的位图可表示128MB内存, 位图位置安排在地址0xc009a000
*   本系统最大支持四个页框的位图, 512MB     
***********************************/
#define MEM_BITMAP_BASE 0xc009a000

/* 0xc0000000 是内核虚拟地址3G起, 0x100000意为跨国低端1MB内存, 使虚拟地址在逻辑上连续 */
#define K_HEAP_START 0xc0100000     // 堆的起始地址

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)   // 获取虚拟地址的高10位
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)   // 获取虚拟地址的中间10位

/* 内存池结构, 生产两个实例用于管理内核内存池和用户内存池 */
struct pool {
    struct bitmap pool_bitmap;  // 本内存池用到的位图结构, 用于管理物理内存
    uint32_t phy_addr_start;    // 本内存池所管理的物理地址起始位置
    uint32_t pool_size;         // 本内存池的字节容量
    struct lock lock;           // 申请内存的锁, 保护内存分配
};

struct pool kernel_pool, user_pool;     // 内核内存池和用户内存池
struct virtual_addr kernel_vaddr;       // 此结构用来给内核分配虚拟地址

/*  在pf表示的虚拟内存池中申请pg_cnt个虚拟页 
*  成功返回虚拟页的起始地址, 失败返回NULL   */
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
    int vaddr_start = 0, bit_idx_start = -1;
    uint32_t cnt = 0;
    if ( pf == PF_KERNEL) { // 内核内存分配
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1) {
            return NULL;
        }
        while(cnt < pg_cnt) {
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        }
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start*PG_SIZE;
    } else {    // 用户内存分配
        struct task_struct* cur = running_thread();
        bit_idx_start = bitmap_scan(&cur->userprog_vaddr.vaddr_bitmap, pg_cnt);
        if(bit_idx_start==-1) return NULL;
        while(cnt < pg_cnt){
            bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        }
        vaddr_start = cur->userprog_vaddr.vaddr_start + bit_idx_start * PG_SIZE;

        ASSERT((uint32_t)vaddr_start < (0xc0000000 - PG_SIZE));
    }
    return (void*)vaddr_start;
}

/*  得到虚拟地址vaddr对应的pte指针  */
uint32_t* pte_ptr(uint32_t vaddr) {
    // 先访问到页表自己 + 再用页目录项pde作为pte的索引访问到页表 + 再用pte的索引
    uint32_t* pte = (uint32_t*)(0xffc00000 + \
        ((vaddr & 0xffc00000) >> 10) + \
        PTE_IDX(vaddr) * 4);
    return pte;
}

/*  得到虚拟地址vaddr对应的pde指针  */
uint32_t* pde_ptr(uint32_t vaddr) {
    // 0xfffff000 访问到页表本身所在的地址
    uint32_t* pde = (uint32_t*) ((0xfffff000) + PDE_IDX(vaddr) * 4);
    return pde;
}

/*  在m_pool指向的物理内存池中分配一个物理页    成功返回页框的物理地址, 失败返回NULL*/
static void* palloc(struct pool* m_pool) {
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1); // 找一个物理界面
    if (bit_idx == -1) {
        return NULL;
    }
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = ((bit_idx*PG_SIZE) + m_pool->phy_addr_start);
    return (void* ) page_phyaddr;
} 

static void page_table_add(void* _vaddr, void* _page_phyaddr) {

    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t* pde = pde_ptr(vaddr) ;
    uint32_t* pte = pte_ptr(vaddr);

    if(*pde&0x00000001) {    // 判断p位: 是否存在
        ASSERT(!(*pte & 0x00000001));
        if(!(*pte & 0x00000001)) {
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        } else {
            PANIC("pte repeat");
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    }
    else {
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);

        memset((void*)((int)pte&0xfffff000), 0, PG_SIZE );
        ASSERT(!(*pte & 0x00000001));
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
    
    // put_str("vaddr:");
    // put_int((uint32_t)_vaddr);
    // put_str("\n");

    // put_str("page_phyaddr:");
    // put_int((uint32_t)_page_phyaddr);
    // put_str("\n");

}

void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
    ASSERT(pg_cnt>0 && pg_cnt<3840);

    void* vaddr_start = vaddr_get(pf, pg_cnt);  // 获取虚拟内存空间
    if(vaddr_start == NULL) {
        return NULL;
    }
    uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

    while(cnt-- > 0) {  // 分配对应次数
        void* page_phyaddr = palloc(mem_pool);  // 获得物理内存空间
        if(page_phyaddr == NULL) {  // 如果实现失败, 要将内存都释放回去
            // 实现内存回收: ---
            return NULL;
        }
        page_table_add((void*)vaddr, page_phyaddr); // 空间映射
        vaddr += PG_SIZE;
    }
    return vaddr_start;
}

void* get_kernel_pages(uint32_t pg_cnt) {
    void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if(vaddr!=NULL){
        memset(vaddr, 0, pg_cnt*PG_SIZE);
    }
    return vaddr;
}

/* 用户内存申请4k内存, 并返回其虚拟地址 */
void* get_user_pages(uint32_t pg_cnt) {
    lock_acquire(&user_pool.lock);
    void* vaddr = malloc_page(PF_USER, pg_cnt);
    memset(vaddr, 0, pg_cnt * PG_SIZE);
    lock_release(&user_pool.lock);
    return vaddr;
}

/* 将地址vaddr 与内存池中的一页相关联, 仅支持一页内存分配*/
void* get_a_page(enum pool_flags pf, uint32_t vaddr) {
    struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
    lock_acquire(&mem_pool->lock);
    struct task_struct* cur = running_thread();
    int32_t bit_idx = -1;

    if(cur->pgdir != NULL && pf==PF_USER){  // 进程分配内存
        bit_idx = (vaddr - cur->userprog_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&cur->userprog_vaddr.vaddr_bitmap, bit_idx, 1);  // 将占用的内存置为1

    } else if (cur->pgdir==NULL && pf==PF_KERNEL){  // 内核分配内存
        bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
        ASSERT(bit_idx > 0);
        bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);
    } else {
        PANIC("get_a_page: not allow kernel alloc userspace or \
            user alloc kernelspace by get_a_page");
    }

    void* page_phyaddr = palloc(mem_pool);
    if(page_phyaddr==NULL) return NULL;
    page_table_add((void*)vaddr, page_phyaddr);
    lock_release(&mem_pool->lock);
    return (void*)vaddr;
}

/* 将虚拟地址转换为物理地址*/
uint32_t addr_v2p(uint32_t vaddr) {
    uint32_t* pte = pte_ptr(vaddr);
    // pte为页表所在的物理页框地址, 去掉低12位的页表项属性+虚拟地址的低12位
    return ((*pte & 0xfffff000) + (vaddr & 0x00000fff)); 
}

/* 初始化内存池 */
static void mem_pool_init(uint32_t all_mem) {
    put_str(" mem_pool_init start\n");
    // 页表大小: 1页的页目录表 + 第0和第768个页目录项指向同一个页表 +
    //  第769~1022个页目录项 共指向254个页表, 共256个页框
    uint32_t page_table_size = PG_SIZE * 256;

    uint32_t used_mem = page_table_size + 0x100000; //0x100000为低端1MB内存
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;   // 一页为4KB 不管内存是否为4k的倍数

    // 计算空闲的图的数量
    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    // bitmap 的长度, 位图中一位表示一页, 以字节为单位
    uint32_t kbm_length = kernel_free_pages / 8;
    uint32_t ubm_length = user_free_pages / 8;

    // 内存池的起始地址
    uint32_t kp_start = used_mem;
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

    /****赋值*****/
    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;

    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;
    /*************/

    kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
    user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_length);

    /* 输出内存池信息 */
    
    put_str("   kernel_pool_bitmap_start:");
    put_int((int)kernel_pool.pool_bitmap.bits); // 地址转换为int
    put_str("   kernel_pool_phy_addr_start:");
    put_int(kernel_pool.phy_addr_start);
            put_str("\n");
            put_str("   user_pool_bitmap_start:");
            put_int((int)user_pool.pool_bitmap.bits); // 地址转换为int
            put_str("     user_pool_phy_addr_start:");
            put_int(user_pool.phy_addr_start);
            put_str("\n");

            bitmap_init(&kernel_pool.pool_bitmap);
        bitmap_init(&user_pool.pool_bitmap);

    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;

    kernel_vaddr.vaddr_bitmap.bits = (void*) (MEM_BITMAP_BASE + kbm_length + ubm_length);

    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);

    lock_init(&kernel_pool.lock);
    lock_init(&user_pool.lock);

    put_str(" mem_pool_init done\n");
}

void mem_init() {
    put_str("mem_init start\n");
    uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));
    mem_pool_init(mem_bytes_total);
    put_str("mem_init done\n");
}

