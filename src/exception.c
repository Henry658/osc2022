#include "exception.h"
#include "utils.h"
#include "mini_uart.h"
#include "timer.h"
#include "entry.h"
#include "typedef.h"
#include "sched.h"
#include "syscall.h"
#include "peripherals/irq.h"
#include "peripherals/timer.h"

char intr_stack[INTR_STACK_SIZE];

const char *entry_error_messages[] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",
	"FIQ_INVALID_EL1t",
	"ERROR_INVALID_EL1t",

	"SYNC_INVALID_EL1h",
	"IRQ_INVALID_EL1h",
	"FIQ_INVALID_EL1h",
	"ERROR_INVALID_EL1h",

	"SYNC_INVALID_EL0_64",
	"IRQ_INVALID_EL0_64",
	"FIQ_INVALID_EL0_64",
	"ERROR_INVALID_EL0_64",

	"SYNC_INVALID_EL0_32",
	"IRQ_INVALID_EL0_32",
	"FIQ_INVALID_EL0_32",
	"ERROR_INVALID_EL0_32"
};

void show_invalid_entry_message(int type, unsigned long esr, unsigned long address){
	uart_printf("%s, ESR: %x, address: %x\r\n",entry_error_messages[type], esr, address);
	uart_printf("Exception class (EC) 0x%x\n",(esr >> 26) & 0b111111);
	uart_printf("Instruction specific syndrome (ISS) 0x%x\n", esr & 0xFFFFFF);
}

void not_implemented(){
	uart_printf("Not implemented function.... \n");
	while(1);
}

void handle_sync(unsigned long esr, unsigned long elr, trapframe_t * trapframe){
	enable_interrupt();
	int ec = (esr >> 26) & 0b111111;
	int iss = esr & 0x1FFFFFF;
	if(ec == 0b010101){
		unsigned long long syscall_num = trapframe->x[8];
		sys_call_router(syscall_num, trapframe);
	}
	else{	
		uart_printf("Exception return address %x\n",elr);
		uart_printf("Exception class (EC) %x\n",ec);
		uart_printf("Instruction specific syndrome (ISS) %x\n",iss);
	}
}

void sys_call_router(uint64_t syscall_num, trapframe_t * trapframe){
	switch(syscall_num){
		case SYS_GETPID: //0
			sys_getpid(trapframe);
			break;
			
		case SYS_UART_READ://1
			sys_uart_read(trapframe);
			break;
			
		case SYS_UART_WRITE://2
			sys_uart_write(trapframe);
			break;
			
		case SYS_EXEC://3
			sys_exec(trapframe);
			break;
			
		case SYS_FORK://4
			sys_fork(trapframe);
			break;
			
		case SYS_EXIT://5
			sys_exit(trapframe);
			break;
		case SYS_MBOX_CALL://6
			sys_mbox_call(trapframe);
			break;
		
		case SYS_KILL://7
			sys_kill(trapframe);
			break;
	}
	
}

void irq_stk_switcher(){
	// Switch to interrupt stack if entry_sp in kernel stack
	register char* entry_sp;
	asm volatile("mov %0, sp" : "=r"(entry_sp));
	if(!(entry_sp <= &intr_stack[4095] && entry_sp >= &intr_stack[0])){
		asm volatile("mov sp, %0" : : "r"(&intr_stack[INTR_STACK_TOP_IDX]));
	}
	
	handle_irq();
	
	if(!(entry_sp <= &intr_stack[4095] && entry_sp >= &intr_stack[0])){
		asm volatile("mov sp, %0" : : "r"(entry_sp));
	}
	
}

void handle_irq(void){
	if(*CORE0_INTERRUPT_SOURCE & SYSTEM_TIMER_IRQ_1){
		core_timer_disable();
		core_timer_handler();
		core_timer_enable();
	}
}

void irq_return(){
	task_struct_t * current = get_current_task();
	if(current->need_resched){
		current->counter = TASKEPOCH;
		current->need_resched = 0;
		schedule();
	}
}

void enable_interrupt(){
	__asm__ __volatile__("msr daifclr, 0xf");
}

void disable_interrupt(){
	__asm__ __volatile__("msr daifset, 0xf");
}


void lock(){
	disable_interrupt();
	lock_count++;	
}

void unlock(){
	lock_count--;
	if(lock_count < 0){
		uart_printf("lock error!!\n");
		while(1)uart_printf("lock error!!\n");
	}
	if(lock_count == 0){
		enable_interrupt();
	}
	
}
