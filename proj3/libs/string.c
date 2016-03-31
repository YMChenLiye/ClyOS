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
 * memmove - 拷贝@n 个比特从@src指向的地址 到@dst指向的地址
 * @dst：	指针指向目的地
 * @src：	指针指向源地址
 * @n：		要被拷贝的比特的数量
 *
 * memmove() 函数返回 @dst
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
