#include <linux/sched.h>

#include "blk.h"

// 请求项数组队列, 共有 32 个请求
struct request request[NR_REQUEST];

void blk_dev_init(void) {
	int i;

	for (i=0 ; i<NR_REQUEST ; i++) {
		request[i].dev = -1;                     // 全部设置为空闲项 -1
		request[i].next = NULL;
	}
}

