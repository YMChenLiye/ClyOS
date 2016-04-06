#include <string.h>
#include <x86.h>

/*
 * strlen - 计算string @s的长度，不包括最后的终止符'\0'
 * @s:		输入字符串
 *
 * strlen() 函数返回字符串@s的长度
 */

size_t
strlen(const char *s){
	size_t cnt = 0;
	while(*s++ != '\0'){
		cnt++;
	}
	return cnt;
}

/*
 * strnlen - 计算string @s的长度，不包括最后的终止符'\0',但是最多检查@len
 * @s:		输入字符串
 * @len：	将会扫描的最大长度
 *
 * Note that, this function looks only at the first @len characters
 * at @s, and never beyond @s + @len.
 * 
 *  返回值是strlen(s),如果那个小于@len，或者就是@len只要在@len长度里没有'\0'
 */
size_t 
strnlen(const char *s,size_t len){
	size_t cnt = 0;
	while(cnt < len && *s++ != '\0'){
		cnt++;
	}
	return cnt;
}


/* *
 * strcpy - 拷贝被@str指向的字符串到@dst指向的地址，包括最后的终止符
 * @dst：	指向要被拷贝的目的地址
 * @src：	将被拷贝的字符串
 *
 * 返回值是@dst
 *
 * 为了放置溢出，被@dst指向的数组大小应该和@src一样长（包括最后的终止符），并且不应该和@src所指向的有重叠
 */
char *
strcpy(char *dst,const char *src){
#ifndef __HAVE_ARCH_STRCPY
	return __strcpy(dst,src);
#else
	char *p =dst;
	while((*p ++ = *src ++) != '\0')
		;//do nothing
	return dst;
#endif
}

/* *
 * strncpy - 拷贝前面的@len个字符从@src到@dst。如果在@len个字符被拷贝之前@src就已经被拷贝了，@dst被'\0'填充，直到总共@len个字符已经被写入
 * @dst：		指向要被拷贝的目的地址
 * @src：		将被拷贝的字符串
 * @len：		从@src拷贝的最大数量
 *
 * 返回值是@dst
 */
char *
strncpy(char *dst,const char *src,size_t len){
	char *p =dst;
	while(len>0){
		if((*p = *src) != '\0') {
			src++;
		}
		p++,len--;
	}
	return dst;
}

/* *
 * strcmp - 比较字符串@s1 和 @s2
 * @s1:			字符串将被比较的
 * @s2：		字符串将被比较的
 *
 * 这个函数开始比较每个字符串的第一个字符。如果他们相同，那么接着比较，直到他们出现不同或者他们到达没最后的终止符。
 *
 * 返回整数值来表明两个字符串的关系：
 * 0表示两个字符串相同
 * 一个大于0的数表明第一个不相同的字符拥有更大的值
 * 一个小于0的数表明第一个不相同的字符拥有更小的值
 */
int
strcmp(const char *s1,const char *s2){
#ifndef __HAVE_ARCH_STRCMP
	return __strcmp(s1,s2);
#else
	while(*s1 != '\0' && *s1 == *s2){
		s1++,s2++;
	}
	return (int)((unsigned char)*s1 - (unsigned char)*s2);
#endif
}

/* *
 * strncmp - 比较最多@n个字符，其他和strcmp一样
 * @s1：		将被比较的字符串
 * @s2：		将被比较的字符串
 * @n：			最大比较的数量
 *
 * 这个函数开始比较每个字符串的第一个字符。如果他们相同，那么接着比较，直到他们出现不同或者他们到达最后的终止符或者@n个字符比较过了
 */
int strncmp(const char *s1,const char *s2,size_t n){
	while(n > 0 && *s1 != '\0' && *s1 == *s2){
		n--,s1++,s2++;
	}
	return (n==0) ? 0:(int)((unsigned char)*s1 - (unsigned char)*s2);
}


/* *
 * strchr - 找到在字符串中出现的第一个@c
 * @s：			输入的字符串
 * @c：			需要被找到的字符
 *
 * strchr()函数返回一个指针指向在@s中第一次出现的字符。如果这个值没有找到，函数返回NULL
 */
char *
strchr(const char *s,char c){
	while(*s != '\0'){
		if(*s == c){
			return (char *)s;
		}
		s++;
	}
	return NULL;
}

/* *
 * strfind - 找到这字符串中出现的第一个@c
 * @s：			输入的字符串
 * @c：			需要被找到的字符
 *
 * strfind()函数就像strchr()那样，除了当@c在@s中没有被发现的时候，它返回一个指针指向@s的最后，而不是’NULL'
 */
char *
strfind(const char *s,char c){
	while(*s != '\0'){
		if(*s == c){
			break;
		}
		s++;
	}
	return (char *)s;
}

