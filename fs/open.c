#include <errno.h>
#include <utime.h>
#include <sys/stat.h>

#include <linux/sched.h>
#include <linux/tty.h>
#include <asm/segment.h>

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
        actime = get_fs_long((unsigned long *) &times->actime);
        modtime = get_fs_long((unsigned long *) &times->modtime);
    } else {
        actime = modtime = CURRENT_TIME;
    }
    inode->i_atime = actime;
    inode->i_mtime = modtime;
    inode->i_dirt = 1;
    iput(inode);
    return 0;
}

int sys_open(const char * filename,int flag,int mode) {
    struct m_inode * inode;
    struct file * f;
    int i,fd;

    // 用户打开文件的模式与进程模式屏蔽码相与,得到许可的文件模式
    mode &= 0777 & ~current->umask;
    // 从进程的文件数组找出一个空闲的项 fd，每个进程最多打开 20 个文件
    for(fd=0 ; fd<NR_OPEN ; fd++)
        if (!current->filp[fd])
            break;
    // 如果 fd 大于 20 则进程可以打开的文件数已经到达最大值，返回失败
    if (fd>=NR_OPEN)
        return -EINVAL;
    // 进程每打开一个文件就会以 fd 位索引将 current->close_on_exec  相应的比特位置位
    // 将来进程关闭时检查每个比特位将打开的文件关闭，这里先将 fd 对应的比特位复位一下
    current->close_on_exec &= ~(1<<fd);
    // 从系统的 file_table 表找一个空闲项
    f=0+file_table;
    for (i=0 ; i<NR_FILE ; i++,f++)
        if (!f->f_count) break;
    // 如果超出系统可以打开的最大文件数 64 则返回错误
    if (i>=NR_FILE)
        return -EINVAL;
    // 将进程记录将要打开文件的空闲项指向系统 file_table 空闲项，并将引用计数 +1
    (current->filp[fd]=f)->f_count++;
    // 通过文件名打开文件，获取文件的 inode
    if ((i=open_namei(filename,flag,mode,&inode))<0) {
        current->filp[fd]=NULL;
        f->f_count=0;
        return i;
    }

    if (S_ISCHR(inode->i_mode)) {
        if (MAJOR(inode->i_zone[0])==4) {
            if (current->leader && current->tty<0) {
                current->tty = MINOR(inode->i_zone[0]);
                tty_table[current->tty].pgrp = current->pgrp;
            }
        } else if (MAJOR(inode->i_zone[0])==5)
            if (current->tty<0) {
                iput(inode);
                current->filp[fd]=NULL;
                f->f_count=0;
                return -EPERM;
            }
    }

    // 设置文件结构属性，引用计数，inode，设置打开位置为 0
    f->f_mode = inode->i_mode;
    f->f_flags = flag;
    f->f_count = 1;
    f->f_inode = inode;
    f->f_pos = 0;
    return (fd);
}

int sys_close(unsigned int fd)
{
   struct file * filp;

   if (fd >= NR_OPEN)
       return -EINVAL;
   current->close_on_exec &= ~(1<<fd);
   if (!(filp = current->filp[fd]))
       return -EINVAL;
   current->filp[fd] = NULL;
   if (filp->f_count == 0)
       panic("Close: file count is 0");
   if (--filp->f_count)
       return (0);
   iput(filp->f_inode);
   return (0);
}

