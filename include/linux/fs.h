#ifndef _FS_H
#define _FS_H

void buffer_init(long buffer_end);

#define NR_HASH 307                    // 管理缓冲区的 hash_table 有 307 项
#define NR_BUFFERS nr_buffers          // 缓冲块数量，变量定义在 fs/buffer.c 中
#define BLOCK_SIZE 1024                // 每个缓冲区的大小 1k
#ifndef NULL
#define NULL ((void *) 0)
#endif

struct buffer_head {
    char * b_data;                     // 指向具体的缓冲区
    unsigned long b_blocknr;           //
    unsigned short b_dev;              // 使用缓冲区的设备
    unsigned char b_uptodate;          // 缓冲区数据是否有效(是不是最新的)
    unsigned char b_dirt;              // 缓冲区是否更新过, 0 没有 1 有更新 (例如需要同步到硬盘)
    unsigned char b_count;             // 缓冲区的引用计数
    unsigned char b_lock;              // 缓冲区是否被锁 0 没有 1 被锁了
    struct task_struct * b_wait;       // 等待该缓冲区的进程
    struct buffer_head * b_prev;       // 前一个缓冲区头
    struct buffer_head * b_next;       // 下一个缓冲区头
    struct buffer_head * b_prev_free;  // 前一个空闲的缓冲区头
    struct buffer_head * b_next_free;  // 下一个空闲的缓冲区头
};

extern int nr_buffers;                 // 缓冲块数量，定义在 fs/buffer.c 中

#endif

