#define outb(value,port) \
__asm__ ("outb %%al,%%dx"::"a" (value),"d" (port))

#define outb_p(value,port) \
__asm__ ("outb %%al,%%dx\n" \
    "\tjmp 1f\n" \
    "1:\tjmp 1f\n" \
    "1:"::"a" (value),"d" (port))

// 参数 port ，返回从端口读取的字节
#define inb_p(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al\n" \
	"\tjmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:":"=a" (_v):"d" (port)); \
_v; \
})

