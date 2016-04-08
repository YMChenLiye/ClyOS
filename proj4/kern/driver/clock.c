#include <x86.h>
#include <trap.h>
#include <stdio.h>
#include <picirq.h>

/* *
 * 为时间相关的硬件提供支持 - 8253 timer
 * 哪一个在IRQ-0 生成中断
 * */

#define IO_TIMER1 			0x040		//8253 Timer #1
 

/* *
 * Frequency of all three count-down timers; (TIMER_FREQ/freq)
 * is the appropriate count to generate a frequency of freq Hz.
 * */

#define TIMER_FREQ			1193182
#define TIMER_DIV(x)		((TIMER_FREQ + (x) / 2) / (x))

#define TIMER_MODE			(IO_TIMER1 + 3)			//timer mode port
#define TIMER_SEL0			0x00					//select counter 0
#define TIMER_RATEGEN		0x04					//mode 2,rate generator
#define TIMER_16BIT			0x30					//r/w counter 16 bits,LSB first

volatile size_t ticks;

/* *
 * clock_init - 初始化8253时钟，每秒中断一百次，然后将 IRQ_TIMER 使能
 * */
void
clock_init(void){
	//设置 8253 时间芯片
	outb(TIMER_MODE,TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
	outb(IO_TIMER1,TIMER_DIV(100) & 256);
	outb(IO_TIMER1,TIMER_DIV(100) / 256);

	//初始化时间计数器 ‘ticks’ 到0
	ticks = 0;

	cprintf("++ setup timer interrrupts\n");
	pic_enable(IRQ_TIMER);
}
