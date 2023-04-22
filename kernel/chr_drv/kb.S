#include <linux/config.h>

.text
.global keyboard_interrupt

size = 1024             /* 需要是 2 的倍数 ! 而且必须要和 tty_io.c 的一致 !!!! */
head = 4
tail = 8
proc_list = 12
buf = 16

mode:
  .byte 0               /* caps, alt, ctrl and shift mode */
leds:
  .byte 2               /* num-lock, caps, scroll-lock mode (nom-lock on) */
e0:
  .byte 0

/*
 *  键盘中断的入口，将键盘的扫描码转换成相应的 acsii 字符
 */
keyboard_interrupt:
  pushl %eax                   # 保护现场
  pushl %ebx
  pushl %ecx
  pushl %edx
  push %ds
  push %es
  movl $0x10,%eax              # 使 ds es 指向内核数据段
  mov %ax,%ds
  mov %ax,%es
  xor %al,%al                  # 清零 al 寄存器
  inb $0x60,%al                # 读取扫描码到 al 寄存器
  cmpb $0xe0,%al
  je set_e0                    # 如果扫描码是 e0 跳转到设置 e0 子程序
  cmpb $0xe1,%al
  je set_e1                    # 如果扫描码是 e1 跳转到 e1 子程序
  call *key_table(,%eax,4)     # 跳转到按键处理子程序 key_table + eax * 4
  movb $0,e0                   # 复位 e0 标志
e0_e1:
  inb $0x61,%al
  jmp 1f
1:
  jmp 1f
1:
  orb $0x80,%al
  jmp 1f
1:
  jmp 1f
1:
  outb %al,$0x61
  jmp 1f
1:
  jmp 1f
1:
  andb $0x7F,%al
  outb %al,$0x61
  movb $0x20,%al
  outb %al,$0x20
  pushl $0
  call do_tty_interrupt
  addl $4,%esp
  pop %es
  pop %ds
  popl %edx
  popl %ecx
  popl %ebx
  popl %eax
  iret
set_e0:
  movb $1,e0
  jmp e0_e1
set_e1:
  movb $2,e0
  jmp e0_e1

/*
 * 这个子程序最多可以向队列填写 8 字节，8 字节从 ebx 和 eax 两个寄存器中获得
 * 按照 al ah eal eah bl bh ... 的顺序依次进入缓冲区(因为 eax 和 ebx 都是 32 位)，
 * 直到 eax 为 0 ，因为 ebx 寄存器中的数据也是通过先移动到 eax 寄存器再进入缓冲区
 */
put_queue:
  pushl %ecx                # 保存 ecx 和 edx 寄存器原本的内容，因为下面的程序需要用到这两个寄存器
  pushl %edx
  movl table_list,%edx      # console 的 read_queue 地址
  movl head(%edx),%ecx      # 取缓冲区队列中头指针保存到 ecx 寄存器
1:
  movb %al,buf(%edx,%ecx)   # 将保存在 al 中的扫描码保存到 console 的 read_queue buf 中 ecx 位置,即buf[ecx] = al
  incl %ecx                 # ecx++
  andl $size-1,%ecx         # %ecx = %ecx & (0x3ff) ,若超出缓冲区，则 ecx 又回到了 0，环形缓冲区
  cmpl tail(%edx),%ecx      # 比较一下 tail 和 head 如果相等表示满了
  je 3f                     # jump if equals , 后面未存入的字符全部丢弃
  shrdl $8,%ebx,%eax        # 将 ebx 右移 8 位放到 eax ，ebx 本身不变
  je 2f                     # 检查一下上面移动 (eax) 还有字符吗？（等于 0 没有字符 跳转 2f）
  shrl $8,%ebx              # 将 ebx 右移 8 位，这次会影响 ebx，跳转到 1f 继续放入缓冲区
  jmp 1b
2:
  movl %ecx,head(%edx)      # 字符都放入缓冲区了，保存 head 指针
  movl proc_list(%edx),%ecx # proc_list 指针
  testl %ecx,%ecx           # 检查等待的进程队列
  je 3f                     # 没有等待的任务跳转 3f 返回
  movl $0,(%ecx)            # 使等待的任务就绪
