#include <sys/stat.h>
#include <errno.h>

#include <linux/sched.h>

int sys_write(unsigned int fd,char * buf,int count) {
    struct file * file;
    struct m_inode * inode;

    // 检查文件描述符是否有效
    if (fd>=NR_OPEN || count <0 || !(file=current->filp[fd]))
        return -EINVAL;
    if (!count)
        return 0;
    // 取出打开文件的 inode
    inode=file->f_inode;
    // 如果打开的文件是字符设备，调用其读写操作
    if (S_ISCHR(inode->i_mode))
        return rw_char(WRITE,inode->i_zone[0],buf,count,&file->f_pos);
    printk("(Write)inode->i_mode=%06o\n\r",inode->i_mode);
    return -EINVAL;
}


