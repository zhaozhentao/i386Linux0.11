#ifndef _SCHED_H
#define _SCHED_H

#define NR_TASKS 64              // 最多可运行进程个数
#define HZ 100

#define FIRST_TASK task[0]
#define LAST_TASK task[NR_TASKS-1]

#include <linux/head.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <signal.h>

#define TASK_RUNNING            0
#define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2

typedef int (*fn_ptr)();

// 数学协处理器结构，进程切换时保存执行状态信息
struct i387_struct {
    long	cwd;
    long	swd;
    long	twd;
    long	fip;
    long	fcs;
    long	foo;
    long	fos;
    long	st_space[20];	/* 8*10 bytes for each FP-reg = 80 bytes */
};

// 任务状态段信息，进程切换时保存各个寄存器的信息
struct tss_struct {
    long	back_link;	/* 16 high bits zero */
    long	esp0;
    long	ss0;		/* 16 high bits zero */
    long	esp1;
    long	ss1;		/* 16 high bits zero */
    long	esp2;
    long	ss2;		/* 16 high bits zero */
    long	cr3;
    long	eip;
    long	eflags;
    long	eax,ecx,edx,ebx;
    long	esp;
    long	ebp;
    long	esi;
    long	edi;
    long	es;		/* 16 high bits zero */
    long	cs;		/* 16 high bits zero */
    long	ss;		/* 16 high bits zero */
    long	ds;		/* 16 high bits zero */
    long	fs;		/* 16 high bits zero */
    long	gs;		/* 16 high bits zero */
    long	ldt;		/* 16 high bits zero */
    long	trace_bitmap;	/* bits: trace 0, bitmap 16-31 */
    struct i387_struct i387;
};

struct task_struct {
    /* these are hardcoded - don't touch */
    long state;                     // 任务运行状态 -1，不可运行，0 可运行(就绪)，> 0 已停止
    long counter;                   // 任务运行时间(递减)，运行时间片
    long priority;                  // 任务优先级，任务开始时 counter = priority 越大运行越长
    long signal;                    // 信号位图，每个比特表示一种信号
    struct sigaction sigaction[32]; // 信号执行属性，即信号的处理器
    long blocked;                   // 进程信号屏蔽位
    /* various fields */
    int exit_code;                  // 任务执行的停止码，父进程会取
    unsigned long start_code;       // 代码段地址 (字节数)
    unsigned long end_code;         // 代码长度 (字节数)
    unsigned long end_data;         // 代码长度 + 数据长度 (字节数)
    unsigned long brk;              // 总长度 (字节数)
    unsigned long start_stack;      // 堆栈段地址
    long pid;                       // 进程号
    long father;                    // 父进程号
    long pgrp;                      // 进程组号
    long session;                   // 会话号
    long leader;                    // 会话首领
    unsigned short uid;             // 用户 id
    unsigned short euid;            // 有效用户 id
    unsigned short suid;            // 保存的用户 id
    unsigned short gid;             // 组 id
    unsigned short egid;            // 有效组 id
    unsigned short sgid;            // 保存的组 id
    long alarm;                     // 报警定时值
    long utime;                     // 用户态时间 (滴答数)
    long stime;                     // 系统态时间 (滴答数)
    long cutime;                    // 子进程用户态时间
    long cstime;                    // 子进程系统态时间
    long start_time;                // 进程开始时刻
    unsigned short used_math;       // 是否使用了协处理器
    /* file system info */
    int tty;                        // 进程使用 tty 的子设备号 -1 表示没有使用
    unsigned short umask;           // 文件创建属性屏蔽位
    struct m_inode * pwd;           // 当前工作目录 i 节点
    struct m_inode * root;          // 根目录 i 节点
    struct m_inode * executable;    // 执行文件 i 节点
    unsigned long close_on_exec;    // 执行时关闭文件句柄位图标志
    struct file * filp[NR_OPEN];    // 进程打开的文件
    /* ldt for this task 0 - zero 1 - cs 2 - ds&ss */
    struct desc_struct ldt[3];      // 本任务的局部描述符，第 0 项空,第 1 项代码段 cs,第 2 项数据和堆栈段 ds & ss
    /* tss for this task */
    struct tss_struct tss;          // 进程状态信息 (寄存器值)
};

