#include <stdio.h>

#define BUFSIZE 1024
static char buf[BUFSIZE];

/* *
 * readline - 从stdin中获得一行
 * @prompt：		将被写到stdout的字符串
 *
 * readline()函数会将输入字符串@prompt先写到stdout中。如果@prompt是NULL或者空字符串，那么也没有提示符流出
 *
 * 这个函数会保持读取字符并且将它们保存到buffer中直到遇见'\n' 或 '\r'
 * 注意：如果将会被读取的字符串的长度超过buffer的大小，字符串的后面部分将会被截断
 *
 * readline()函数热土让那读取到的行的文本，如果一些错误发生了，NULL将会被返回。
 * 返回值是一个全局变量，所以使用前应该先拷贝。
 */
char *
readline(const char *prompt){
	if(prompt != NULL){
		cprintf("%s",prompt);
	}
	int i=0,c;
	while(1){
		c = getchar();
		if(c < 0){
			return NULL;
		}
		else if(c >= ' ' && i < BUFSIZE -1){
			cputchar(c);
			buf[i++] = c;
		}
		else if(c == '\b' && i > 0){
			cputchar(c);
			i--;
		}
		else if(c == '\n' || c == '\r'){
			cputchar(c);
			buf[i] = '\0';
			return buf;
		}
	}
}
