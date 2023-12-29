#include <errno.h>
#include <fcntl.h>

#include <linux/sched.h>
#include <asm/segment.h>

#define MIN(a,b) (((a)<(b))?(a):(b))

// 由 inode 可以知道设备号，buf 是用户空间中的缓冲区, count 指定要读取的字节数
int file_read(struct m_inode * inode, struct file * filp, char * buf, int count) {
    int left,chars,nr;
    struct buffer_head * bh;

    if ((left=count)<=0)
        return 0;

    while (left) {
        // 通过 inode 和文件读写位置获取文件在设备上的逻辑块号
        if ((nr = bmap(inode, (filp->f_pos)/BLOCK_SIZE))) {
            // 根据逻辑块号去读取数据
            if (!(bh=bread(inode->i_dev,nr)))
                break;
        } else
            bh = NULL;
        // 计算读写指针在数据块中的偏移 nr
        nr = filp->f_pos % BLOCK_SIZE;
        // 比较一下当前块和剩余未读数量，如果未读数量大于当前块剩余字节数则要读取下一个数据块
        chars = MIN( BLOCK_SIZE-nr , left );
        filp->f_pos += chars;
        left -= chars;
        // 如果上面从设备读到数据，则复制到缓冲区，否则填 0
        if (bh) {
            char * p = nr + bh->b_data;
            while (chars-->0)
                put_fs_byte(*(p++),buf++);
            brelse(bh);
        } else {
            while (chars-->0)
                put_fs_byte(0,buf++);
        }
    }
    inode->i_atime = CURRENT_TIME;
    return (count-left)?(count-left):-ERROR;
}

// 由 inode 可以知道设备号，buf 是用户空间中的缓冲区, count 指定要写入的字节数
int file_write(struct m_inode * inode, struct file * filp, char * buf, int count) {
    off_t pos;
    int block, c;
    struct buffer_head * bh;
    char * p;
    int i=0;

    // 追加或写入
    if (filp->f_flags & O_APPEND)
        pos = inode->i_size;
    else
        pos = filp->f_pos;

    while (i < count) {
        // 先取数据块号 pos/BLOCK_SIZE 在设备对应的逻辑块号 block, 如果不存在就创建一块，创建不成功就结束
        if (!(block = create_block(inode,pos/BLOCK_SIZE)))
            break;
        // 读取数据块
        if (!(bh=bread(inode->i_dev,block)))
            break;
        // 从指定的偏移位置开始写入，因为目前未实现 seek 相关代码，所以 pos 目前都是 0 开始
        c = pos % BLOCK_SIZE;
        // 数据从这个位置开始写入
        p = c + bh->b_data;
        bh->b_dirt = 1;
        c = BLOCK_SIZE-c;
        // 如果剩下需要写入的数据量在当前块能够放得下, 即 c = count-i
        if (c > count-i) c = count-i;
        pos += c;
        // 如果 pos 大于文件长度，则修改文件长度
        if (pos > inode->i_size) {
            inode->i_size = pos;
            inode->i_dirt = 1;
        }
        // 已经写入字节数增加
        i += c;
        while (c-->0)
            *(p++) = get_fs_byte(buf++);
        brelse(bh);
    }
    inode->i_mtime = CURRENT_TIME;
    if (!(filp->f_flags & O_APPEND)) {
        filp->f_pos = pos;
        inode->i_ctime = CURRENT_TIME;
    }
    return (i?i:-1);
}