/* *
 * strtol - 将字符串转换成整数
 * @s：			包含整数表示的输入字符串
 * @endptr：	reference to an object of type char *, whose value is set by the
 *         function to the next character in @s after the numerical value. This
 *         parameter can also be a null pointer, in which case it is not used.
 * @base:		进制
 *
 * 函数首先消除尽可能多的空白如果有必要的话，直到第一个不是空白的字符被发现。然后，从这个字符开始，根据所决定的进制的语义采用尽可能多的字符，将他们解释成数字的值。最后，一个指针指向在@s中跟随整数表示的第一个字符，这个指针被存储在@endptr中
 *
 * 如果@base的值是0的话，语义就像和整数内容一样：
 * -一个可选的加或减标志
 * -一个可选的前缀表示八进制或十六进制("0" or "0x")
 * -一连串的十进制(如果没有进制前缀)或者八进制或十六进制如果有一个特殊的前缀
 *
 *  如果base的值在2到36之间，期望的格式是数字和字母。
 *
 *  strtol()函数返回被转换的整数用一个long int表示
 */
long
strtol(const char *s,char **endptr,int base){
	int neg=0;
	long val=0;

	//消除空白
	while(*s == ' ' || *s == '\t'){
		s++;
	}

	//加或减标志
	if(*s == '+'){
		s++;
	}
	else if(*s == '-'){
		s++,neg=1;
	}

	//十六进制或八进制
	if((base == 0 || base == 16) && (s[0] == '0' && s[1] == 'x')){
		s+= 2,base=16;
	}
	else if(base ==0 && s[0] == '0'){
		s++,base =8;
	}
	else if(base ==0){
		base =10;
	}

	//数字
	while(1){
		int dig;

		if(*s >= '0' && *s <= '9'){
			dig = *s - '0';
		}
		else if(*s >= 'a' && *s <= 'z'){
			dig = *s - 'a' + 10;
		}
		else if(*s >= 'A' && *s <= 'Z'){
			dig = *s - 'A' + 10;
		}
		else{
			break;
		}
		if(dig >= base){
			break;
		}
		s++,val=(val*base)+dig;
		//我们不检测溢出
	}

	 if(endptr){
		 *endptr = (char *)s;
	 }
	 return (neg ? -val : val);
}


/* 
 * memset - 设置最开始@s 指向的的@n 比特的内存位特殊的值@c
 * @s：	指向要填充的内存区域
 * @c：	要设置的值
 * @n：	要设置的数量
 *
 * memset() 函数返回 @s
 */
void *
memset(void *s,char c,size_t n){
#ifdef __HAVE_ARCH_MEMSET
	return __memset(s,c,n);
#else
	char *p = s;
	while(n-- > 0){
		*p++ =c;
	}
	return s;
#endif //__HAVE_ARCH_MEMSET
}

/* 
 * memmove - 拷贝@n 个比特从@src指向的地址 到@dst指向的地址。@src和@dst可以重叠
 * @dst：	指针指向目的地
 * @src：	指针指向源地址
 * @n：		要被拷贝的比特的数量
 *
 * memmove() 函数返回 @dst
 */
void *
memmove(void *dst,const void *src,size_t n){
#ifdef __HAVE_ARCH_MEMMOVE
	return __memmove(dst,src,n);
#else
	const char *s = src;
	char *d = dst;
	if(s<d && s + n >d){
		s+=n,d+=n;
		while(n-- > 0){
			*--d=*--s;
		}
	}
	else{
		while(n-- > 0){
			*d++ = *s++;
		}
	}
	return dst;
#endif
}


/* 
 * memcpy - 拷贝@n 个比特从@src指向的地址 到@dst指向的地址
 * @dst：	指针指向目的地
 * @src：	指针指向源地址
 * @n：		要被拷贝的比特的数量
 *
 * memcpy() 函数返回 @dst
 * 注意：这个函数不会检查任何在@src中的终止符，它总是拷贝@n 个比特。
 * 为了防止重叠，被@src和@dst指向的数组应该最少@n 比特，并且不应该重叠
 * （对于重叠的内存区域，memmove是更安全的方法）
 */
void *
memcpy(void *dst,const void *src,size_t n){
#ifndef __HAVE_ARCH_MEMCPY
	return __memcpy(dst,src,n);
#else
	const char *s = src;
	char *d = dst;
	while(n-- > 0){
		*d++ =*s++;
	}
	return dst;
#endif
}



/* *
 * memcmp - 比较两个内存
 * @v1：		指针指向内存块
 * @v2：		指针指向内存块
 * @n：			比较的数量
 *
 * memcmp()函数返回整数值来表示两个内存块中的内容的大小
 * -一个0值表示两个内存块中的内容是相等的
 * -一个大于0的值表示大于
 * -一个小于0的值表示小于
 */
int
memcmp(const void *v1,const void *v2,size_t n){
	const char *s1 = (const char *)v1;
	const char *s2 = (const char *)v2;
	while(n-- > 0){
		if(*s1 != *s2){
			return (int)((unsigned char)*s1 - (unsigned char)*s2);
		}
		s1++,s2++;
	}
	return 0;
}

