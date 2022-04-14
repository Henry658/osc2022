#include "alloc.h"
#include "mini_uart.h"
void test_alloc(){
    int i;
    unsigned int size = 5;
    char * ptr = simple_alloc(4*size);
    for(i = 0; i < size ; i ++){
       *(ptr + i) = (char)(65+i);
    }
    *(ptr + i) = '\0';
    uart_send_string(ptr);
}

void test(){
	uart_printf("A\n");
}
//void (*p_get_el_value)(void) = &get_el_value;

void get_el_value(){
	int el = get_el();
	uart_printf("Exception level %d \n",el); 
}
