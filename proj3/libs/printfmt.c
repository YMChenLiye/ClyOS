#include <types.h>
#include <x86.h>
#include <error.h>
#include <stdio.h>
#include <string.h>

/* 
 * 空格或0填充和filed width只在数字格式中被支持
 * 被特指的格式 %e 有一个整数错误码，并且打印出一个字符串来描述错误
 * 这个整数可能是正数也有可能是负数
 * 所以 -E_NO_MEM 和 E_NO_MEN 是等同的
 */
static const char * const error_string[MAXERROR + 1]={
	[0]				NULL,
	[E_UNSPECIFIED]		"unspecified error",
	[E_BAD_PROC]		"bad process",
	[E_INVAL]			"invalid parameter",
	[E_NO_MEM]			"out of memory",
	[E_NO_FREE_PROC]	"out of processes",
	[E_FAULT]			"segmentation fault",
};


/* 
 * printnum - 逆序打印一个数(base <= 16) 
 * @putch:			指定的putch函数，打印单个字符
 * @putdat：		被putch函数使用
 * @num：			将会被打印的数量
 * @base：			base for print，必须在[1,16]中===进制
 * @width：			数字的最大个数，如果实际宽度小于@width，使用@padc来代替
 * @padc：			如果实际宽度小于@width，@padc是用来填充在左边的字符
 */
static void
printnum(void (*putch)(int,void *),void *putdat,
		unsigned long long num,unsigned base,int width,int padc){
	unsigned long long result = num;
	unsigned mod = do_div(result,base);//===> result /= base;mod=result % base;

	//第一次递归打印所有的前面的数字(权重更高)
	if(num>=base){
		printnum(putch,putdat,result,base,width-1,padc);
	}else{
		//在数字钱打印任何需要填充的字符
		while(--width > 0){
			putch(padc,putdat);
		}
	}
	//然后打印这个(权重最低的)数字
	putch("0123456789abcdef"[mod],putdat);
}


/* 
 * getuint - get an unsigned int of various possible sizes from a varargs list
 * @ap:			一个可变参数列表的指针
 * @lflag：		决定@ap指向的可变参数的大小
 */
static unsigned long long
getuint(va_list *ap,int lflag){
	if(lflag >= 2){
		return va_arg(*ap,unsigned long long);
	}
	else if(lflag){
		return va_arg(*ap,unsigned long);
	}
	else{
		return va_arg(*ap,unsigned int);
	}
}


/* 
 * getint - same as getuint but signed,we can't use getuint because of sign extension
 * @ap:			a varargs list pointer
 * @lflag:		determines the size of the vararg that @ap points to
 */
static long long
getint(va_list *ap,int lflag){
	if(lflag >= 2){
		return va_arg(*ap,long long);
	}
	else if(lflag){
		return va_arg(*ap,long);
	}
	else{
		return va_arg(*ap,int);
	}
}


/* 
 * printfmt - 格式化一个字符串，然后使用putch打印它
 * @putch:			指定的putch函数，打印单个字符
 * @putdat：		被putch函数使用
 * @fmt:			将要使用的格式
 */
void printfmt(void (*putch)(int,void *),void *putdat,const char *fmt,...){
	va_list ap;

	va_start(ap,fmt);
	vprintfmt(putch,putdat,fmt,ap);
	va_end(ap);
}


/* 
 * vprintfmt - 格式化一个字符串，然后使用putch打印它，这是被va_list调用
 * 来替代一个可变数量的参数
 * @putch:			指定的putch函数，打印单个字符
 * @putdat：		被putch函数使用
 * @fmt:			将要使用的格式
 * @ap:				格式化字符串的参数
 *
 * 你已经在处理一个va_list，才会调用这个函数，否则你可能是想调用printfmt()
 */
