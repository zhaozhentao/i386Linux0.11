extern int printk(const char *fmt, ...);
extern void con_init(void);

void main(void) {
    con_init();

    printk("hi");

    for (;;);
}

