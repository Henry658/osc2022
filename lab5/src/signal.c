#include "signal.h"
#include "syscall.h"
#include "sched.h"
#include "malloc.h"

void check_signal()
{
    lock();
    if(curr_thread->signal_is_checking)return;
    //prevent nested running signal handler
    curr_thread->signal_is_checking = 1;
    unlock();
    for (int i = 0; i <= SIGNAL_MAX; i++)
    {
        store_context(&curr_thread->signal_saved_context);
        if(curr_thread->sigcount[i]>0)
        {
            lock();
            curr_thread->sigcount[i]--;
            unlock();
            run_signal(i);
        }
    }
    lock();
    curr_thread->signal_is_checking = 0;
    unlock();
}

void run_signal(int signal)
{
    curr_thread->curr_signal_handler = curr_thread->singal_handler[signal];

    //run default handler in kernel
    if (curr_thread->curr_signal_handler == signal_default_handler)
    {
        signal_default_handler();
        return;
    }

    char *temp_signal_userstack = kmalloc(USTACK_SIZE);

    asm("msr elr_el1, %0\n\t"
        "msr sp_el0, %1\n\t"
        "eret\n\t" ::"r"(signal_handler_wrapper),
        "r"(temp_signal_userstack + USTACK_SIZE));

}

void signal_handler_wrapper()
{
    (curr_thread->curr_signal_handler)();
    //system call sigreturn
    asm("mov x8,50\n\t"
        "svc 0\n\t");
}

void signal_default_handler()
{
    kill(0,curr_thread->pid);
}