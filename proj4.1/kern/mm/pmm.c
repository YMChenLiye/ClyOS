#include <types.h>
#include <x86.h>
#include <mmu.h>
#include <memlayout.h>
#include <pmm.h>

/* *
 * Task State Segment(任务状态段)
 * 
 * TSS可以存在在内存中的任意地方。一个特殊的寄存器叫Task Register(任务寄存器)(TR)保存一个指向一个在GDT中的有效的TSS段描述符段选择子。因此，为了使用一个TSS，下面这些必须在函数gdt_init中完成
 * ---在GDT中创建一个TSS描述符条目
 * ---增加足够的信息到内存中的TSS，需要的话
 * ---为了那个段载入一个段选择子到TR寄存器
 *
 * 当一个特权级发生变化的时候，在TSS中，有几个字段是要指明新的栈指针。但是在我们的内核中只有 SS0 和 ESP0是有用的
 *
 * 字段SS0 包含了CPL=0的栈段选择子，ESP0 包含了CPL=0的新的ESP的值。当一个中断发生在保护模式下的时候，x86CPU将会在TSS中寻找SS0和ESP0，然后将他们的值分别载入到SS 和 ESP中
 * */
static struct taskstate ts = {0};

/* *
 * Global Descriptor Table:(全局描述符表)
 * 
 * 内核和用户段是相同的(除了DPL)。为了加载%ss寄存器，CPL必须等于DPL.因此，我们必须为了用户和内核复制段。定义如下：
 *   -0x0:	unused(总是错误 -- for trapping NULL far pointers)
 *   -0x8:	内核代码段
 *	 -0x10:	内核数据段
 *	 -0x18: 用户代码段
 *	 -0x20:	用户数据段
 * 	 -0x28:	为了tss而定义的，在gdt_init中被初始化
 * */
static struct segdesc gdt[] = {
	SEG_NULL,
	[SEG_KTEXT] = SEG(STA_X | STA_R,0x0,0xFFFFFFFF,DPL_KERNEL),
	[SEG_KDATA] = SEG(STA_W,0x0,0xFFFFFFFF,DPL_KERNEL),
	[SEG_UTEXT] = SEG(STA_X | STA_R,0x0,0xFFFFFFFF,DPL_USER),
	[SEG_UDATA] = SEG(STA_W,0x0,0xFFFFFFFF,DPL_USER),
	[SEG_TSS]	= SEG_NULL,
};

static struct pseudodesc gdt_pd = {
	sizeof(gdt) -1,(uintptr_t)gdt
};

/* *
 * lgdt - 加载全局描述符表寄存器，并且为内核设置数据/代码段寄存器
 */
static inline void
lgdt(struct pseudodesc *pd){
	asm volatile ("lgdt (%0)" :: "r" (pd));
	asm volatile ("movw %%ax,%%gs" :: "a" (USER_DS));
	asm volatile ("movw %%ax,%%fs" :: "a" (USER_DS));
	asm volatile ("movw %%ax,%%es" :: "a" (KERNEL_DS));
	asm volatile ("movw %%ax,%%ds" :: "a" (KERNEL_DS));
	asm volatile ("movw %%ax,%%ss" :: "a" (KERNEL_DS));
	//reload cs
	asm volatile ("ljmp %0,$1f\n 1:\n" :: "i" (KERNEL_CS));
}

//临时内核栈
uint8_t stack0[1024];

//gdt_init - 初始化默认的GDT和TSS
static void
gdt_init(void){
	//设定一个TSS，那样当我们从用户态陷入内核态的时候我们就能得到一个正确的栈。
	//但是在这里并不安全，这只是一个临时的值。
	ts.ts_esp0 = (uint32_t)stack0 + sizeof(stack0);
	ts.ts_ss0  = KERNEL_DS;

	//初始化gdt的TSS字段
	gdt[SEG_TSS] = SEGTSS(STS_T32A,(uint32_t)&ts,sizeof(ts),DPL_KERNEL);

	//重新加载所有的段寄存器
	lgdt(&gdt_pd);

	//加载TSS
	ltr(GD_TSS);
}

//pmm_init - 初始化物理内存管理
//pmm)init - 设置一个pmm来管理物理内存，建立 PDT&PT 来设置分页机制
//		   - 检测pmm和paging mechanism的正确性，打印 PDT&PT
void
pmm_init(void){
	gdt_init();
}
