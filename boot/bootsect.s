.code16                               # 开机时处于 16 位的实模式运行 bootsect 需要以 16 位编译

.global _start 

.equ BOOTSEG, 0x07c0		
.equ ROOT_DEV, 0x301

ljmp    $BOOTSEG, $_start

_start:
	mov	$BOOTSEG, %ax	              #将ds段寄存器设置为0x7C0

.org 508                              # 此处经过编译后是地址 508 处, 通过后面的标识 55 aa，BIOS 就会认为这是 主引导记录MBR 代码，会将 bootsect 所在的 512 byte加载到内存中运行
root_dev:
  .word ROOT_DEV
boot_flag:
  .word 0xAA55

