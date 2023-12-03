#ifndef _ERRNO_H
#define _ERRNO_H

#define ERROR                   99      // 一般错误
#define EPERM                   1       // 操作没有许可
#define ENOENT                  2       // 文件或目录不存在
#define ENOEXEC                 8       // 执行程序格式错误
#define EBADF                   9       // 文件描述符错误
#define EAGAIN                  11      // 资源暂时不可用
#define ENOMEM                  12      // 内存不足
#define EACCES                  13      // 没有权限
#define EEXIST                  17      // 文件已经存在
#define EXDEV                   18      // 非法连接
#define ENODEV                  19      // 找不到对应的设备
#define ENOTDIR                 20      // 不是目录文件
#define EISDIR                  21      // 是目录文件
#define EINVAL                  22      // 参数无效
#define EMFILE                  24      // 打开文件数过多
#define ENOSPC                  28      // 没有空间
#define ENOSYS                  38      // 功能还没实现
#define ENOTEMPTY               39      // 目录不为空

#endif