3:
  popl %edx                 # 出栈，将前面保存的值恢复回 edx 和 ecs 寄存器
  popl %ecx
  ret

ctrl:
  movb $0x04,%al
  jmp 1f
alt:
  movb $0x10,%al
1:
  cmpb $0,e0
  je 2f
  addb %al,%al
2:
  orb %al,mode
  ret
unctrl:
  movb $0x04,%al
  jmp 1f
unalt:
  movb $0x10,%al
1:
  cmpb $0,e0
  je 2f
  addb %al,%al
2:
  notb %al
  andb %al,mode
  ret

lshift:
  orb $0x01,mode
  ret
unlshift:
  andb $0xfe,mode
  ret
rshift:
  orb $0x02,mode
  ret
unrshift:
  andb $0xfd,mode
  ret
caps:
  testb $0x80,mode
  jne 1f
  xorb $4,leds
  xorb $0x40,mode
  orb $0x80,mode
set_leds:
  call kb_wait
  movb $0xed,%al		/* set leds command */
  outb %al,$0x60
  call kb_wait
  movb leds,%al
  outb %al,$0x60
  ret
uncaps:
  andb $0x7f,mode
  ret
scroll:
  xorb $1,leds
  jmp set_leds
num:
  xorb $2,leds
  jmp set_leds

/*
 *  curosr-key/numeric keypad cursor keys are handled here.
 *  checking for numeric keypad etc.
 */
cursor:
  subb $0x47,%al
  jb 1f
  cmpb $12,%al
  ja 1f
  jne cur2		/* check for ctrl-alt-del */
  testb $0x0c,mode
  je cur2
  testb $0x30,mode
  jne reboot
cur2:
  cmpb $0x01,e0		/* e0 forces cursor movement */
  je cur
  testb $0x02,leds	/* not num-lock forces cursor */
  je cur
  testb $0x03,mode	/* shift forces cursor */
  jne cur
  xorl %ebx,%ebx
  movb num_table(%eax),%al
  jmp put_queue
1:
  ret

cur:
  movb cur_table(%eax),%al
  cmpb $'9,%al
  ja ok_cur
  movb $'~,%ah
ok_cur:
  shll $16,%eax
  movw $0x5b1b,%ax
  xorl %ebx,%ebx
  jmp put_queue

#if defined(KBD_FR)
num_table:
  .ascii "789 456 1230."
#else
num_table:
  .ascii "789 456 1230,"
#endif
cur_table:
  .ascii "HA5 DGC YB623"

/*
 * this routine handles function keys
 */
func:
  # todo

#if	defined(KBD_FINNISH)
key_map:
  .byte 0,27
  .ascii "1234567890+'"
  .byte 127,9
  .ascii "qwertyuiop}"
  .byte 0,13,0
  .ascii "asdfghjkl|{"
  .byte 0,0
  .ascii "'zxcvbnm,.-"
  .byte 0,'*,0,32		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte '-,0,0,0,'+	/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '<
  .fill 10,1,0

shift_map:
  .byte 0,27
  .ascii "!\"#$%&/()=?`"
  .byte 127,9
  .ascii "QWERTYUIOP]^"
  .byte 13,0
  .ascii "ASDFGHJKL\\["
  .byte 0,0
  .ascii "*ZXCVBNM;:_"
  .byte 0,'*,0,32		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte '-,0,0,0,'+	/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '>
  .fill 10,1,0

alt_map:
  .byte 0,0
  .ascii "\0@\0$\0\0{[]}\\\0"
  .byte 0,0
  .byte 0,0,0,0,0,0,0,0,0,0,0
  .byte '~,13,0
  .byte 0,0,0,0,0,0,0,0,0,0,0
  .byte 0,0
  .byte 0,0,0,0,0,0,0,0,0,0,0
  .byte 0,0,0,0		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte 0,0,0,0,0		/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '|
  .fill 10,1,0

#elif defined(KBD_US)

key_map:
  .byte 0,27
  .ascii "1234567890-="
  .byte 127,9
  .ascii "qwertyuiop[]"
  .byte 13,0
  .ascii "asdfghjkl;'"
  .byte '`,0
  .ascii "\\zxcvbnm,./"
  .byte 0,'*,0,32		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte '-,0,0,0,'+	/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '<
  .fill 10,1,0


