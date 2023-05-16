#ifndef _BLK_H
#define _BLK_H

#define DEVICE_INTR do_hd

#ifdef DEVICE_INTR
void (*DEVICE_INTR)(void) = NULL;
#endif

static inline void end_request(struct buffer_head* bh) {
    bh->b_lock = 0;
    bh->b_uptodate = 1;
}

#endif

