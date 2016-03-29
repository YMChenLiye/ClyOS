#include <types.h>
#include <x86.h>

#define COM1			0x3F8
#define CRTPORT			0x3D4
#define LPTPORT			0x378
#define COM_TX			0
#define COM_LSR			5
#define COM_LSR_TXRDY	20

static uint16_t *crt = (uint16_t *)0xB8000;			//CGA memory

//stupid I/O delay routine necessitated by historical PC design flaws
static void
delay(void){
	inb(0x84);
	inb(0x84);
	inb(0x84);
	inb(0x84);
}

//copy console output to parallel port 并口
static void
lpt_putc(int c){
	int i;
	for(i=0;!(inb(LPTPORT + 1) & 0x80) && i < 12800;i++){
		delay();
	}
	outb(LPTPORT + 0,c);
	outb(LPTPORT + 2,0x08 | 0x04 | 0x01);
	outb(LPTPORT + 2,0x08);
}
/*
1. 读I/O端口地址0x379,等待并口准备好;
2. 向I/O端口地址0x378发出要输出的字符;
3. 向I/O端口地址0x37A发出控制命令,让并口处理要输出的字符。*/

//cga_putc - print character to console
static void
cga_putc(int c){
	int pos;

	//cursor position :col + 80*row
	outb(CRTPORT,14);
	pos = inb(CRTPORT + 1) << 8;
	outb(CRTPORT,15);
	pos |= inb(CRTPORT + 1);

	if(c == '\n'){
		pos += 80 - pos % 80;
	}
	else{
		crt[pos++] = (c & 0xff) | 0x0700;
	}

	outb(CRTPORT,14);
	outb(CRTPORT+1,pos>>8);
	outb(CRTPORT,15);
	outb(CRTPORT+1,pos);
}

static void
serial_putc(int c){
	int i;
	for(i=0;!(inb(COM1+COM_LSR) & COM_LSR_TXRDY) && i<12800;i++){
		delay();
	}
	outb(COM1+COM_TX,c);
}

//cons_putc - print a single character to console
static void
cons_putc(int c){
	lpt_putc(c);
	cga_putc(c);
	serial_putc(c);
}

//cons_puts - print a string to console 
static void
cons_puts(const char *str){
	while(*str != '\0'){
		cons_putc(*str++);
	}
}


void bootmain(void){
	cons_puts("This is a bootloader : Hello world!!");

	//do nothing
	while(1);
}

