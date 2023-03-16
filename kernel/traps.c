#include <linux/head.h>
#include <asm/system.h>

void divide_error(void);

void do_divide_error(long esp, long error_code) {

}

void trap_init(void) {
    int i;

    set_trap_gate(0, &divide_error);
}

