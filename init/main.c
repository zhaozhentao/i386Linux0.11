extern void con_init(void);
extern int printk(const char *fmt, ...);

void main(void) {
    con_init();
    printk("hello\nkernel");
}

