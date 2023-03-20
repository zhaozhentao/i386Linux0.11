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
        {0,0,0}                     /* console */
    },
    {
        {0x3f8,0,0}                 /* rs 1 read_q, struct tty_queue data, head, tail */
    },
    {
        {0x2f8,0,0}                 /* rs 2 read_q, struct tty_queue data, head, tail */
    }
};

void tty_init(void) {
    rs_init();
    con_init();
}

// todo: sleep_if_full
int tty_write(unsigned channel, char * buf, int nr) {
    static int cr_flag=0;
    struct tty_struct * tty;
	char c, *b=buf;

    if (channel > 2 || nr < 0) return -1;

    tty = channel + tty_table;

    while (nr > 0) {
        while(nr > 0 && !FULL(tty->write_q)) {
            c = get_fs_byte(b);
            if (O_POST(tty)) {

            }

        }
    }

    return (b-buf);
}

void chr_dev_init(void) {

}

