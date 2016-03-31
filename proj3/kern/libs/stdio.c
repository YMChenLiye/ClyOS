#include <types.h>
#include <stdio.h>
#include <console.h>

//高层面的console I/O

/* 
 * cputch - 写一个字符@c 到stdout，并且它会增长由@cnt指向的值
 */
static void
cputch(int c,int *cnt){
	cons_putc(c);
	(*cnt)++;
}

/* *
 * vcprintf - 格式化一个字符串并且将它写入到stdout
 *
 * 返回值是将会被写入到stdout中的字符的数量
 *
 * 当你已经准备好处理va_list 才会调用这个函数，否则你可能需要cprintf()
 */
int
vcprintf(const char *fmt,va_list ap){
	int cnt=0;
	vprintfmt((void *)cputch,&cnt,fmt,ap);
	return cnt;
}

/* *
 * cprintf - 格式化一个字符串并且写道stdout
 *
 * 返回值是将会被写入到stdout中的字符的数量
 * */
int 
cprintf(const char *fmt,...){
	va_list ap;
	int cnt;
	va_start(ap,fmt);
	cnt=vcprintf(fmt,ap);
	va_end(ap);
	return cnt;
}

//cputchar - 写单个字符到stdout
void cputchar(int c){
	cons_putc(c);
}

/* *
 * cputs - 将由@str指向的字符串写入到stdout中，并且增加一个新行字符('\n')
 * */
int 
cputs(const char *str){
	int cnt=0;
	char c;
	while((c=*str++) != '\0'){
		cputch(c,&cnt);
	}
	cputch('\n',&cnt);
	return cnt;
}
