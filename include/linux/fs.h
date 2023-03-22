#ifndef _FS_H
#define _FS_H

void buffer_init(long buffer_end);

struct buffer_head {
    char * b_data;			/* pointer to data block (1024 bytes) */
    unsigned long b_blocknr;	/* block number */
    unsigned short b_dev;		/* device (0 = free) */
    unsigned char b_uptodate;
    unsigned char b_dirt;		/* 0-clean,1-dirty */
    unsigned char b_count;		/* users using this block */
    unsigned char b_lock;		/* 0 - ok, 1 -locked */
    struct task_struct * b_wait;
    struct buffer_head * b_prev;
    struct buffer_head * b_next;
    struct buffer_head * b_prev_free;
    struct buffer_head * b_next_free;
};

#define NR_OPEN 20

#ifndef NULL
#define NULL ((void *) 0)
#endif

extern int ROOT_DEV;

#endif

