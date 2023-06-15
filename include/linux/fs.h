#ifndef _FS_H
#define _FS_H

#include <sys/types.h>

#define READ 0
#define WRITE 1
#define READA 2       /* read-ahead - don't pause */
#define WRITEA 3      /* "write-ahead" - silly, but somewhat useful */

void buffer_init(long buffer_end);

#define MAJOR(a) (((unsigned)(a))>>8)
#define MINOR(a) ((a)&0xff)

#define I_MAP_SLOTS 8
#define Z_MAP_SLOTS 8

#define NR_FILE 64                     // 系统最多打开文件个数
#define NR_SUPER 8                     // 系统包含的超级块个数
#define NR_HASH 307                    // 管理缓冲区的 hash_table 有 307 项
#define NR_BUFFERS nr_buffers          // 缓冲块数量，变量定义在 fs/buffer.c 中
#define BLOCK_SIZE 1024                // 每个缓冲区的大小 1k
#ifndef NULL
#define NULL ((void *) 0)
#endif

struct buffer_head {
    char * b_data;                     // 指向具体的缓冲区
    unsigned long b_blocknr;           // 要操作的块号
    unsigned short b_dev;              // 使用缓冲区的设备
    unsigned char b_uptodate;          // 缓冲区数据是否有效(是不是最新的)
    unsigned char b_dirt;              // 缓冲区是否更新过, 0 没有 1 有更新 (例如需要同步到硬盘)
    unsigned char b_count;             // 缓冲区的引用计数
    unsigned char b_lock;              // 缓冲区是否被锁 0 没有 1 被锁了
    struct buffer_head * b_prev;       // 前一个缓冲区头
    struct buffer_head * b_next;       // 下一个缓冲区头
    struct buffer_head * b_prev_free;  // 前一个空闲的缓冲区头
    struct buffer_head * b_next_free;  // 下一个空闲的缓冲区头
};

struct d_inode {
    unsigned short i_mode;
    unsigned short i_uid;
    unsigned long i_size;
    unsigned long i_time;
    unsigned char i_gid;
    unsigned char i_nlinks;
    unsigned short i_zone[9];
};

struct m_inode {
    unsigned short i_mode;
    unsigned short i_uid;
    unsigned long i_size;
    unsigned long i_mtime;
    unsigned char i_gid;
    unsigned char i_nlinks;
    unsigned short i_zone[9];
    /* these are in memory also */
    struct task_struct * i_wait;
    unsigned long i_atime;
    unsigned long i_ctime;
    unsigned short i_dev;
    unsigned short i_num;
    unsigned short i_count;
    unsigned char i_lock;
    unsigned char i_dirt;
    unsigned char i_pipe;
    unsigned char i_mount;
    unsigned char i_seek;
    unsigned char i_update;
};

struct file {
    unsigned short f_mode;
    unsigned short f_flags;
    unsigned short f_count;
    struct m_inode * f_inode;
    off_t f_pos;
};

struct super_block {
    unsigned short s_ninodes;
    unsigned short s_nzones;
    unsigned short s_imap_blocks;
    unsigned short s_zmap_blocks;
    unsigned short s_firstdatazone;
    unsigned short s_log_zone_size;
    unsigned long s_max_size;
    unsigned short s_magic;
    /* These are only in memory */
    struct buffer_head * s_imap[8];
    struct buffer_head * s_zmap[8];
    unsigned short s_dev;
    struct m_inode * s_isup;
    struct m_inode * s_imount;
    unsigned long s_time;
    struct task_struct * s_wait;
    unsigned char s_lock;
    unsigned char s_rd_only;
    unsigned char s_dirt;
};

struct d_super_block {
    unsigned short s_ninodes;
    unsigned short s_nzones;
    unsigned short s_imap_blocks;
    unsigned short s_zmap_blocks;
    unsigned short s_firstdatazone;
    unsigned short s_log_zone_size;
    unsigned long s_max_size;
    unsigned short s_magic;
};

extern struct file file_table[NR_FILE];

extern struct buffer_head * bread(int dev,int block);
extern int ROOT_DEV;

extern void mount_root(void);

#endif