void 
vprintfmt(void (*putch)(int,void *),void *putdat,const char *fmt,va_list ap){
	register const char *p;
	register int ch,err;
	unsigned long long num;
	int base,width,precision,lflag,altflag;

	while(1){
		while((ch = *(unsigned char *)fmt ++) != '%'){
			if(ch == '\0'){
				return ;
			}
			putch(ch,putdat);
		}

		//process a %-escape sequence
		char padc = ' ';
		width = precision = -1;
		lflag = altflag = 0;

	reswitch:
		switch( ch = *(unsigned char *)fmt ++){

			//flag to pad on right
			case '-':
				padc = '-';
				goto reswitch;

			//flag to pad with 0's instead of spaces
			case '0':
				padc = '0';
				goto reswitch;
			
			//width field
			case '1' ... '9':
				for(precision =0;;++fmt){
					precision = precision * 10 + ch - '0';
					ch = *fmt;
					if(ch < '0' || ch > '9'){
						break;
					}
				}
				goto process_precision;
			
			case '*':
				precision = va_arg(ap,int);
				goto process_precision;

			case '.':
				if(width < 0)
					width =0;
				goto reswitch;

			case '#':
				altflag = 1;
				goto reswitch;

			process_precision:
				if(width < 0)
					width = precision , precision = -1;
				goto reswitch;

			//long flag(double for long long)
			case 'l':
				lflag++;
				goto reswitch;

			//character
			case 'c':
				putch(va_arg(ap,int),putdat);
				break;
			
			//error message
			case 'e':
				err = va_arg(ap,int);
				if(err < 0){
					err = -err;
				}
				if(err > MAXERROR || (p = error_string[err]) == NULL){
					printfmt(putch,putdat,"error %d",err);
				}
				else{
					printfmt(putch,putdat,"%s",p);
				}
				break;

			//string
			case 's':
				if((p = va_arg(ap,char *)) == NULL){
					p = "(null)";
				}
				if(width > 0 && padc != '-'){
					for(width -= strnlen(p,precision);width > 0;width--){
						putch(padc,putdat);
					}
				}
				for(;(ch = *p++) != '\0' && (precision < 0 || --precision >= 0);width--){
					if(altflag && (ch < ' ' || ch > '~')){
						putch('?',putdat);
					}
					else{
						putch(ch,putdat);
					}
				}
				for(;width > 0;width--){
					putch(' ',putdat);
				}
				break;

			//(signed) decimal
			case 'd':
				num = getint(&ap,lflag);
				if((long long)num < 0){
					putch('-',putdat);
					num = -(long long)num;
				}
				base = 10;
				goto number;

			//unsigned decimal
			case 'u':
				num = getuint(&ap,lflag);
				base =10;
				goto number;

			//(unsigned) octal
			case 'o':
				num = getuint(&ap,lflag);
				base = 8;
				goto number;

			//pointer
			case 'p':
				putch('0',putdat);
				putch('x',putdat);
				num = (unsigned long long)(uintptr_t)va_arg(ap,void *);
				base = 16;
				goto number;

			//(unsigned) hexadecimal
			case 'x':
				num = getuint(&ap,lflag);
				base = 16;

			number:
				printnum(putch,putdat,num,base,width,padc);
				break;

			//escaped '%' character
			case '%':
				putch(ch,putdat);
				break;

			//unrecognized escape sequence - just print it literally
			default:
				putch('%',putdat);
				for(fmt--;fmt[-1] != '%';fmt--)
					;//do nothing
				break;
		}
	}
}


// sprintbuf 是被用来保存关于一个buf的足够信息
struct sprintbuf{
	char *buf;			//指向第一个没有使用的内存地址
	char *ebuf;			//指向buffer的最后
	int cnt;			//在这个buffer中已经被代替的字符数量
};
		

/* 
 * sprintputch - '打印'单个字符到一个buffer中
 * @ch：			将会被打印的字符
 * @b：				放置@ch 的buffer
 */
static void
sprintputch(int ch,struct sprintbuf *b){
	b->cnt++;
	if(b->buf < b->ebuf){
		*b->buf ++ =ch;
	}
}

/* 
 * snprintf - 格式化一个字符串并将它放在一个buffer中
 * @str:			放置结果的buffer
 * @size：			buffer的大小，包括最后的中断符
 * @fmt：			将使用的格式化的字符串
 */
int
snprintf(char *str,size_t size,const char *fmt,...){
	va_list ap;
	int cnt;
	va_start(ap,fmt);
	cnt = vsnprintf(str,size,fmt,ap);
	va_end(ap);
	return cnt;
}

/* 
 * vsnprintf - 格式化一个字符串并且把它放在一个buffer中，它被va_list一起调用来代替可变数量的参数
 * @str:			放置结果的buffer
 * @size：			buffer的大小，包括最后的中断符
 * @fmt：			将使用的格式化字符串
 * @ap：			为了格式化字符串将使用的参数
 *
 * 返回值是字符(给定输入所产生的)数量，不包括'\0'
 *
 * 如果你已经准备处理va_list,才会调用这个函数
 * 否则可能你需要调用sprintf()
 */
int 
vsnprintf(char *str,size_t size,const char *fmt,va_list ap){
	struct sprintbuf b = {str,str+size-1,0};
	if(str == NULL || b.buf > b.ebuf){
		return -E_INVAL;
	}

	//打印字符串到buffer
	vprintfmt((void *)sprintputch,&b,fmt,ap);

	//null terminate the buffer
	*b.buf = '\0';
	return b.cnt;
}
