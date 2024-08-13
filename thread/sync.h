#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include "list.h"
#include "stdint.h"
#include "thread.h"

// 信号量   
struct semaphore {
    uint8_t value;          // 信号量
    struct list waiters;    // 等待锁的线程队列
};

struct lock {
    struct task_struct* holder;     // 锁的持有者
    struct semaphore semaphore;     // 用二元信号量实现锁
    uint32_t holder_repeat_nr;      // 锁的持有者重复申请锁的次数   作用: ??? 
};

void sema_init(struct semaphore* psema, uint8_t value);
void lock_init(struct lock* plock);
void sema_down(struct semaphore* psema);
void sema_up(struct semaphore* psema);
void lock_acquire(struct lock* plock);
void lock_release(struct lock* plock);

#endif