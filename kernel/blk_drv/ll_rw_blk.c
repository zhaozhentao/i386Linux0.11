#include <linux/fs.h>

#include "blk.h"

struct request request[NR_REQUEST];

struct blk_dev_struct blk_dev[NR_BLK_DEV] = {
	{ NULL, NULL },		/* no_dev */
	{ NULL, NULL },		/* dev mem */
	{ NULL, NULL },		/* dev fd */
	{ NULL, NULL },		/* dev hd */
	{ NULL, NULL },		/* dev ttyx */
	{ NULL, NULL },		/* dev tty */
	{ NULL, NULL }		/* dev lp */
};

void blk_dev_init(void) {
    int i;

    // 初始化请求项
    for (i=0 ; i<NR_REQUEST ; i++) {
        request[i].dev = -1;
        request[i].next = NULL;
    }
}