shift_map:
  .byte 0,27
  .ascii "!@#$%^&*()_+"
  .byte 127,9
  .ascii "QWERTYUIOP{}"
  .byte 13,0
  .ascii "ASDFGHJKL:\""
  .byte '~,0
  .ascii "|ZXCVBNM<>?"
  .byte 0,'*,0,32		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte '-,0,0,0,'+	/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '>
  .fill 10,1,0

alt_map:
  .byte 0,0
  .ascii "\0@\0$\0\0{[]}\\\0"
  .byte 0,0
  .byte 0,0,0,0,0,0,0,0,0,0,0
  .byte '~,13,0
  .byte 0,0,0,0,0,0,0,0,0,0,0
  .byte 0,0
  .byte 0,0,0,0,0,0,0,0,0,0,0
  .byte 0,0,0,0		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte 0,0,0,0,0		/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '|
  .fill 10,1,0

#elif defined(KBD_GR)

key_map:
  .byte 0,27
  .ascii "1234567890\\'"
  .byte 127,9
  .ascii "qwertzuiop@+"
  .byte 13,0
  .ascii "asdfghjkl[]^"
  .byte 0,'#
  .ascii "yxcvbnm,.-"
  .byte 0,'*,0,32		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte '-,0,0,0,'+	/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '<
  .fill 10,1,0

shift_map:
  .byte 0,27
  .ascii "!\"#$%&/()=?`"
  .byte 127,9
  .ascii "QWERTZUIOP\\*"
  .byte 13,0
  .ascii "ASDFGHJKL{}~"
  .byte 0,''
  .ascii "YXCVBNM;:_"
  .byte 0,'*,0,32		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte '-,0,0,0,'+	/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '>
  .fill 10,1,0

alt_map:
  .byte 0,0
  .ascii "\0@\0$\0\0{[]}\\\0"
  .byte 0,0
  .byte '@,0,0,0,0,0,0,0,0,0,0
  .byte '~,13,0
  .byte 0,0,0,0,0,0,0,0,0,0,0
  .byte 0,0
  .byte 0,0,0,0,0,0,0,0,0,0,0
  .byte 0,0,0,0		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte 0,0,0,0,0		/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '|
  .fill 10,1,0


#elif defined(KBD_FR)

key_map:
  .byte 0,27
  .ascii "&{\"'(-}_/@)="
  .byte 127,9
  .ascii "azertyuiop^$"
  .byte 13,0
  .ascii "qsdfghjklm|"
  .byte '`,0,42		/* coin sup gauche, don't know, [*|mu] */
  .ascii "wxcvbn,;:!"
  .byte 0,'*,0,32		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte '-,0,0,0,'+	/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '<
  .fill 10,1,0

shift_map:
  .byte 0,27
  .ascii "1234567890]+"
  .byte 127,9
  .ascii "AZERTYUIOP<>"
  .byte 13,0
  .ascii "QSDFGHJKLM%"
  .byte '~,0,'#
  .ascii "WXCVBN?./\\"
  .byte 0,'*,0,32		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte '-,0,0,0,'+	/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '>
  .fill 10,1,0

alt_map:
  .byte 0,0
  .ascii "\0~#{[|`\\^@]}"
  .byte 0,0
  .byte '@,0,0,0,0,0,0,0,0,0,0
  .byte '~,13,0
  .byte 0,0,0,0,0,0,0,0,0,0,0
  .byte 0,0
  .byte 0,0,0,0,0,0,0,0,0,0,0
  .byte 0,0,0,0		/* 36-39 */
  .fill 16,1,0		/* 3A-49 */
  .byte 0,0,0,0,0		/* 4A-4E */
  .byte 0,0,0,0,0,0,0	/* 4F-55 */
  .byte '|
  .fill 10,1,0

#else
#error "KBD-type not defined"
#endif
/*
 * do_self handles "normal" keys, ie keys that don't change meaning
 * and which have just one character returns.
 */