// 定义第一个任务，基址 = 0 ，段限长 = 0x9ffff (640kb)
#define INIT_TASK \
/* state etc */	{ 0,15,15, \
/* signals */	0,{{},},0, \
/* ec,brk... */	0,0,0,0,0,0, \
/* pid etc.. */	0,-1,0,0,0, \
/* uid etc */	0,0,0,0,0,0, \
/* alarm */	0,0,0,0,0,0, \
/* math */	0, \
/* fs info */	-1,0022,NULL,NULL,NULL,0, \
/* filp */	{NULL,}, \
	{ \
		{0,0}, \
/* ldt */	{0x9f,0xc0fa00}, \
		{0x9f,0xc0f200}, \
	}, \
/*tss*/	{0,PAGE_SIZE+(long)&init_task,0x10,0,0,0,0,(long)&pg_dir,\
	 0,0,0,0,0,0,0,0, \
	 0,0,0x17,0x17,0x17,0x17,0x17,0x17, \
	 _LDT(0),0x80000000, \
		{} \
	}, \
}

extern struct task_struct *task[NR_TASKS];
extern struct task_struct *last_task_used_math;
extern struct task_struct *current;
extern long volatile jiffies;

#define FIRST_TSS_ENTRY 4
#define FIRST_LDT_ENTRY (FIRST_TSS_ENTRY+1)
#define _TSS(n) ((((unsigned long) n)<<4)+(FIRST_TSS_ENTRY<<3))
#define _LDT(n) ((((unsigned long) n)<<4)+(FIRST_LDT_ENTRY<<3))
#define ltr(n) __asm__("ltr %%ax"::"a" (_TSS(n)))
#define lldt(n) __asm__("lldt %%ax"::"a" (_LDT(n)))

#define switch_to(n) {\
struct {long a,b;} __tmp; \
__asm__("cmpl %%ecx,current\n\t" \
	"je 1f\n\t" \
	"movw %%dx,%1\n\t" \
	"xchgl %%ecx,current\n\t" \
	"ljmp *%0\n\t" \
	"cmpl %%ecx,last_task_used_math\n\t" \
	"jne 1f\n\t" \
	"clts\n" \
	"1:" \
	::"m" (*&__tmp.a),"m" (*&__tmp.b), \
	"d" (_TSS(n)),"c" ((long) task[n])); \
}

#define _set_base(addr,base)  \
__asm__ ("push %%edx\n\t" \
    "movw %%dx,%0\n\t" \
    "rorl $16,%%edx\n\t" \
    "movb %%dl,%1\n\t" \
    "movb %%dh,%2\n\t" \
    "pop %%edx" \
    ::"m" (*((addr)+2)), \
    "m" (*((addr)+4)), \
    "m" (*((addr)+7)), \
    "d" (base) \
    )

#define set_base(ldt,base) _set_base( ((char *)&(ldt)) , (base) )

static inline unsigned long _get_base(char * addr)
{
    unsigned long __base;
    __asm__("movb %3,%%dh\n\t"
        "movb %2,%%dl\n\t"
        "shll $16,%%edx\n\t"
        "movw %1,%%dx"
        :"=&d" (__base)
        :"m" (*((addr)+2)),
        "m" (*((addr)+4)),
        "m" (*((addr)+7)));
    return __base;
}

#define get_base(ldt) _get_base( ((char *)&(ldt)) )

#define get_limit(segment) ({ \
unsigned long __limit; \
__asm__("lsll %1,%0\n\tincl %0":"=r" (__limit):"r" (segment)); \
__limit;})

#endif

