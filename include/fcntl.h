#ifndef _FCNTL_H
#define _FCNTL_H

#define O_ACCMODE             00003    // 文件访问模式屏蔽码
#define O_RDONLY                 00
#define O_WRONLY                 01    // 只写模式打开文件
#define O_RDWR                   02    // 以读写方式打开文件
#define O_CREAT               00100    // 如果文件不存在就创建
#define O_EXCL                00200    // 独占使用文件标志
#define O_TRUNC               01000    // 若文件存在且是写操作，截断文件
#define O_APPEND              02000
#define O_NONBLOCK            04000    /* not fcntl */


#define F_DUPFD		0	/* dup */
#define F_GETFD		1	/* get f_flags */
#define F_SETFD		2	/* set f_flags */
#define F_GETFL		3	/* more flags (cloexec) */
#define F_SETFL		4
#define F_GETLK		5	/* not implemented */
#define F_SETLK		6
#define F_SETLKW	7

#endif

