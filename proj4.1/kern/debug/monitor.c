#include <stdio.h>
#include <string.h>
#include <trap.h>
#include <monitor.h>
#include <kdebug.h>

/* *
 * 简单的命令行内核监视，交互式的来控制内核和探索系统是很有用的
 * */

struct command{
	const char *name;
	const char *desc;
	//返回-1来强制monitor来退出
	int (*func)(int argc,char **argv,struct trapframe *tf);
};

static struct command commands[] = {
	{"help", "Display this list of commands.", mon_help},
	{"kerninfo", "Display information about the kernel.", mon_kerninfo},
	{"backtrace", "Print backtrace of stack frame.", mon_backtrace},
};

#define NCOMMANDS (sizeof(commands)/sizeof(struct command))

//内核monitor命令解释

#define MAXARGS 	16
#define WHITESPACE	"\t\n\r"

//parse - 分析命令缓存到空格分隔的参数
static int
parse(char *buf,char **argv){
	int argc = 0;
	while(1){
		//发现全局空白
		while(*buf != '\0' && strchr(WHITESPACE,*buf) != NULL){
			*buf ++ = '\0';
		}
		if(*buf == '\0'){
			break;
		}

		if(argc == MAXARGS - 1){
			cprintf("too many argument(max %d).\n",MAXARGS);
		}
		argv[argc++] = buf;
		while(*buf != '\0' && strchr(WHITESPACE,*buf) == NULL){
			buf++;
		}
	}
	return argc;
}


/* *
 * runcmd - 分析输入字符串，将它分离成几个独立的参数
 * 然后查找和调用一些相关的命令
 * */
static int
runcmd(char *buf,struct trapframe *tf){
	char *argv[MAXARGS];
	int argc = parse(buf,argv);
	if(argc == 0){
		return 0;
	}
	int i;
	for(i = 0;i<NCOMMANDS;i++){
		if(strcmp(commands[i].name,argv[0]) == 0){
			return commands[i].func(argc-1,argv+1,tf);
		}
	}
	cprintf("Unknown command '%s',\n",argv[0]);
	return 0;
}


//实现基础的内核monitor命令
void
monitor(struct trapframe *tf){
	cprintf("Welcome to the kernel debug monitor!!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if(tf != NULL){
		print_trapframe(tf);
	}
	
	char *buf;
	while(1){
		if((buf = readline("K> ")) != NULL){
			if(runcmd(buf,tf) < 0){
				break;
			}
		}
	}
}

//mon_help - 打印关于mon_* 函数的信息
int
mon_help(int argc,char **argv,struct trapframe *tf){
	int i;
	for(i=0;i< NCOMMANDS;i++){
		cprintf("%s - %s\n",commands[i].name,commands[i].desc);
	}
	return 0;
}

/* *
 * mon_kerninfo - 调用print_kerninfo在kern/debug/kdebug.c中
 * 来打印内核中内存占有率
 * */
int
mon_kerninfo(int argc,char **argv,struct trapframe *tf){
	print_kerninfo();
	return 0;
}

/* *
 * mon_backtrace - 调用print_stackframe 在kern/debug/kdebug.c 中
 * 来打印栈的回溯
 * */
int
mon_backtrace(int argc,char **argv,struct trapframe *tf){
	print_stackframe();
	return 0;
}