# 根据 mode 标志选择 alt_map shift_map key_map  映射表
do_self:
  lea alt_map,%ebx             # 将 alt_map 地址加载到 ebx 寄存器
  testb $0x20,mode             # 如果 mode & 0x20 为 0 (没有按下)，zero 标志位写入 1, 否则 zero 写入 0 (按下了alt),  用于检测按键 alt 是否按下了
  jne 1f                       # 如果 zero 标志位为 0 跳转到 1f
  lea shift_map,%ebx
  testb $0x03,mode
  jne 1f
  lea key_map,%ebx             # 前面的映射表都没选中，使用普通映射表 key_map
# 根据扫描码取映射表的对应的 ascii 字符，如果找不到返回
1:
  movb (%ebx,%eax),%al         # 将扫描码作为索引取对应 ascii 码保存值 al 寄存器即 al = ebx[eax]，eax 在前面的程序保存了扫描码
  orb %al,%al                  # al = al | al 判断一下有没有取到 ascii 码，如果取到，操作结果不为零，清零 zero flag
  je none                      # jump if zero，如果前面没有找到 acsii 码，跳转到 none
  testb $0x4c,mode             # 检测 ctrl是否按下 或 caps 灯亮
  je 2f                        # jump if zero，没有按下跳转到 2f
  cmpb $'a,%al                 # al 寄存器的值与 字符 a 比较
  jb 2f                        # jump if below， 如果 al < 'a' 跳转到 2f
  cmpb $'},%al                 # al 与 } 比较
  ja 2f                        # jump if above， al 大于 } 跳转到 2f
  subb $32,%al                 # 将 al 转换成大写 al = al - 0x20
2:
  testb $0x0c,mode             # ctrl 按下了吗
  je 3f                        # 没有按下跳转到 3f
  cmpb $64,%al                 # 将 al 与 @ 符号比较
  jb 3f                        # al < @ 跳转到 3f
  cmpb $64+32,%al              # al 与 96 比较，判断字符范围
  jae 3f                       # 如果 a >= 96 跳转 3f
  subb $64,%al                 # al = al - 0x40 转换成 0x0 ~ 0x1f 控制字符
3:
  testb $0x10,mode             # 检查是否按下左 alt
  je 4f                        # 没有按下跳转到 4f
  orb $0x80,%al                # al = al | 0x80 置为 7 位置
# 将 al 中的字符放进读缓冲队列
4:
  andl $0xff,%eax              # 清零 eax 高位 ah
  xorl %ebx,%ebx               # 因为字符数 <= 4，清零 ebx
  call put_queue
none:
  ret

/*
 * minus has a routine of it's own, as a 'E0h' before
 * the scan code for minus means that the numeric keypad
 * slash was pushed.
 */
minus:
  cmpb $1,e0
  jne do_self
  movl $'/,%eax
  xorl %ebx,%ebx
  jmp put_queue

/*
 * This table decides which routine to call when a scan-code has been
 * gotten. Most routines just call do_self, or none, depending if
 * they are make or break.
 */
