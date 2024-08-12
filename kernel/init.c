#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "memory.h"
#include "timer.h"
#include "thread.h"


void init_all(){
    put_str("init_all\n");
    idt_init();
    mem_init();
    thread_init();
    timer_init();

}