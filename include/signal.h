#ifndef _SIGNAL_H
#define _SIGNAL_H

typedef unsigned int sigset_t;		/* 32 bits */

#define SIGINT		 2
#define SIGQUIT		 3
#define SIGILL		 4
#define SIGKILL		 9

#define SA_NOCLDSTOP	1
#define SA_NOMASK	0x40000000
#define SA_ONESHOT	0x80000000

// 信号处理数据结构
struct sigaction {
    void (*sa_handler)(int);        // 信号的处理函数
    sigset_t sa_mask;               // 信号屏蔽码，信号执行时将阻塞对这些信号的处理
    int sa_flags;                   // 指定改变信号处理过程的信号集
    void (*sa_restorer)(void);      // 恢复函数指针，用于清理用户状态堆栈
};

#endif /* _SIGNAL_H */

