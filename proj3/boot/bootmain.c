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

#define SECTSIZE		512			//扇区大小
#define ELFHDR			((struct elfhdr *)0x10000)

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



void bootmain(void){
	//read the 1st page off disk
	readseg((uintptr_t)ELFHDR,SECTSIZE * 8,0);

	//is this a valid ELF?
	if(ELFHDR->e_magic != ELF_MAGIC){
		goto bad;
	}

	struct proghdr *ph,*eph;

	//载入每个程序段（忽略ph flags）
	ph = (struct proghdr *)((uintptr_t)ELFHDR + ELFHDR->e_phoff);
	eph = ph + ELFHDR->e_phnum;
	for(;ph < eph;ph++){
		readseg(ph->p_va & 0xFFFFFF,ph->p_memsz,ph->p_offset);
	}

	//调用从ELF头中得知的进入点
	//注意：没有返回
	//(void (*)(void)  ===== 是没有返回值，没有参数的函数指针，将e->entry强制类型转换成函数指针，然后调用
	((void (*)(void))(ELFHDR->e_entry & 0xFFFFFF))();

bad:
	outw(0x8A00,0x8A00);
	outw(0x8A00,0x8E00);

	//do nothing
	while(1);

}

