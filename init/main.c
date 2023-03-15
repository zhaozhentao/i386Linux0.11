#include <linux/fs.h>

#define ORIG_ROOT_DEV (*(unsigned short *)0x901FC)

void main(void)
{
    ROOT_DEV = ORIG_ROOT_DEV;
}

