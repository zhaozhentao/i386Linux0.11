#include <linux/sched.h>

extern int end;  // 这个变量是由编译器添加的，是 bss 段结束后的第一个地址，表示内核程序的结束边界

struct buffer_head * start_buffer = (struct buffer_head *) &end; // 取内核程序的结束地址作为内核缓冲区的起始地址
struct buffer_head * hash_table[NR_HASH];                        // 内核使用 hash_table 管理内存，307 项
static struct buffer_head * free_list;                           // 空闲的内存链表
int NR_BUFFERS = 0;                                              // 用于统计缓冲块数量

void buffer_init(long buffer_end) {
    struct buffer_head * h = start_buffer;
    void * b;
    int i;

    if (buffer_end == 1<<20)
        b = (void *) (640*1024);
    else
        b = (void *) buffer_end;

    while ( (b -= BLOCK_SIZE) >= ((void *) (h+1)) ) {
        h->b_dev = 0;                                 // 使该缓冲区的设备号
        h->b_dirt = 0;                                // 脏标志，缓冲区是否被修改过
        h->b_count = 0;                               // 缓冲区引用计数
        h->b_lock = 0;                                // 缓冲区是否被锁
        h->b_uptodate = 0;                            // 缓冲区更新标志(缓冲区数据是否有效)
        h->b_wait = NULL;                             // 等待该缓冲区解锁的进程
        h->b_next = NULL;                             // 具有相同 hash 值的下一个缓冲头
        h->b_prev = NULL;                             // 具有相同 hash 值的前一个缓冲头
        h->b_data = (char *) b;                       // 缓冲区指向的具体数据块
        h->b_prev_free = h-1;                         // 链表的前一项
        h->b_next_free = h+1;                         // 链表的下一项
        h++;                                          // 使 h 指向下一个缓冲头
        NR_BUFFERS++;                                 // 缓冲区块数量增加
        if (b == (void *) 0x100000)
            b = (void *) 0xA0000;
    }

    h--;
    free_list = start_buffer;                         // 空闲链表指向第一个缓冲头块
    free_list->b_prev_free = h;                       // 使空闲链表的前一项指向最后一个缓冲头，形成双向环形链表
    h->b_next_free = free_list;                       // 最后一个缓冲头的下一项指向空闲链表的头，也是为了形成环形链表
    for (i=0;i<NR_HASH;i++)
        hash_table[i]=NULL;                           // 初始化 hash_table 全部指向 NULL
}

