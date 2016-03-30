#include <types.h>
#include <x86.h>
#include <elf.h>

/*
 * 这是一个简单的boot loader，她的工作就是从第一块磁盘扇区引导一个ELF 内核映像
 *
 * DISK LAYOUT
 * 		这个程序(bootasm.S and bootmain.c)是bootloader
 * 		它应该储存在磁盘的第一个扇区上
 * 		第二块扇区接着储存内核映像
 * 		内核映像必须是ELF格式的
 *
 * BOOT UP STEPS
 * 		当cpu通电时，它将BIOS载入内存并且运行
 * 		BIOS初始化设备，设置中断例程，读取启动设备(比如硬盘)的第一块扇区进内存，然后跳转到那里
 * 		假设这个boot loader是储存在硬盘的第一个扇区中，这个代码开始接管。。。
 * 		控制开始在bootasm.S --哪一个设置了保护模式，创建了堆栈，然后c代码运行，调用bootmain()
 * 		在这个文件中的bootmain()接管，读取内核，并且跳转到内核。
 */





#define CRTPORT			0x3D4
#define LPTPORT			0x378
#define SECTSIZE		512			//扇区大小
#define ELFHDR			((struct elfhdr *)0x10000)


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
waitdisk(void){
	while((inb(0x1F7) & 0xc0) != 0x40)
		;//do nothing
}

static void
readsect(void *dst,uint32_t secno){
/*
 I/O地址
 功能

 0x1f0
 读数据,当0x1f7不为忙状态时,可以读。

 0x1f2
 要读写的扇区数,每次读写前,需要指出要读写几个扇区。

 0x1f3
 如果是LBA模式,就是LBA参数的0-7位

 0x1f4
 如果是LBA模式,就是LBA参数的8-15位

 0x1f5
 如果是LBA模式,就是LBA参数的16-23位

 0x1f6
 第0~3位:如果是LBA模式就是24-27位
 第4位:为0主盘;为1从盘
 第6位:为1=LBA模式;0 = CHS模式
 第7位和第5位必须为1

 0x1f7
 状态和命令寄存器。操作时先给命令,再读取内容;如果不是忙状态就从0x1f0端口读数据
 *
 */



	//wait for disk to be ready
	waitdisk();

	outb(0x1F2,1);							//count = 1
	outb(0x1F3,secno & 0xFF);
	outb(0x1F4,(secno>>8) & 0xFF);
	outb(0x1F5,(secno>>16) & 0xFF);
	outb(0x1F6,((secno>>24) & 0xF) | 0xE0);
	outb(0x1F7,0x20);						//cmd 0x20 - read sectors

	//wait for disk to be ready
	waitdisk();

	//read a sector
	insl(0x1F0,dst,SECTSIZE/4);
}

static void
readseg(uintptr_t va,uint32_t count,uint32_t offset){
	uintptr_t end_va = va + count;

	//round down to sector boundary
	va -= offset%SECTSIZE;

	//translate from bytes to sectors;kernel starts at sector 1
	uint32_t secno = (offset/SECTSIZE) + 1;

	for(;va<end_va;va += SECTSIZE,secno++){
		readsect((void *)va,secno);
	}
}



//cons_putc - print a single character to console
static void
cons_putc(int c){
	lpt_putc(c);
	cga_putc(c);
}


void bootmain(void){
	//read the 1st page off disk
	readseg((uintptr_t)ELFHDR,SECTSIZE * 8,0);

	//is this a valid ELF?
	if(ELFHDR->e_magic != ELF_MAGIC){
	}

	cons_putc('B');

	//do nothing
	while(1);

}

