#include <types.h>
#include <x86.h>
#include <stab.h>
#include <stdio.h>
#include <string.h>
#include <kdebug.h>

#define STACKFRAME_DEPTH 20

extern const struct stab __STAB_BEGIN__[];	//stabs表的开始
extern const struct stab __STAB_END__[];	//stabs表的结束
extern const char __STABSTR_BEGIN__[];		//string表的开始
extern const char __STABSTR_END__[];		//string表的结束

//调试信息关于一个特殊指令指针
struct eipdebuginfo{
	const char *eip_file;				//源代码文件名
	int eip_line;						//源代码行号
	const char *eip_fn_name;			//包含eip的函数名
	int eip_fn_namelen;					//函数名的长度
	uintptr_t eip_fn_addr;				//函数开始的地址
	int eip_fn_narg;					//函数的参数数量
};

/* *
 * stab_binsearch - 根据输入，初始的值的范围是[*@region_left,*@region_right],找到一个stab条目(包含地址@addr并且符合类型@type),然后保存边界到@region_left和@region_right所指向的地区
 *
 * 一些stab类型是被安排成通过指令地址升序的。例如，N_FUN stabs(stab条目中 n_type == N_FUN)标记函数，N_SO stabs标记源文件
 *
 * 搜索发生在范围[*@region_left,*@region_right].
 * 因此，为了搜索一个N个stabs的条目，你可能要：
 *
 * 		left =0;
 * 		right = N-1;
 * 		stab_binsearch(stabs,&left,&right,type,addr);
 *
 *
 * 这个搜索修改了 *region_left 和 *region_right 来分类 @addr
 * *@region_left指向匹配的含有@addr的stab
 * *@region_right指向下一个stab的前一个
 * 如果*@region_left > *region_right,那么@addr不包含任何匹配的stab
 *
 * 例如
 *      Index  Type   Address
 *      0      SO     f0100000
 *      13     SO     f0100040
 *      117    SO     f0100176
 *      118    SO     f0100178
 *      555    SO     f0100652
 *      556    SO     f0100654
 *      657    SO     f0100849
 * this code:
 *      left = 0, right = 657;
 *      stab_binsearch(stabs, &left, &right, N_SO, 0xf0100184);
 * will exit setting left = 118, right = 554.
 * */
static void
stab_binsearch(const struct stab* stabs,int *region_left,int *region_right,
		int type,uintptr_t addr){
	int l=*region_left,r=*region_right,any_matches=0;
	while(l<=r){
		int true_m = (l+r)/2,m=true_m;

		//搜索最早出现的拥有正确类型的stab
		while(m>=l && stabs[m].n_type != type){
			m--;
		}
		if(m < l){//no match in [l,m]
			l = true_m +1;
			continue;
		}

		//实际的二叉搜索
		any_matches = 1;
		if(stabs[m].n_value < addr){
			*region_left = m;
			l=true_m + 1;
		}else if(stabs[m].n_value > addr){
			*region_right = m -1 ;
			r = m - 1;
		}else{
			*region_left = m;
			l = m;
			addr ++;
		}
	}

	if(!any_matches){
		*region_right = *region_left - 1;
	}else{
		l = *region_right;
		for(; l > *region_left && stabs[l].n_type != type ; l--){
			;//do nothing
		}
		*region_left = l;
	}
}

/* *
 * debuginfo_eip - 将关于特殊指令地址@addr的信息装入结构题@info中。
 * 如果信息被发现了，返回0,如果没有，返回负数。
 * 但是就算返回了负数，也会储存一些信息进‘*info'中
 * */
int
debuginfo_eip(uintptr_t addr,struct eipdebuginfo *info){
	const struct stab *stabs,*stab_end;
	const char *stabstr,*stabstr_end;

	info->eip_file = "<unknown>";
	info->eip_line = 0;
	info->eip_fn_name = "<unknown>";
	info->eip_fn_namelen = 9;
	info->eip_fn_addr = addr;
	info->eip_fn_narg = 0;

	stabs = __STAB_BEGIN__;
	stab_end = __STAB_END__;
	stabstr = __STABSTR_BEGIN__;
	stabstr_end = __STABSTR_END__;

	//字符串表的合法性检查
	if(stabstr_end <= stabstr || stabstr_end[-1] != 0){
		return -1;
	}

	// Now we find the right stabs that define the function containing
	// 'eip'.  First, we find the basic source file containing 'eip'.
	// Then, we look in that source file for the function.  Then we look
	// for the line number.

	//为了源文件搜索整个stabs的集合(type N_SO)
	int lfile = 0,rfile = (stab_end - stabs) - 1;
	stab_binsearch(stabs,&lfile,&rfile,N_SO,addr);
	if(lfile = 0)
		return -1;

	// Search within that file's stabs for the function definition
	// (N_FUN).
	int lfun = lfile, rfun = rfile;
	int lline, rline;
	stab_binsearch(stabs, &lfun, &rfun, N_FUN, addr);

	if (lfun <= rfun) {
		// stabs[lfun] points to the function name
		// in the string table, but check bounds just in case.
		if (stabs[lfun].n_strx < stabstr_end - stabstr) {
			info->eip_fn_name = stabstr + stabs[lfun].n_strx;
		}
		info->eip_fn_addr = stabs[lfun].n_value;
		addr -= info->eip_fn_addr;
		// Search within the function definition for the line number.
		lline = lfun;
		rline = rfun;
	} else {
		// Couldn't find function stab!  Maybe we're in an assembly
		// file.  Search the whole file for the line number.
		info->eip_fn_addr = addr;
		lline = lfile;
		rline = rfile;
	}
	info->eip_fn_namelen = strfind(info->eip_fn_name, ':') - info->eip_fn_name;

	// Search within [lline, rline] for the line number stab.
	// If found, set info->eip_line to the right line number.
	// If not found, return -1.
	stab_binsearch(stabs, &lline, &rline, N_SLINE, addr);
	if (lline <= rline) {
		info->eip_line = stabs[rline].n_desc;
	} else {
		return -1;
	}

	// Search backwards from the line number for the relevant filename stab.
	// We can't just use the "lfile" stab because inlined functions
	// can interpolate code from a different file!
	// Such included source files use the N_SOL stab type.
	while (lline >= lfile
			&& stabs[lline].n_type != N_SOL
			&& (stabs[lline].n_type != N_SO || !stabs[lline].n_value)) {
		lline --;
	}
	if (lline >= lfile && stabs[lline].n_strx < stabstr_end - stabstr) {
		info->eip_file = stabstr + stabs[lline].n_strx;
	}

	// Set eip_fn_narg to the number of arguments taken by the function,
	// or 0 if there was no containing function.
	if (lfun < rfun) {
		for (lline = lfun + 1;
				lline < rfun && stabs[lline].n_type == N_PSYM;
				lline ++) {
			info->eip_fn_narg ++;
		}
	}
	return 0;
}

