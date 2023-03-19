#include <linux/tty.h>

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

void chr_dev_init(void) {

}

