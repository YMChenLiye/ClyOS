#ifndef _LIBS_X86_H_
#define _LIBS_X86_H_

#include <types.h>

#define do_div(n,base)({									\
		unsigned long __upper,__low,__high,__mod,__base;	\
		__base=(base);										\
		asm("":"=a"(__low),"=d"(__high):"A"(n));			\
		__upper=__high;										\
		if (__high != 0){									\
		__upper = __high % __base;							\
		__high = __high / __base;							\
		}													\
		asm("divl %2":"=a"(__low),"=d"(__mod)				\
			:"rm"(__base),"0"(__low),"1"(__upper));			\
		asm("":"=A"(n):"a"(__low),"d"(__high));				\
		__mod;												\
		})


#define barrier() __asm__ __volatile__("" :::"memory")

static inline uint8_t inb(uint16_t port) __attribute__((always_inline));
static inline void insl(uint32_t port,void *addr,int cnt) __attribute__((always_inline));
static inline void outb(uint16_t port,uint8_t data) __attribute__((always_inline));
static inline void outw(uint16_t port,uint16_t data) __attribute__((always_inline));
static inline uint32_t read_ebp(void) __attribute__((always_inline));


static inline uint8_t
inb(uint16_t port)
{
	uint8_t data;
	asm volatile ("inb %1,%0":"=a"(data):"d"(port):"memory");
	return data;
}

static inline void
insl(uint32_t port,void *addr,int cnt){
	asm volatile(
			"cld;"
			"repne;insl;"
			: "=D"(addr),"=c"(cnt)
			: "d"(port),"0"(addr),"1"(cnt)
			: "memory","cc");
}

static inline void
outb(uint16_t port,uint8_t data)
{
	asm volatile("outb %0,%1"::"a"(data),"d"(port):"memory");
}

static inline void
outw(uint16_t port,uint16_t data){
	asm volatile("outw %0,%1"::"a"(data),"d"(port):"memory");
}

static inline uint32_t
read_ebp(void){
	uint32_t ebp;
	asm volatile("movl %%ebp,%0" : "=r"(ebp));
	return ebp;
}


static inline int __strcmp(const char *s1,const char *s2) __attribute__((always_inline));
static inline char *__strcpy(char *dst,const char *src) __attribute__((always_inline));
static inline void *__memset(void *s,char c,size_t n) __attribute__((always_inline));
static inline void *__memmove(void *dst,const void *src,size_t n) __attribute__((always_inline));
static inline void *__memcpy(void *dst,const void *src,size_t n) __attribute__((always_inline));


#ifndef __HAVE_ARCH_STRCMP
#define __HAVE_ARCH_STRCMP
static inline int
__strcmp(const char *s1,const char *s2){
	int d0,d1,ret;
	asm volatile(
			"1: lodsb;"
			"scasb;"
			"jne 2f;"
			"testb %%al,%%al;"
			"jne 1b;"
			"xorl %%eax,%%eax;"
			"jmp 3f;"
			"2: sbbl %%eax,%%eax;"
			"orb $1,%%al;"
			"3:"
			:"=a"(ret),"=&S"(d0),"=&D"(d1)
			:"1"(s1),"2"(s2)
			:"memory");
	return ret;
}
#endif

#ifndef __HAVE_ARCH_STRCPY
#define __HAVE_ARCH_STRCPY
static inline char *
__strcpy(char *dst,const char *src){
	int d0,d1,d2;
	asm volatile(
			"1: lodsb;"
			"stosb;"
			"testb %%al,%%al;"
			"jne 1b;"
			:"=&S"(d0),"=&D"(d1),"=&a"(d2)
			:"0"(src),"1"(dst):"memory");
	return dst;
}
#endif



#ifndef __HAVE_ARCH_MEMSET
#define __HAVE_ARCH_MEMSET
static inline void *
__memset(void *s,char c,size_t n){
	int d0,d1;
	asm volatile(
			"rep;stosb;"
			:"=&c"(d0),"=&D"(d1)
			:"0"(n),"a"(c),"1"(s)
			:"memory");
	return s;
}
#endif  //__HAVE_ARCH_MEMSET

#ifndef __HAVE_ARCH_MEMMOVE
#define __HAVE_ARCH_MEMMOVE
static inline void *
__memmove(void *dst,const void *src,size_t n){
	if(dst<src){//因为des<src，所以修改目的地址不会影响到源地址的数据
		return __memcpy(dst,src,n);
	}
	int d0,d1,d2;
	asm volatile(
			"std;"
			"rep;movsb;"
			"cld;"
			:"=&c"(d0),"=&S"(d1),"=&D"(d2)
			:"0"(n),"1"(n-1+src),"2"(n-1+dst)
			:"memory");
	return dst;
}


#endif //__HAVE_ARCH_MEMMOVE 

#ifndef __HAVE_ARCH_MEMCPY
#define __HAVE_ARCH_MEMCPY
static inline void *
__memcpy(void *dst,const void *src,size_t n){
	int d0,d1,d2;
	asm volatile(
			"rep;movsl;"
			"movl %4,%%ecx;"
			"andl $3,%%ecx;"
			"jz 1f;"
			"rep;movsb;"
			"1:"
			:"=&c"(d0),"=&D"(d1),"=&S"(d2)
			:"0"(n/4),"g"(n),"1"(dst),"2"(src)
			:"memory");
	return dst;
}



#endif //__HAVE_ARCH_MEMCPY

#endif //__LIBS_X86_H__
