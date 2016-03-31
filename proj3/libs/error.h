#ifndef __LIBS_ERROR_H__
#define __LIBS_ERROR_H__

//内核错误代码 -- 与 lib/printfmt.c 中的列表保持同步
#define E_UNSPECIFIED		1	//没有特指或不知道的错误
#define E_BAD_PROC			2	//进程不存在或者其他
#define E_INVAL				3	//无效的参数
#define E_NO_MEM			4	//由于内存短缺引起的请求失效
#define E_NO_FREE_PROC		5	//尝试创造一个新进程
#define E_FAULT				6	//内存故障

//最大允许的错误码
#define MAXERROR			6


#endif
