#ifndef __KERN_DEBUG_ASSERT_H__
#define __KERN_DEBUG_ASSERT_H__

void __warn(const char *file,int line,const char *fmt,...);
void __panic(const char *file,int line,const char *fmt,...) __attribute__((noreturn));

#define warn(...)							\
	__warn(__FILE__,__LINE__,__VA_ARGS__)

#define panic(...)							\
	__panic(__FILE__,__LINE__,__VA_ARGS__)

#define assert(x)							\
	do{										\
		if(!(x)){							\
			panic("assertion failed:%s",#x);\
		}									\
	}while(0)								\

//static_assert(x) 将会生成一个编译时错误如果‘x'是false的话
#define static_assert(x)					\
	switch(x) { case 0: case (x): ;}

#endif
