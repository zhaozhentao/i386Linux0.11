.code16

.global _start 

.equ BOOTSEG, 0x07c0		# original address of boot-sector
.equ ROOT_DEV, 0x301

ljmp    $BOOTSEG, $_start
_start:
	mov	$BOOTSEG, %ax	#将ds段寄存器设置为0x7C0

.org 508
root_dev:
  .word ROOT_DEV
boot_flag:
  .word 0xAA55

