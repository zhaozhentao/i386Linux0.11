#include <linux/sched.h>
#include <linux/tty.h>
#include <asm/system.h>
#include <asm/io.h>

#define ORIG_X              (*(unsigned char *)0x90000)                          // 光标位置 x
#define ORIG_Y              (*(unsigned char *)0x90001)                          // 光标位置 y
#define ORIG_VIDEO_MODE     ((*(unsigned short *)0x90006) & 0xff)                // 显示模式
#define ORIG_VIDEO_COLS     (((*(unsigned short *)0x90006) & 0xff00) >> 8)       // 显示列数
#define ORIG_VIDEO_LINES    (25)                                                 // 显示行数
#define ORIG_VIDEO_EGA_BX	(*(unsigned short *)0x9000a)

#define VIDEO_TYPE_MDA      0x10                                                 // 单色文本
#define VIDEO_TYPE_CGA      0x11                                                 // CGA 显示器
#define VIDEO_TYPE_EGAM     0x20                                                 // VGA EGA 单色
#define VIDEO_TYPE_EGAC     0x21                                                 // VGA EGA 彩色

extern void keyboard_interrupt(void);

static unsigned char	video_type;                                              // 使用的显示类型

static unsigned long    video_num_columns;                                       // 屏幕文本列数
static unsigned long    video_size_row;                                          // 屏幕每行字节数
static unsigned long    video_num_lines;                                         // 屏幕文本行数
static unsigned long    video_mem_start;                                         // 显示内存起始地址

static unsigned long	video_mem_end;                                           // 显示内存结束地址
static unsigned short	video_port_reg;                                          // 显示控制索引寄存器端口
static unsigned short	video_port_val;                                          // 显示控制数据寄存器端口
static unsigned short	video_erase_char;                                        // 擦除字符和属性

// 下面这些用于卷屏操作
static unsigned long    origin;                                                  // 滚屏起始地址
static unsigned long    scr_end;                                                 // 滚屏末端地址
static unsigned long    pos;                                                     // 当前光标对应的显示内存位置
static unsigned long    x,y;                                                     // 当前光标位置
static unsigned long    top,bottom;                                              // 
static unsigned long    state=0;
static unsigned char    attr=0x07;


static inline void gotoxy(unsigned int new_x, unsigned int new_y) {
    if (new_x > video_num_columns || new_y > video_num_lines)
        return;

    x = new_x;
    y = new_y;
    pos = origin + y * video_size_row + (x << 1);
}

static inline void set_origin(void)
{
    cli();
    outb_p(12, video_port_reg);
    outb_p(0xff&((origin-video_mem_start)>>9), video_port_val);
    outb_p(13, video_port_reg);
    outb_p(0xff&((origin-video_mem_start)>>1), video_port_val);
    sti();
}

static void scrup(void)
{
    if (video_type == VIDEO_TYPE_EGAC || video_type == VIDEO_TYPE_EGAM)
    {
        if (!top && bottom == video_num_lines) {
            origin += video_size_row;
            pos += video_size_row;
            scr_end += video_size_row;
            if (scr_end > video_mem_end) {
                __asm__("cld\n\t"
                    "rep\n\t"
                    "movsl\n\t"
                    "movl video_num_columns,%1\n\t"
                    "rep\n\t"
                    "stosw"
                    ::"a" (video_erase_char),
                    "c" ((video_num_lines-1)*video_num_columns>>1),
                    "D" (video_mem_start),
                    "S" (origin)
                    );
                scr_end -= origin-video_mem_start;
                pos -= origin-video_mem_start;
                origin = video_mem_start;
            } else {
                __asm__("cld\n\t"
                    "rep\n\t"
                    "stosw"
                    ::"a" (video_erase_char),
                    "c" (video_num_columns),
                    "D" (scr_end-video_size_row)
                    );
            }
            set_origin();
        } else {
            __asm__("cld\n\t"
                "rep\n\t"
                "movsl\n\t"
                "movl video_num_columns,%%ecx\n\t"
                "rep\n\t"
                "stosw"
                ::"a" (video_erase_char),
                "c" ((bottom-top-1)*video_num_columns>>1),
                "D" (origin+video_size_row*top),
                "S" (origin+video_size_row*(top+1))
                );
        }
    }
    else		/* Not EGA/VGA */
    {
        __asm__("cld\n\t"
            "rep\n\t"
            "movsl\n\t"
            "movl video_num_columns,%%ecx\n\t"
            "rep\n\t"
            "stosw"
            ::"a" (video_erase_char),
            "c" ((bottom-top-1)*video_num_columns>>1),
            "D" (origin+video_size_row*top),
            "S" (origin+video_size_row*(top+1))
            );
    }
}