key_table:
  .long none,do_self,do_self,do_self	/* 00-03 s0 esc 1 2 */
  .long do_self,do_self,do_self,do_self	/* 04-07 3 4 5 6 */
  .long do_self,do_self,do_self,do_self	/* 08-0B 7 8 9 0 */
  .long do_self,do_self,do_self,do_self	/* 0C-0F + ' bs tab */
  .long do_self,do_self,do_self,do_self	/* 10-13 q w e r */
  .long do_self,do_self,do_self,do_self	/* 14-17 t y u i */
  .long do_self,do_self,do_self,do_self	/* 18-1B o p } ^ */
  .long do_self,ctrl,do_self,do_self	/* 1C-1F enter ctrl a s */
  .long do_self,do_self,do_self,do_self	/* 20-23 d f g h */
  .long do_self,do_self,do_self,do_self	/* 24-27 j k l | */
  .long do_self,do_self,lshift,do_self	/* 28-2B { para lshift , */
  .long do_self,do_self,do_self,do_self	/* 2C-2F z x c v */
  .long do_self,do_self,do_self,do_self	/* 30-33 b n m , */
  .long do_self,minus,rshift,do_self	/* 34-37 . - rshift * */
  .long alt,do_self,caps,func		/* 38-3B alt sp caps f1 */
  .long func,func,func,func		/* 3C-3F f2 f3 f4 f5 */
  .long func,func,func,func		/* 40-43 f6 f7 f8 f9 */
  .long func,num,scroll,cursor		/* 44-47 f10 num scr home */
  .long cursor,cursor,do_self,cursor	/* 48-4B up pgup - left */
  .long cursor,cursor,do_self,cursor	/* 4C-4F n5 right + end */
  .long cursor,cursor,cursor,cursor	/* 50-53 dn pgdn ins del */
  .long none,none,do_self,func		/* 54-57 sysreq ? < f11 */
  .long func,none,none,none		/* 58-5B f12 ? ? ? */
  .long none,none,none,none		/* 5C-5F ? ? ? ? */
  .long none,none,none,none		/* 60-63 ? ? ? ? */
  .long none,none,none,none		/* 64-67 ? ? ? ? */
  .long none,none,none,none		/* 68-6B ? ? ? ? */
  .long none,none,none,none		/* 6C-6F ? ? ? ? */
  .long none,none,none,none		/* 70-73 ? ? ? ? */
  .long none,none,none,none		/* 74-77 ? ? ? ? */
  .long none,none,none,none		/* 78-7B ? ? ? ? */
  .long none,none,none,none		/* 7C-7F ? ? ? ? */
  .long none,none,none,none		/* 80-83 ? br br br */
  .long none,none,none,none		/* 84-87 br br br br */
  .long none,none,none,none		/* 88-8B br br br br */
  .long none,none,none,none		/* 8C-8F br br br br */
  .long none,none,none,none		/* 90-93 br br br br */
  .long none,none,none,none		/* 94-97 br br br br */
  .long none,none,none,none		/* 98-9B br br br br */
  .long none,unctrl,none,none		/* 9C-9F br unctrl br br */
  .long none,none,none,none		/* A0-A3 br br br br */
  .long none,none,none,none		/* A4-A7 br br br br */
  .long none,none,unlshift,none		/* A8-AB br br unlshift br */
  .long none,none,none,none		/* AC-AF br br br br */
  .long none,none,none,none		/* B0-B3 br br br br */
  .long none,none,unrshift,none		/* B4-B7 br br unrshift br */
  .long unalt,none,uncaps,none		/* B8-BB unalt br uncaps br */
  .long none,none,none,none		/* BC-BF br br br br */
  .long none,none,none,none		/* C0-C3 br br br br */
  .long none,none,none,none		/* C4-C7 br br br br */
  .long none,none,none,none		/* C8-CB br br br br */
  .long none,none,none,none		/* CC-CF br br br br */
  .long none,none,none,none		/* D0-D3 br br br br */
  .long none,none,none,none		/* D4-D7 br br br br */
  .long none,none,none,none		/* D8-DB br ? ? ? */
  .long none,none,none,none		/* DC-DF ? ? ? ? */
  .long none,none,none,none		/* E0-E3 e0 e1 ? ? */
  .long none,none,none,none		/* E4-E7 ? ? ? ? */
  .long none,none,none,none		/* E8-EB ? ? ? ? */
  .long none,none,none,none		/* EC-EF ? ? ? ? */
  .long none,none,none,none		/* F0-F3 ? ? ? ? */
  .long none,none,none,none		/* F4-F7 ? ? ? ? */
  .long none,none,none,none		/* F8-FB ? ? ? ? */
  .long none,none,none,none		/* FC-FF ? ? ? ? */

/*
 * kb_wait waits for the keyboard controller buffer to empty.
 * there is no timeout - if the buffer doesn't empty, we hang.
 */
kb_wait:
  pushl %eax
1:
  inb $0x64,%al
  testb $0x02,%al
  jne 1b
  popl %eax
  ret
/*
 * This routine reboots the machine by asking the keyboard
 * controller to pulse the reset-line low.
 */
reboot:
  call kb_wait
  movw $0x1234,0x472	/* don't do memory check */
  movb $0xfc,%al		/* pulse reset and A20 low */
  outb %al,$0x64
die:
  jmp die

