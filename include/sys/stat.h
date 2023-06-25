#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#define S_IFMT  00170000
#define S_IFDIR  0040000

#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)

#endif

