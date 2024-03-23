# TODO: This is lab1.1
/* Real Mode Hello World */
/*
.code16

.global start
start:
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	movw $0x7d00, %ax
	movw %ax, %sp # setting stack pointer to 0x7d00
	pushw $13 # size
	pushw $message # address of message
	callw printStr

loop:
	jmp loop

message:
	.string "Hello, World!\n\0"

printStr:
	pushw %bp

	movw 4(%esp), %ax
	movw %ax, %bp
	movw 6(%esp), %cx
	movw $0x1301, %ax
	movw $0x0049, %bx
	movw $0x0000, %dx
	int $0x10
	popw %bp
	ret
*/

# TODO: This is lab1.2
/* Protected Mode Hello World */
/*
.code16

.global start
start:
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	# TODO:关闭中断
	cli

	# 启动A20总线
	inb $0x92, %al 
	orb $0x02, %al
	outb %al, $0x92

	# 加载GDTR
	data32 addr32 lgdt gdtDesc # loading gdtr, data32, addr32

	# TODO：设置CR0的PE位（第0位）为1
	movl %cr0, %eax    # 读取CR0到EAX
	orl $0x1, %eax     # 设置PE位
	movl %eax, %cr0    # 写回CR0


	# 长跳转切换至保护模式
	data32 ljmp $0x08, $start32 # reload code segment selector and ljmp to start32, data32

.code32
start32:
	movw $0x10, %ax # setting data segment selector
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %ss
	movw $0x18, %ax # setting graphics data segment selector
	movw %ax, %gs
	
	movl $0x8000, %eax # setting esp
	movl %eax, %esp
	# TODO:输出Hello World
    movl $0xB8000, %edi    # 显存开始地址
    movl $message, %esi    # 字符串的地址

printStr:
    lodsb                  # 将字符串的下一个字节加载到AL
    test %al, %al          # 检查是否到达字符串末尾（AL=0）
    jz loop32              # 如果是，跳转到完成标签
    movb %al, (%edi)       # 将字符写入显存
    addl $2, %edi          # 移动到下一个字符位置（跳过属性字节）
    jmp printStr           # 继续下一个字符

loop32:
	jmp loop32

message:
	.string "Hello, World!\n\0"



.p2align 2
gdt: # 8 bytes for each table entry, at least 1 entry
	# .word limit[15:0],base[15:0]
	# .byte base[23:16],(0x90|(type)),(0xc0|(limit[19:16])),base[31:24]
	# GDT第一个表项为空
	.word 0,0
	.byte 0,0,0,0

	# TODO：code segment entry
	.word 0xFFFF, 0x0000      # 段界限的低16位，基地址的低16位
	.byte 0x00, 0x9A, 0xCF, 0x00  # 基地址的第3字节，访问标志，段界限的高4位+标志，基地址的高字节

	# TODO：data segment entry
	.word 0xFFFF, 0x0000      # 段界限的低16位，基地址的低16位
	.byte 0x00, 0x92, 0xCF, 0x00  # 基地址的第3字节，访问标志，段界限的高4位+标志，基地址的高字节
	
	# TODO：graphics segment entry
	.word 0xFFFF, 0x8000      # 段界限的低16位，基地址的低16位
	.byte 0x0B, 0x92, 0xCF, 0x00  # 基地址的第3字节，访问标志，段界限的高4位+标志，基地址的高字节

gdtDesc: 
	.word (gdtDesc - gdt -1) 
	.long gdt 
*/


# TODO: This is lab1.3
/* Protected Mode Loading Hello World APP */

.code16

.global start
start:
	movw %cs, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %ss
	# TODO:关闭中断
	cli

	# 启动A20总线
	inb $0x92, %al 
	orb $0x02, %al
	outb %al, $0x92

	# 加载GDTR
	data32 addr32 lgdt gdtDesc # loading gdtr, data32, addr32

	# TODO：设置CR0的PE位（第0位）为1
	movl %cr0, %eax    # 读取CR0到EAX
	orl $0x1, %eax     # 设置PE位
	movl %eax, %cr0    # 写回CR0

	# 长跳转切换至保护模式
	data32 ljmp $0x08, $start32 # reload code segment selector and ljmp to start32, data32

.code32
start32:
	movw $0x10, %ax # setting data segment selector
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %ss
	movw $0x18, %ax # setting graphics data segment selector
	movw %ax, %gs
	
	movl $0x8000, %eax # setting esp
	movl %eax, %esp
	jmp bootMain # jump to bootMain in boot.c

.p2align 2
gdt: # 8 bytes for each table entry, at least 1 entry
	# .word limit[15:0],base[15:0]
	# .byte base[23:16],(0x90|(type)),(0xc0|(limit[19:16])),base[31:24]
	# GDT第一个表项为空
	.word 0,0
	.byte 0,0,0,0

	# TODO：code segment entry
	.word 0xFFFF, 0x0000      # 段界限的低16位，基地址的低16位
	.byte 0x00, 0x9A, 0xCF, 0x00  # 基地址的第3字节，访问标志，段界限的高4位+标志，基地址的高字节

	# TODO：data segment entry
	.word 0xFFFF, 0x0000      # 段界限的低16位，基地址的低16位
	.byte 0x00, 0x92, 0xCF, 0x00  # 基地址的第3字节，访问标志，段界限的高4位+标志，基地址的高字节
	
	# TODO：graphics segment entry
	.word 0xFFFF, 0x8000      # 段界限的低16位，基地址的低16位
	.byte 0x0B, 0x92, 0xCF, 0x00  # 基地址的第3字节，访问标志，段界限的高4位+标志，基地址的高字节


gdtDesc: 
	.word (gdtDesc - gdt - 1) 
	.long gdt 