static inline void lf(void) {
    if (y+1 < bottom) {
        y++;
        pos += video_size_row;
        return;
    }
    scrup();
}

static void cr(void) {
    pos -= x<<1;
    x = 0;
}

static inline void set_cursor(void) {
    cli();
    outb_p(14, video_port_reg);
    outb_p(0xff&((pos-video_mem_start)>>9), video_port_val);
    outb_p(15, video_port_reg);
    outb_p(0xff&((pos-video_mem_start)>>1), video_port_val);
    sti();
}

void con_write(struct tty_struct * tty) {
    int nr;
    char c;

    nr = CHARS(tty->write_q);

    while (nr--) {
        GETCH(tty->write_q,c);
        switch (state) {
            case 0:
                if (c > 31 && c < 127) {
                    if (x>=video_num_columns) {
                        x -= video_num_columns;
                        pos -= video_size_row;
                        lf();
                    }
                    __asm__("movb attr,%%ah\n\t"
                        "movw %%ax,%1\n\t"
                        ::"a" (c),"m" (*(short *)pos)
                        );
                    pos += 2;
                    x++;
                } else if (c == 10 || c == 11 || c == 12) {
                    lf();
                } else if (c == 13) {
                    cr();
                }
                break;
        }
    }

    set_cursor();
}

void con_init(void) {
    register unsigned char a;

    video_num_columns = ORIG_VIDEO_COLS;                 // 将 setup.s 中保存在内存的一些硬件参数取出
    video_size_row = video_num_columns * 2;
    video_num_lines = ORIG_VIDEO_LINES;
    video_erase_char = 0x0720;

    if (ORIG_VIDEO_MODE == 7) {
        video_mem_start = 0xb0000;
        video_port_reg = 0x3b4;
        video_port_val = 0x3b5;
        if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10) {
            video_type = VIDEO_TYPE_EGAM;
            video_mem_end = 0xb8000;
        } else {
            video_type = VIDEO_TYPE_MDA;
            video_mem_end = 0xb2000;
        }
    } else {
        video_mem_start = 0xb8000;
        video_port_reg = 0x3d4;
        video_port_val = 0x3d5;
        if ((ORIG_VIDEO_EGA_BX & 0xff) != 0x10) {
            video_type = VIDEO_TYPE_EGAC;
            video_mem_end = 0xbc000;
        } else {
            video_type = VIDEO_TYPE_CGA;
            video_mem_end = 0xba000;
        }
    }

    origin = video_mem_start;
    scr_end	= video_mem_start + video_num_lines * video_size_row;
    top	= 0;
    bottom = video_num_lines;

    gotoxy(ORIG_X,ORIG_Y);
    set_trap_gate(0x21, &keyboard_interrupt); // 注册键盘中断
    outb_p(inb_p(0x21)&0xfd,0x21);            // 取消对键盘中断的屏蔽
    a = inb_p(0x61);                          // 读取键盘端口
    outb_p(a|0x80,0x61);                      // 设置禁用键盘
    outb(a,0x61);                             // 允许键盘工作（复位）
}
