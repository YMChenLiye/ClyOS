#ifndef _LIBS_TYPES_H_
#define _LIBS_TYPES_H_

#ifndef NULL
#define NULL ((void *)0)
#endif

//代表true-false
typedef int bool;

//显式说明整数类型的版本
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

//指针和地址是32位的
//我们使用指针类型来表示地址
//uintptr_t 来表示地址的数值
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;

//size_t 是用来表示内存对象的大小的
typedef uintptr_t size_t;


#endif
