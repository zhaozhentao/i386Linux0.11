#define ORIG_X              (*(unsigned char *)0x90000)                          // 光标位置 x
#define ORIG_Y              (*(unsigned char *)0x90001)                          // 光标位置 y
#define ORIG_VIDEO_MODE     ((*(unsigned short *)0x90006) & 0xff)                // 显示模式
#define ORIG_VIDEO_COLS     (((*(unsigned short *)0x90006) & 0xff00) >> 8)       // 显示列数
#define ORIG_VIDEO_LINES    (25)                                                 // 显示行数

static unsigned long    video_num_columns;                                       // 屏幕文本列数
static unsigned long    video_size_row;                                          // 屏幕每行字节数
static unsigned long    video_num_lines;                                         // 屏幕文本行数
static unsigned long    video_mem_start;                                         // 显示内存起始地址

static unsigned long    origin;
static unsigned long	pos;
static unsigned long    x,y;
static unsigned char	attr=0x07;

static inline void gotoxy(unsigned int new_x, unsigned int new_y) {
    if (new_x > video_num_columns || new_y > video_num_lines)
        return;

    x = new_x;
    y = new_y;
    pos = origin + y * video_size_row + (x << 1);
}

void con_write(void) {
    char *message = "hello kernel";

    gotoxy(ORIG_X, ORIG_Y);

    for (int i = 0; i < 12; i++) {
        __asm__("movb attr,%%ah\n\t"
        "movw %%ax,%1\n\t"
        ::"a" (message[i]),"m" (*(short *) pos)
        );
        pos += 2;
    }
}

void con_init(void) {
    video_num_columns = ORIG_VIDEO_COLS;                 // 将 setup.s 中保存在内存的一些硬件参数取出
    video_size_row = video_num_columns * 2;
    video_num_lines = ORIG_VIDEO_LINES;

    if (ORIG_VIDEO_MODE == 7) {
        video_mem_start = 0xb0000;
    } else {
        video_mem_start = 0xb8000;
    }

    origin = video_mem_start;
}

