#include <ctype.h>

#include <linux/tty.h>

#include <asm/segment.h>

#define _L_FLAG(tty,f)	((tty)->termios.c_lflag & f)
#define _I_FLAG(tty,f)	((tty)->termios.c_iflag & f)
#define _O_FLAG(tty,f)	((tty)->termios.c_oflag & f)

#define L_CANON(tty)	_L_FLAG((tty),ICANON)
#define L_ISIG(tty)	_L_FLAG((tty),ISIG)
#define L_ECHO(tty)	_L_FLAG((tty),ECHO)
#define L_ECHOE(tty)	_L_FLAG((tty),ECHOE)
#define L_ECHOK(tty)	_L_FLAG((tty),ECHOK)
#define L_ECHOCTL(tty)	_L_FLAG((tty),ECHOCTL)
#define L_ECHOKE(tty)	_L_FLAG((tty),ECHOKE)

#define I_UCLC(tty)	_I_FLAG((tty),IUCLC)
#define I_NLCR(tty)	_I_FLAG((tty),INLCR)
#define I_CRNL(tty)	_I_FLAG((tty),ICRNL)
#define I_NOCR(tty)	_I_FLAG((tty),IGNCR)

#define O_POST(tty)	_O_FLAG((tty),OPOST)
#define O_NLCR(tty)	_O_FLAG((tty),ONLCR)
#define O_CRNL(tty)	_O_FLAG((tty),OCRNL)
#define O_NLRET(tty)	_O_FLAG((tty),ONLRET)
#define O_LCUC(tty)	_O_FLAG((tty),OLCUC)

struct tty_struct tty_table[] = {
    {
        {ICRNL,                     /* change incoming CR to NL */
        OPOST|ONLCR,                /* change outgoing NL to CRNL */
        0,
        ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,
        0,                          /* console termio */
        INIT_C_CC},
        0,                          /* initial pgrp */
        0,                          /* initial stopped */
        con_write,
        {0,0,0,0,""},               /* console read-queue */
        {0,0,0,0,""},               /* console write-queue */
        {0,0,0,0,""}                /* console secondary queue */
    }
};

int tty_write(unsigned channel, char * buf, int nr) {
    char c, *b = buf;
    static int cr_flag = 0;

    return 0;

//    static int cr_flag = 0;
//    struct tty_struct *tty;
//    char c, *b = buf;
//
//    if (channel > 2 || nr < 0) return -1;
//
//    tty = channel + tty_table;
//    while (nr > 0) {
//        while (nr > 0 && !FULL(tty->write_q)) {
//            c = get_fs_byte(b);
//            if (O_POST(tty)) {
//                if (c=='\r' && O_CRNL(tty))
//                    c='\n';
//                else if (c=='\n' && O_NLRET(tty))
//                    c='\r';
//                if (c=='\n' && !cr_flag && O_NLCR(tty)) {
//                    cr_flag = 1;
//                    PUTCH(13,tty->write_q);
//                    continue;
//                }
//                if (O_LCUC(tty))
//                    c=toupper(c);
//            }
//            b++; 
//            nr--;
//            cr_flag = 0;
//            PUTCH(c,tty->write_q);
//        }
//        tty->write(tty);
//    }
//
//    return (b - buf);
}

