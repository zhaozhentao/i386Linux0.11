.code16                               # 开机时处于 16 位的实模式运行 bootsect 需要以 16 位编译

.global _start 

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
  rep  movs                           # 重复执行并递减cx的值, 将 (e)cx 个字从 ds:[(e)si] 移到 es:[(e)di]
  ljmp $INITSEG, $go                  # 段间跳转，这里 INITSEG 指出跳转到的段地址，跳转后 cs 代码段寄存器的值为0x9000

go:
  mov  %cs, %ax                       # 将 ds，es，ss 都设置成移动后代码所在的段处 (0x9000)
  mov  %ax, %ds
  mov  %ax, %es
# put stack at 0x9ff00.               # 栈指针通过 ss:sp 确定
  mov  %ax, %ss
  mov  $0xFF00, %sp                   # arbitrary value >> 512

.org 508                              # 此处经过编译后是地址 508 处, 通过后面的标识 55 aa，BIOS 就会认为这是 主引导记录MBR 代码，会将 bootsect 所在的 512 byte加载到内存中运行
root_dev:
  .word ROOT_DEV
boot_flag:
  .word 0xAA55

