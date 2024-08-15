#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "list.h"
#include "memory.h"
#include "stdint.h"

/* 自定义通用函数类型 */
typedef void thread_func(void*);

/* 进程或线程的状态 */
enum task_status {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_HANGING,
    TASK_DIED
};

/* ***************中断栈intr_stack***************** 
 * 此结构用于中断发生时保护程序的上下文环境
 * 进程或线程被外部中断或软中断打断时, 按照此结构压入上下文
 * 寄存器, intr_exit 中的出栈操作是此结构的逆操作
 * 此栈在线程自己的内核栈中位置固定, 位于页的最下端
 * *********************************************** */
struct intr_stack {
    uint32_t vec_no;  // 中断号
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp_dummy; // pushad 会把esp压入, 但是esp是不断变化的, 所以popad自动忽略esp
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;

    /* 由cpu从低特权级压入高特权级时压入 */
    uint32_t err_code;
    void(*eip) (void);
    uint32_t cs;
    uint32_t eflags;
    void* esp;
    uint32_t ss;
};

/* ****************线程栈 thread_stack*************** 
 * 线程自己的栈, 用于存储线程中待执行的函数
 * 此结构在线程中的位置不固定
 * 仅用在switch_to 时保存线程环境
 * 实际位置取决于实际运行情况
 * ************************************************* */
struct thread_stack {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    /* 线程第一次执行, eip指向待调用的函数kernel_thread     其他时候指向 switch_to 的返回地址 */
    void (*eip) (thread_func* func, void* func_arg);

    /* 一下仅供第一次被调度上cpu时调用 */

    void (*unused_retaddr);     // 占位充数为返回地址
    thread_func* function;      // 由kernel_thread 调用的函数名
    void* func_arg;
};

struct task_struct {
    uint32_t* self_kstack;      // 各内核线程都有自己的内核栈 
    enum task_status status;    
    uint8_t priority;           // 优先级
    char name[16];
    uint32_t ticks;             // 每次在处理器上执行时间的滴答数
    uint32_t elapsed_ticks;     // 执行了多少个滴答数, 即执行了多久
    struct list_elem general_tag;  // 用于线程在一般的队列中的结点
    struct list_elem all_list_tag; // 用于线程队列中的节点

    uint32_t* pgdir;            // 进程自己页表的虚拟地址
    struct virtual_addr userprog_vaddr; // 用户进程的虚拟地址

    uint32_t stack_magic;       // 栈的边界标记, 用于检测栈的溢出
};

extern struct list thread_ready_list;          // 就绪队列
extern  struct list thread_all_list;            // 所有任务队列

//static void kernel_thread(thread_func*, void*);
void thread_create(struct task_struct*, thread_func, void*); 
void init_thread(struct task_struct*, char*, int);
struct task_struct* thread_start(char*, int, thread_func, void*);
struct task_struct* running_thread();
void schedule();
void thread_init();
void thread_block(enum task_status stat);
void thread_unblock(struct task_struct* pthread);

#endif
