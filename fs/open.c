#include <errno.h>
#include <utime.h>

#include <linux/sched.h>

// 取文件系统信息，返回已经安装的文件系统统计信息向 ubuf 中设置总空闲块数和空闲 inode 数
int sys_ustat(int dev, struct ustat * ubuf) {
    // Linux 0.11 中未实现该系统调用
    return -ENOSYS;
}

// 修改文件访问时间和修改时间
int sys_utime(char * filename, struct utimbuf * times)
{
    struct m_inode * inode;
    long actime,modtime;

    if (!(inode=namei(filename)))
        return -ENOENT;
    if (times) {
        // todo copy from user
        actime = times->actime;
        modtime = times->modtime;
    } else {
        // actime = modtime = CURRENT_TIME;
    }
    inode->i_atime = actime;
    inode->i_mtime = modtime;
    inode->i_dirt = 1;
    iput(inode);
    return 0;
}

