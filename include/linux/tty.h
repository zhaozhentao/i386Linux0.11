#ifndef _TTY_H
#define _TTY_H

#include <termios.h>

#define TTY_BUF_SIZE 1024

struct tty_queue {
    unsigned long data;
    unsigned long head;                                            // 缓冲区数据头指针
    unsigned long tail;                                            // 缓冲区数据尾指针
    struct task_struct * proc_list;                                // 进程相关
    char buf[TTY_BUF_SIZE];                                        // 缓冲区
};

struct tty_struct {
    struct termios termios;                                        // 终端 io 属性和控制字符数据结构
    int pgrp;                                                      // 所属进程组
    int stopped;                                                   // 停止标志
    void (*write)(struct tty_struct * tty);                        // tty 写函数指针
    struct tty_queue read_q;                                       // tty 读缓冲区
    struct tty_queue write_q;                                      // tty 写缓冲区
    struct tty_queue secondary;                                    // tty 辅助缓冲区，存放规范模式(又叫熟模式)字符序列
};

// 缓冲区指针向前移 1 字节，超出缓冲区则回到 0 ，就是我们平时说的环形缓冲区
#define INC(a) ((a) = ((a)+1) & (TTY_BUF_SIZE-1))
// 缓冲区空闲长度
#define LEFT(a) (((a).tail-(a).head-1)&(TTY_BUF_SIZE-1))
// 判断缓冲区是否已经满，即空闲长度为 0
#define FULL(a) (!LEFT(a))
// 缓冲区已存放的字符长度
#define CHARS(a) (((a).head-(a).tail)&(TTY_BUF_SIZE-1))
// 从缓冲区取 1 个字节，并使 tail 后移 1 (字节)
#define GETCH(queue,c) \
(void)({c=(queue).buf[(queue).tail];INC((queue).tail);})
// 向缓冲区 queue 中存放一个字符，并使 head 后移 1 (字节)
#define PUTCH(c,queue) \
(void)({(queue).buf[(queue).head]=(c);INC((queue).head);})

#define INIT_C_CC "\003\034\177\025\004\0\1\0\021\023\032\0\022\017\027\026\0"

void con_write(struct tty_struct * tty);

#endif

