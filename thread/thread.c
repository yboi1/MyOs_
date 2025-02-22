#include "thread.h"
#include "interrupt.h"
#include "list.h"
#include "print.h"
#include "stdint.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "debug.h"
#include "process.h"
#include "sync.h"

#define PG_SIZE 4096

struct task_struct* main_thread;        // 主线程pcb
struct list thread_ready_list;          // 就绪队列
struct list thread_all_list;            // 所有任务队列
static struct list_elem* thread_tag;    // 保存队列中的线程节点 ?????
struct lock pid_lock;

extern void switch_to(struct task_struct* cur, struct task_struct* next);

/* 获取当前线程pcb指针 --结构体地址*/
struct task_struct* running_thread() {
    uint32_t esp;
    asm ("mov %%esp, %0" : "=g"(esp));
    return (struct task_struct*)(esp & 0xfffff000);
}

void thread_funca(){

}
/* 由kernel_thread 去执行function(func_arg) */
static void kernel_thread(thread_func* function, void* func_arg){
    // 避免后面的时钟中断被屏蔽, 而无法调度其他线程
    intr_enable();
    struct task_struct* thread = running_thread();
    function(func_arg);
}

/* 分配pid */
static pid_t allocate_pid(void) {
    static pid_t next_pid = 0;
    lock_acquire(&pid_lock);
    next_pid++;
    lock_release(&pid_lock);
    return next_pid;
}

/* 初始化线程栈thread_stack
将待执行的函数和参数放到thread_stack 中对应的位置 */
void thread_create(struct task_struct* pthread, thread_func function, void* func_arg) {
    // 留出中断使用栈的空间
    pthread->self_kstack -= sizeof(struct intr_stack);
    // 留出线程栈空间
    pthread->self_kstack -= sizeof(struct thread_stack);
    struct thread_stack* kthread_stack = (struct thread_stack*)pthread->self_kstack;
    kthread_stack->eip = kernel_thread;
    kthread_stack->function = function;
    kthread_stack->func_arg = func_arg;
    kthread_stack->ebp = kthread_stack->ebx = \
    kthread_stack->esi = kthread_stack->edi = 0;
}

// 初始化线程的基本信息
void init_thread(struct task_struct* pthread, char* name, int prio) {
    memset(pthread, 0, sizeof(*pthread));
    pthread->pid = allocate_pid();
    strcpy(pthread->name, name);

    if(pthread == main_thread) {
        pthread->status = TASK_RUNNING;
    } else {
        pthread->status = TASK_READY;
    }
    pthread->ticks = prio;
    pthread->elapsed_ticks = 0;

    pthread->priority = prio;
    //
    pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
    pthread->pgdir = NULL;
    pthread->stack_magic = 0x20040207;
}

struct task_struct* thread_start(char* name, int prio, thread_func function, void* func_arg) {
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, prio);
    thread_create(thread, function, func_arg);

    // 确保之前不在队列中
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);

    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);


    // asm volatile ("movl %0, %%esp; pop %%ebp; pop %%ebx; pop %%edi; pop %% esi; ret" 
    // : : "g"(thread->self_kstack) : "memory");
    return thread;
}

static void make_main_thread(void ) {
    main_thread = running_thread();
    init_thread(main_thread, "main", 31);

    ASSERT(!elem_find(&thread_all_list, &main_thread->all_list_tag));
    list_append(&thread_all_list, &main_thread->all_list_tag);
}

void dbgfunc(){
    return;
}
// 任务调度器
void schedule() {
    ASSERT(intr_get_status() == INTR_OFF ) ;
    struct task_struct* cur = running_thread();

    if(cur->status == TASK_RUNNING) {
        ASSERT(!elem_find(&thread_ready_list, &cur->general_tag));
        list_append(&thread_ready_list, &cur->general_tag);
        cur->ticks = cur->priority;
        cur->status = TASK_READY;
    } else {
        /* 需要等待其他事件的完成才能继续运行, 不需要加入队列, 当前线程不在就绪队列中 */
    }

    ASSERT(!list_empty(&thread_ready_list));

    thread_tag = NULL;
    thread_tag = list_pop(&thread_ready_list);
    struct task_struct* next = elem2enyrt(struct task_struct, general_tag, thread_tag);
    //struct task_struct* next = (struct task_struct*)((uint32_t)thread_tag & 0xfffff000);
    next->status = TASK_RUNNING;
    put_str(next->name);
    process_activate(next);
    //put_str("ok!\n\n");
    if(next->pgdir!=NULL) {
        dbgfunc();
    }
    switch_to(cur, next);
}

void thread_init(void) {
    put_str("thread_init start\n");
    list_init(&thread_all_list);
    list_init(&thread_ready_list);
    lock_init(&pid_lock);
    make_main_thread();
    put_str("thread_init done\n");
}

void thread_block(enum task_status stat) {
    /* 当状态为 BLOCKED, WAITING, HANGDING 时不会被调度*/
    ASSERT(((stat==TASK_WAITING) || (stat==TASK_BLOCKED) || (stat==TASK_HANGING)));
    enum intr_status old_status = intr_disable();
    struct task_struct* cur_thread = running_thread();
    cur_thread->status = stat;
    schedule();
    intr_set_status(old_status);
}

void thread_unblock(struct task_struct* pthread) {
    enum intr_status old_status = intr_disable();
    ASSERT((pthread->status==TASK_BLOCKED)||(pthread->status==TASK_HANGING)||(pthread->status==TASK_WAITING));
    if(pthread->status!=TASK_READY) {
        ASSERT(!elem_find(&thread_ready_list, &pthread->general_tag));
        if(elem_find(&thread_ready_list, &pthread->general_tag)) {
            PANIC("thread_unblock : blocked thread in ready_list!\n");
        }
        list_push(&thread_ready_list, &pthread->general_tag);
        pthread->status = TASK_READY;
    }
    intr_set_status(old_status);
}