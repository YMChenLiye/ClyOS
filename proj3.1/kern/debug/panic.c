#include <types.h>
#include <x86.h>
#include <stdio.h>
#include <monitor.h>

static bool is_panic = 0;

/* *
 * __panic - __panic是在遇到不能解决的致命错误时调用的。它打印
 * “panic：’message‘，然后进入内核monitor
 * */
void
__panic(const char *file,int line,const char *fmt,...){
	if(is_panic){
		goto panic_dead;
	}
	is_panic = 1;

	//打印’message'
	va_list ap;
	va_start(ap,fmt);
	cprintf("kernel panic at %s,%d:\n  ",file,line);
	vcprintf(fmt,ap);
	cprintf("\n");
	va_end(ap);

panic_dead:
	while(1){
		monitor(NULL);
	}
}

//__warn - 像panic，但并不是
void
__warn(const char *file,int line,const char *fmt,...){
	va_list ap;
	va_start(ap,fmt);
	cprintf("kernel warning at %s:%d:\n  ",file,line);
	vcprintf(fmt,ap);
	cprintf("\n");
	va_end(ap);
}

bool
is_kernel_panic(void){
	return is_panic;
}
