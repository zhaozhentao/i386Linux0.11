#ifndef _FS_H
#define _FS_H

void buffer_init(long buffer_end);

#define NR_OPEN 20

#ifndef NULL
#define NULL ((void *) 0)
#endif

extern int ROOT_DEV;

#endif

