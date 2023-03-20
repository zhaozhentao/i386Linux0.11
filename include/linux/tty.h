#ifndef _TTY_H
#define _TTY_H

#include <termios.h>

#define TTY_BUF_SIZE 1024

struct tty_queue {
    unsigned long data;
    unsigned long head;
    unsigned long tail;
};

#define LEFT(a) (((a).tail-(a).head-1)&(TTY_BUF_SIZE-1))
#define FULL(a) (!LEFT(a))

struct tty_struct {
    struct termios termios;
    struct tty_queue read_q;
    struct tty_queue write_q;
};

extern struct tty_struct tty_table[];

void rs_init(void);
void con_init(void);
void tty_init(void);

#endif