/* *
 * print_kerninfo - print the information about kernel, including the location
 * of kernel entry, the start addresses of data and text segements, the start
 * address of free memory and how many memory that kernel has used.
 * */
void
print_kerninfo(void) {
	extern char etext[], edata[], end[], kern_init[];
	cprintf("Special kernel symbols:\n");
	cprintf("  entry  0x%08x (phys)\n", kern_init);
	cprintf("  etext  0x%08x (phys)\n", etext);
	cprintf("  edata  0x%08x (phys)\n", edata);
	cprintf("  end    0x%08x (phys)\n", end);
	cprintf("Kernel executable memory footprint: %dKB\n", (end - kern_init + 1023)/1024);
}

/* *
 * print_debuginfo - read and print the stat information for the address @eip,
 * and info.eip_fn_addr should be the first address of the related function.
 * */
void
print_debuginfo(uintptr_t eip) {
	struct eipdebuginfo info;
	if (debuginfo_eip(eip, &info) != 0) {
		cprintf("    <unknow>: -- 0x%08x --\n", eip);
	}
	else {
		char fnname[256];
		int j;
		for (j = 0; j < info.eip_fn_namelen; j ++) {
			fnname[j] = info.eip_fn_name[j];
		}
		fnname[j] = '\0';
		cprintf("    %s:%d: %s+%d\n", info.eip_file, info.eip_line,
				fnname, eip - info.eip_fn_addr);
	}
}

static uint32_t read_eip(void) __attribute__((noinline));

static uint32_t
read_eip(void) {
	uint32_t eip;
	asm volatile("movl 4(%%ebp), %0" : "=r" (eip));
	return eip;
}

/* *
 * print_stackframe - print a list of the saved eip values from the nested 'call'
 * instructions that led to the current point of execution
 *
 * The x86 stack pointer, namely esp, points to the lowest location on the stack
 * that is currently in use. Everything below that location in stack is free. Pushing
 * a value onto the stack will invole decreasing the stack pointer and then writing
 * the value to the place that stack pointer pointes to. And popping a value do the
 * opposite.
 *
 * The ebp (base pointer) register, in contrast, is associated with the stack
 * primarily by software convention. On entry to a C function, the function's
 * prologue code normally saves the previous function's base pointer by pushing
 * it onto the stack, and then copies the current esp value into ebp for the duration
 * of the function. If all the functions in a program obey this convention,
 * then at any given point during the program's execution, it is possible to trace
 * back through the stack by following the chain of saved ebp pointers and determining
 * exactly what nested sequence of function calls caused this particular point in the
 * program to be reached. This capability can be particularly useful, for example,
 * when a particular function causes an assert failure or panic because bad arguments
 * were passed to it, but you aren't sure who passed the bad arguments. A stack
 * backtrace lets you find the offending function.
 *
 * The inline function read_ebp() can tell us the value of current ebp. And the
 * non-inline function read_eip() is useful, it can read the value of current eip,
 * since while calling this function, read_eip() can read the caller's eip from
 * stack easily.
 *
 * In print_debuginfo(), the function debuginfo_eip() can get enough information about
 * calling-chain. Finally print_stackframe() will trace and print them for debugging.
 *
 * Note that, the length of ebp-chain is limited. In boot/bootasm.S, before jumping
 * to the kernel entry, the value of ebp has been set to zero, that's the boundary.
 * */
void
print_stackframe(void) {
	uint32_t ebp = read_ebp(), eip = read_eip();

	int i, j;
	for (i = 0; ebp != 0 && i < STACKFRAME_DEPTH; i ++) {
		cprintf("ebp:0x%08x eip:0x%08x args:", ebp, eip);
		uint32_t *args = (uint32_t *)ebp + 2;
		for (j = 0; j < 4; j ++) {
			cprintf("0x%08x ", args[j]);
		}
		cprintf("\n");
		print_debuginfo(eip - 1);
		eip = ((uint32_t *)ebp)[1];
		ebp = ((uint32_t *)ebp)[0];
	}
}

