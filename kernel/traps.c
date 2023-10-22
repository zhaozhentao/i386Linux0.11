#include <asm/system.h>

#include <linux/head.h>

void page_fault(void);

static void die(char *str, long esp_ptr, long nr) {
    printk("%s: %04x\n\r",str,nr&0xffff);
}

void divide_error(void);

void do_divide_error(long esp, long error_code) {
    die("divide error", esp, error_code);
}

void trap_init(void) {
    int i;

    set_trap_gate(0, &divide_error);
    set_trap_gate(14,&page_fault);
}
