.code16                               # 开机时处于 16 位的实模式运行 bootsect 需要以 16 位编译

.global _start 

.equ SETUPLEN, 4                      # nr of setup 模块的扇区数
.equ BOOTSEG, 0x07c0                  # 启动扇区代码的起始地址 0x7c00
.equ INITSEG, 0x9000                  # 启动代码将会搬到 0x90000 这个位置, 注意是 INITSEG 左移 4 位
.equ ROOT_DEV, 0x301

_start:
  mov  $BOOTSEG, %ax                  # 将 ds 数据段寄存器设置为0x7C0
  mov  %ax, %ds
  mov  $INITSEG, %ax                  # 将 es 附加段段寄存器设置为0x900
  mov  %ax, %es
  mov  $256, %cx                      # 设置移动计数值 256 字,即 512 字节
  sub  %si, %si                       # si = si - si 表示将 si 寄存器清零, 因为在当前实模式下 寻址方式为 ds << 4 + si 即 0x7c00 + 0x0
  sub  %di, %di                       # 同上 目标地址为 es << 4 + di 即 0x90000 + 0x0
  rep  movsw                          # 重复执行并递减 cx 的值, 将 (e)cx 个字从 ds:[(e)si] 移到 es:[(e)di]
  ljmp $INITSEG, $go                  # 段间跳转，这里 INITSEG 指出跳转到的段地址，跳转后 cs 代码段寄存器的值为0x9000

go:
  mov  %cs, %ax                       # 将 ds，es，ss 都设置成移动后代码所在的段处 (0x9000)
  mov  %ax, %ds
  mov  %ax, %es
# put stack at 0x9ff00.               # 栈指针通过 ss:sp 确定
  mov  %ax, %ss
  mov  $0xFF00, %sp                   # arbitrary value >> 512

load_setup:                           # 将 setup 模块加载到 bootsect 模块后, 即 0x90200 地址处
  mov  $0x0000, %dx                   # drive 0, head 0
  mov  $0x0002, %cx                   # sector 2, track 0
  mov  $0x0200, %bx                   # address = 512, in INITSEG
  .equ AX, 0x0200 + SETUPLEN
  mov  $AX, %ax                       # service 2, nr of sectors
  int  $0x13                          # 使用 BIOS 读取磁盘功能
  jnc  ok_load_setup                  # 成功读取磁盘内容，跳转到 ok_load_setup
  mov  $0x0000, %dx
  mov  $0x0000, %ax                   # reset the diskette
  int  $0x13
  jmp  load_setup

ok_load_setup:
  mov  $0x00, %dl                     # 调用 BIOS 获取硬盘参数
  mov  $0x0800, %ax                   # AH=8 表示获取硬盘参数
  int  $0x13
  mov  $0x00, %ch
  #seg cs
  mov  %cx, %cs:sectors + 0           # 将保存在 cx 寄存器的磁道数保存在 sectors 中
  mov  $INITSEG, %ax
  mov  %ax, %es                       # 重新将 INITSEG 保存到 es 附加寄存器


# Print some inane message
  mov  $0x03, %ah                     # 读取光标位置
  xor  %bh, %bh
  int  $0x10

  mov  $30, %cx
  mov  $0x0007, %bx                   # page 0, attribute 7 (normal)
  mov  $msg1, %bp
  mov  $0x1301, %ax                   # write string, move cursor
  int  $0x10

sectors:
  .word 0

msg1:
  .byte 13, 10
  .ascii "IceCityOS is booting ..."
  .byte 13, 10, 13, 10

.org 508                              # 此处经过编译后是地址 508 处, 通过后面的标识 55 aa，BIOS 就会认为这是 主引导记录MBR 代码，会将 bootsect 所在的 512 byte加载到内存中运行
root_dev:
  .word ROOT_DEV
boot_flag:
  .word 0xAA55

