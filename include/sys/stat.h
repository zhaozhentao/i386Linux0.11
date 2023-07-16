#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#define S_IFMT  00170000
#define S_IFDIR  0040000

#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)

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

