#include <asm.h>

#启动cpu，转变位32位的保护模式，跳转到c代码

#BIOS将磁盘中的第一个扇区载入进物理内存地址：0x7c00，然后开始运行在实模式：%cs=0,%ip=7c00

.set PROT_MODE_CSEG,	0x8				//内核代码段选择子
.set PROT_MODE_DSEG,	0x10			//内核数据段选择子
.set CR0_PE_ON,			0x1				//保护模式使能flag
.set SMAP,				0x534d4150

#start地址应该是：0x7c00,在实模式下，是运行bootloader的起始地址
.globl start
start:
.code16						#16位模式的汇编
	cli						#关中断
	cld						#将标志寄存器Flag的方向标志位DF清零。
				#在字串操作中使变址寄存器SI或DI的地址指针自动增加，字串处理由前往后。

	#设置重要的数据段寄存器（DS,ES,SS）
	xorw %ax,%ax		#将ax置0
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%ss

	#使能A20地址线：
	#为了兼容以前的pc，物理地址线20是被关闭的，所以超过1MB的将会默认回卷到0,下面的代码就是打开A20地址线

seta20.1:
	inb $0x64,%al
	testb $0x2,%al
	jnz	seta20.1

	movb $0xd1,%al					#0xd1 -> port 0x60
	outb %al,$0x64					#0xd1 means:write date to 8042's p2 port

seta20.2:
	inb $0x64,%al
	testb $0x2,%al
	jnz seta20.2

	movb $0xdf,%al					#0xdf -> port 0x60
	outb %al,$0x60					#0xdf=11011111,means set P2's A20 bit(the 1 bit) to 1

	#从实模式转换到保护模式，使用一个引导GDT和段转换得以使虚拟地址和物理地址相等，所以有效的内存映射不会在转换期间改变

	lgdt gdtdesc
	movl %cr0,%eax
	orl $CR0_PE_ON,%eax
	movl %eax,%cr0

	#跳转到下一条指令，但是是在32位的代码段
	#将处理器转换到32位模式
	ljmp $PROT_MODE_CSEG,$protcseg

.code32							#32位模式的汇编
protcseg:
	#设置保护模式的数据段寄存器
	movw $PROT_MODE_DSEG,%ax		#我们数据段选择子
	movw %ax,%ds
	movw %ax,%es
	movw %ax,%fs
	movw %ax,%gs
	movw %ax,%ss

	#设置栈指针，然后call c代码。栈段是从0--start(0x7c00)
	#因为代码是从低到搞运行的，所以0--7c00就是空着的，正好用作bootloader的栈
	movl $0x0,%ebp
	movl $start,%esp
	call bootmain

	# If bootmain returns(it shouldn't),loop
spin:
	jmp spin

#Bootstrap GDT
.p2align 2
gdt:
	SEG_NULLASM										#null seg
	SEG_ASM(STA_X|STA_R,0x0,0xffffffff)				#code seg for bootloader and kernel
	SEG_ASM(STA_W,0x0,0xffffffff)					#data seg for bootloader and kernel

gdtdesc:
	.word 0x17			#sizeof(gdt) - 1
	.long gdt			#address gdt
