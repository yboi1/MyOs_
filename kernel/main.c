#include "ioqueue.h"
#include "keyboard.h"
#include "list.h"
#include "memory.h"
#include "print.h"
#include "init.h"
#include "debug.h"
#include "process.h"
#include "stdint.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);
int test_var_a = 0, test_var_b = 0;

int main(void) {    
    put_str("hello kernel...\n");
    init_all();

    thread_start("consumer_a", 8, k_thread_a, "  A_");
    thread_start("consumer_b", 31, k_thread_b, "  B_");
    // thread_start("consumer_b", 31, k_thread_b, "  B_");


    process_execute(u_prog_a, "user_prog_a");
    process_execute(u_prog_b, "user_prog_b");

    intr_enable();

    while(1);

    return 0;
}

void k_thread_a(void* arg) {
    while(1) {
            intr_disable();
            console_put_str("v_a:0x");
            console_put_int(test_var_a);
            console_put_str(" ");
            intr_enable();
        
    }
}

void k_thread_b(void* arg) {
    while(1) {
        intr_disable();
        console_put_str("v_b:0x");
        console_put_int(test_var_b);
        console_put_str(" ");
        intr_enable();
    }
}


void u_prog_a(void){
    put_str("start _prog_a...\n");
    while(1){
        test_var_a++;
    }
}

void u_prog_b(void){
    put_str("start _prog_b...\n");
    while(1){
        test_var_b++;
    }
}
