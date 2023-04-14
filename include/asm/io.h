#define outb(value,port) \
__asm__ ("outb %%al,%%dx"::"a" (value),"d" (port))

#define outb_p(value,port) \
__asm__ ("outb %%al,%%dx\n" \
    "\tjmp 1f\n" \
    "1:\tjmp 1f\n" \
    "1:"::"a" (value),"d" (port))

