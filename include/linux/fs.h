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

#define NAME_LEN 14                    // 文件名长度
#define ROOT_INO 1                     // 根 i 节点

#define I_MAP_SLOTS 8                  // i 节点位图槽数
#define Z_MAP_SLOTS 8                  // 逻辑块位图槽数
#define SUPER_MAGIC 0x137F             // 文件系统魔数

#define NR_OPEN 20                     // 打开文件数
#define NR_INODE 32                    // 系统同时最多可以打开 i 节点个数
#define NR_FILE 64                     // 系统最多打开文件个数
#define NR_SUPER 8                     // 系统包含的超级块个数
#define NR_HASH 307                    // 管理缓冲区的 hash_table 有 307 项
#define NR_BUFFERS nr_buffers          // 缓冲块数量，变量定义在 fs/buffer.c 中
#define BLOCK_SIZE 1024                // 每个缓冲区的大小 1k
#ifndef NULL
#define NULL ((void *) 0)
#endif

#define INODES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct d_inode)))
// 每个块能够存放的目录项数目
#define DIR_ENTRIES_PER_BLOCK ((BLOCK_SIZE)/(sizeof (struct dir_entry)))

struct buffer_head {
    char * b_data;                     // 指向具体的缓冲区
    unsigned long b_blocknr;           // 要操作的块号
    unsigned short b_dev;              // 使用缓冲区的设备
    unsigned char b_uptodate;          // 缓冲区数据是否有效(是不是最新的)
    unsigned char b_dirt;              // 缓冲区是否更新过, 0 没有 1 有更新 (例如需要同步到硬盘)
    unsigned char b_count;             // 缓冲区的引用计数
    unsigned char b_lock;              // 缓冲区是否被锁 0 没有 1 被锁了
    struct task_struct * b_wait;
    struct buffer_head * b_prev;       // 前一个缓冲区头
    struct buffer_head * b_next;       // 下一个缓冲区头
    struct buffer_head * b_prev_free;  // 前一个空闲的缓冲区头
    struct buffer_head * b_next_free;  // 下一个空闲的缓冲区头
};

// 磁盘上的 i 节点数据结构
struct d_inode {
    unsigned short i_mode;             // 文件类型和属性(rwx 位)
    unsigned short i_uid;              // 用户 id
    unsigned long i_size;              // 文件大小
    unsigned long i_time;              // 修改时间
    unsigned char i_gid;               // 组 id
    unsigned char i_nlinks;            // 链接数(多少个文件目录项指向这个节点)
    unsigned short i_zone[9];          // 区段或称为逻辑块
};

// 内存中的 i 节点数据结构，前面 7 个字段和 d_inode 一致
struct m_inode {
    unsigned short i_mode;
    unsigned short i_uid;
    unsigned long i_size;
    unsigned long i_mtime;
    unsigned char i_gid;
    unsigned char i_nlinks;
    unsigned short i_zone[9];
    /* 只存在于内存中的字段 */
    // 暂时不涉及进程相关
    //struct task_struct * i_wait;
    unsigned long i_atime;              // 最后访问时间
    unsigned long i_ctime;              // i 节点自身修改时间
    unsigned short i_dev;               // i 节点所在的设备号
    unsigned short i_num;               // i 节点号
    unsigned short i_count;             // i 节点被使用次数，0 表示节点空闲
    unsigned char i_lock;               // 锁定标志
    unsigned char i_dirt;               // 是否被修改过标志
    unsigned char i_pipe;               // 管道标志
    unsigned char i_mount;              // 安装标志
    unsigned char i_seek;               // 搜寻标志
    unsigned char i_update;             // 更新标志
};

struct file {
    unsigned short f_mode;              // 文件操作模式(RW 位)
    unsigned short f_flags;             // 文件打开和控制的标志
    unsigned short f_count;             // 对应的文件句柄(文件描述符)
    struct m_inode * f_inode;           // 指向 i 节点
    off_t f_pos;                        // 文件位置(读写偏移值)
};

struct super_block {
    unsigned short s_ninodes;          // 节点数
    unsigned short s_nzones;           // 逻辑块数
    unsigned short s_imap_blocks;      // i 节点位图数据块数
    unsigned short s_zmap_blocks;      // 逻辑块位图数据块数
    unsigned short s_firstdatazone;    // 第一个数据逻辑块号
    unsigned short s_log_zone_size;    // log (数据块数/逻辑块) 以2为底数
    unsigned long s_max_size;          // 文件最大长度
    unsigned short s_magic;            // 文件系统魔数
    /* 下面这些字段只保存在内存中，不保存在磁盘 */
    struct buffer_head * s_imap[8];    // i 节点位图缓冲块指针数组(占用 8 块, 可以表示 64m )
    struct buffer_head * s_zmap[8];    // 逻辑块位图缓冲块指针数组(占用 8 块)
    unsigned short s_dev;              // 超级块所在的设备号
    struct m_inode * s_isup;           // 根目录 i 节点
    struct m_inode * s_imount;         // 被安装到的 i 节点
    unsigned long s_time;              // 修改时间
    // 暂时未接触到进程相关内容
    struct task_struct * s_wait;
    unsigned char s_lock;              // 锁定标志
    unsigned char s_rd_only;           // 只读标志
    unsigned char s_dirt;              // 脏标志(是否被修改过)
};

// 磁盘中的超级块结构
struct d_super_block {
    unsigned short s_ninodes;          // 节点数
    unsigned short s_nzones;           // 逻辑块数
    unsigned short s_imap_blocks;      // i 节点位图数据块数
    unsigned short s_zmap_blocks;      // 逻辑块位图数据块数
    unsigned short s_firstdatazone;    // 第一个数据逻辑块号
    unsigned short s_log_zone_size;    // log (数据块数/逻辑块) 以2为底数
    unsigned long s_max_size;          // 文件最大长度
    unsigned short s_magic;            // 文件系统魔数
};

// 目录项
struct dir_entry {
    unsigned short inode;              // inode 号
    char name[NAME_LEN];               // 文件名
};

extern struct file file_table[NR_FILE];
extern struct super_block super_block[NR_SUPER];
extern int nr_buffers;

extern struct buffer_head * bread(int dev,int block);
extern int ROOT_DEV;

#endif

