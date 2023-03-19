#ifndef _TTY_H
#define _TTY_H

struct tty_queue {
    unsigned long data;
    unsigned long head;
    unsigned long tail;
};

struct tty_struct {
    struct tty_queue read_q;
};

extern struct tty_struct tty_table[];

void rs_init(void);
void con_init(void);
void tty_init(void);

#endif

