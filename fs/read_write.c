#include <sys/stat.h>
#include <errno.h>

#include <linux/sched.h>

extern int block_read(int dev, off_t * pos, char * buf, int count);

int sys_read(unsigned int fd,char * buf,int count)
{
    struct file * file;
    struct m_inode * inode;
    
    if (fd>=NR_OPEN || count<0 || !(file=current->filp[fd]))
        return -EINVAL;
    if (!count)
        return 0;
    verify_area(buf,count);
    inode = file->f_inode;
    // todo pipe
    //if (inode->i_pipe)
    //    return (file->f_mode&1)?read_pipe(inode,buf,count):-EIO;

    if (S_ISCHR(inode->i_mode))
        return rw_char(READ,inode->i_zone[0],buf,count,&file->f_pos);
    if (S_ISBLK(inode->i_mode))
        return block_read(inode->i_zone[0],&file->f_pos,buf,count);
    if (S_ISDIR(inode->i_mode) || S_ISREG(inode->i_mode)) {
        if (count+file->f_pos > inode->i_size)
            count = inode->i_size - file->f_pos;
        if (count<=0)
            return 0;
        return file_read(inode,file,buf,count);
    }
    printk("(Read)inode->i_mode=%06o\n\r",inode->i_mode);
    return -EINVAL;
}

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


