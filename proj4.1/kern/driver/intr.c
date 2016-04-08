#include <x86.h>
#include <intr.h>

//intr_enable - irq中断使能
void
intr_enable(void){
	sti();
}

//intr_disable - 关闭irq中断
void
intr_disable(void){
	cli();
}
