#ifndef _BLK_H
#define _BLK_H

#define DEVICE_INTR do_hd

#ifdef DEVICE_INTR
void (*DEVICE_INTR)(void) = NULL;
#endif

#endif

