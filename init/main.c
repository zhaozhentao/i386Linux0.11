#include <asm/system.h>

extern void con_init(void);

void main(void) {
    con_init();
    trap_init();
    sti();

    for (;;);
}
