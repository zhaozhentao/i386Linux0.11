#include <linux/sched.h>

#include "blk.h"

// 请求项数组队列, 共有 32 个请求
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

	for (i=0 ; i<NR_REQUEST ; i++) {
		request[i].dev = -1;                     // 全部设置为空闲项 -1
		request[i].next = NULL;
	}
}

