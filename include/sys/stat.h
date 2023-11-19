#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/types.h>

struct stat {
    dev_t	st_dev;     // 文件所在设备号
    ino_t	st_ino;     // inode 号
    umode_t	st_mode;    // 文件类型和属性
    nlink_t	st_nlink;   // 文件连接数
    uid_t	st_uid;     // 文件用户号
    gid_t	st_gid;     // 文件组号
    dev_t	st_rdev;    // 设备号(如果文件是特殊字符文件或块文件)
    off_t	st_size;    // 文件大小
    time_t	st_atime;   // 文件最后访问时间
    time_t	st_mtime;   // 文件最后修改时间
    time_t	st_ctime;   // 文件节点修改时间
};

#define S_IFMT  00170000
#define S_IFREG  0100000     // 常规文件
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_ISUID  0004000     // 执行时设置用户 id
#define S_ISGID  0002000     // 执行时设置组 id

#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)   // 测试是否常规文件
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)   // 测试是否字符设备

#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100

#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010

#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001

#endif

