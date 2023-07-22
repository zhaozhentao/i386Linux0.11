#include <errno.h>
#include <sys/stat.h>

#include <linux/fs.h>

// 把 inode 中的信息 copy 到用户空间中的 statbuf
static void cp_stat(struct m_inode * inode, struct stat * statbuf) {
    struct stat tmp;
    int i;

    // todo verify_area
    // verify_area(statbuf,sizeof (* statbuf));
    tmp.st_dev = inode->i_dev;
    tmp.st_ino = inode->i_num;
    tmp.st_mode = inode->i_mode;
    tmp.st_nlink = inode->i_nlinks;
    tmp.st_uid = inode->i_uid;
    tmp.st_gid = inode->i_gid;
    tmp.st_rdev = inode->i_zone[0];
    tmp.st_size = inode->i_size;
    tmp.st_atime = inode->i_atime;
    tmp.st_mtime = inode->i_mtime;
    tmp.st_ctime = inode->i_ctime;
    // todo copy to user
    for (i=0 ; i<sizeof (tmp) ; i++)
        ((char *) statbuf)[i] = ((char *) &tmp)[i];
}

// 文件状态系统调用, 根据指定的文件名获取文件状态，文件状态存放在 statbuf 中
int sys_stat(char * filename, struct stat * statbuf) {
    struct m_inode * inode;

    if (!(inode=namei(filename)))
        return -ENOENT;
    cp_stat(inode,statbuf);
    iput(inode);
    return 0;
}

