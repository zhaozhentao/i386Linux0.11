#define MAJOR_NR 3

#include "blk.h"

void do_hd_request(void) {

}

void hd_init(void) {
    blk_dev[MAJOR_NR].request_fn = DEVICE_REQUEST;
}

