#include <asm/system.h>

extern void con_init(void);

void main(void) {
    int i = 1;

    con_init();
    trap_init();
    sti();

    i = i / 0;
    for (;;);
}
