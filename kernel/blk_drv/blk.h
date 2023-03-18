#ifndef _BLK_H
#define _BLK_H

#define NR_BLK_DEV	7                         /* 块设备数量 */

#define NR_REQUEST	32

struct request {
	int dev;                                  /* -1 if no request */
	int cmd;                                  /* READ or WRITE */
	int errors;                               /* 操作时产生的错误次数 */
	unsigned long sector;                     /* 起始扇区 */
	unsigned long nr_sectors;                 /* 读些扇区数 */
	char * buffer;                            /* 数据缓冲区 */
	struct task_struct * waiting;             /* 任务等待操作执行完成的地方 */
	struct buffer_head * bh;                  /* 缓冲区指针头 */
	struct request * next;                    /* 下一个请求项 */
};

extern struct request request[NR_REQUEST];

#endif

