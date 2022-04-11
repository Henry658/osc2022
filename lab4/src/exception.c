#include "uart.h"
#include "exception.h"
#include "timer.h"
#include "task.h"

static int timer_irq_count = 0;
void sync_64_router(unsigned long long x0)
{
    unsigned long long spsr_el1;
	__asm__ __volatile__("mrs %0, SPSR_EL1\n\t" : "=r" (spsr_el1));

    unsigned long long elr_el1;
	__asm__ __volatile__("mrs %0, ELR_EL1\n\t" : "=r" (elr_el1));

    unsigned long long esr_el1;
	__asm__ __volatile__("mrs %0, ESR_EL1\n\t" : "=r" (esr_el1));

    uart_printf("exception sync_el0_64_router -> spsr_el1 : 0x%x, elr_el1 : 0x%x, esr_el1 : 0x%x\r\n",spsr_el1,elr_el1,esr_el1);
}

void irq_router(unsigned long long x0){
    disable_interrupt();
    //uart_printf("ena : %d\r\n", is_disable_interrupt());

    //uart_printf("exception type: %x\n",x0);
    //uart_printf("irq_basic_pending: %x\n",*IRQ_BASIC_PENDING);
    //uart_printf("irq_pending_1: %x\n",*IRQ_PENDING_1);
    //uart_printf("irq_pending_2: %x\n",*IRQ_PENDING_2);
    //uart_printf("source : %x\n",*CORE0_INTERRUPT_SOURCE);

    //目前實測能從pending_1 AUX_INT, CORE0_INTERRUPT_SOURCE=GPU 辨別其他都是0(或再找)

    if (*IRQ_PENDING_1 & IRQ_PENDING_1_AUX_INT && *CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_GPU) // from aux && from GPU0 -> uart exception
    {
        //https://cs140e.sergio.bz/docs/BCM2837-ARM-Peripherals.pdf p13
        /*
        AUX_MU_IIR
        on read bits[2:1] :
        00 : No interrupts
        01 : Transmit holding register empty
        10 : Receiver holds valid byte
        11: <Not possible> 
        */

        // buffer read, write
        if (*AUX_MU_IIR & (0b01 << 1)) //can write
        {
            disable_mini_uart_w_interrupt(); // lab 3 : advanced 2 -> mask device line (enable by handler)
            add_task(uart_interrupt_w_handler, UART_IRQ_PRIORITY);
            run_preemptive_tasks();
            //uart_interrupt_w_handler();
        }
        else if (*AUX_MU_IIR & (0b10 << 1)) // can read
        {
            disable_mini_uart_r_interrupt(); // lab 3 : advanced 2 -> mask device line (enable by handler)
            add_task(uart_interrupt_r_handler, UART_IRQ_PRIORITY);
            run_preemptive_tasks();
            //uart_interrupt_r_handler();
        }
        else
        {
            uart_printf("uart handler error\r\n");
        }

    }else if(*CORE0_INTERRUPT_SOURCE & INTERRUPT_SOURCE_CNTPNSIRQ)  //from CNTPNS (core_timer)
    {
        timer_irq_count++;
        core_timer_disable();   // lab 3 : advanced 2 -> mask device line
        add_task(core_timer_handler, TIMER_IRQ_PRIORITY);
        run_preemptive_tasks();
        core_timer_handler();

        // prevent core_timer_disable been enable by previous irq handler
        disable_interrupt();
        timer_irq_count--;
        if (timer_irq_count==0)core_timer_enable(); // lab 3 : advanced 2 -> unmask device line
    }
}

void invalid_exception_router(unsigned long long x0){
    unsigned long long elr_el1;
    __asm__ __volatile__("mrs %0, ELR_EL1\n\t"
                         : "=r"(elr_el1));
    uart_printf("invalid exception : 0x%x\r\n", elr_el1);
    uart_printf("invalid exception : 0x%x\r\n",x0);
    while(1);
}

//https://developer.arm.com/documentation/ddi0595/2020-12/AArch64-Registers/DAIF--Interrupt-Mask-Bits
//https://www.twblogs.net/a/5b7c4ca52b71770a43da534e
//zero -> enable
//others -> disable
unsigned long long is_disable_interrupt()
{
    unsigned long long daif;
    __asm__ __volatile__("mrs %0, daif\n\t"
                         : "=r"(daif));

    return daif;  //enable -> daif == 0 (no mask)
}