;------------------- 进入loader所需要的宏 --------------------------

LOADER_START_SECTOR equ 2
LOADER_BASE_ADDR equ 0x900 

;-------------------- gdt描述符属性 --------------------------------

DESC_G_4K equ 1_00000000000000000000000b ;第23位G 段界限的单位大小， 0表示1字节，大小为1MB  1代表4kb字节， 大小为4GB
DESC_D_32 equ 1_0000000000000000000000b  ;第22位D/B位 代码段来说：指令的操作位数，1表示32位 用eip， 0表示16位用ip寄存器  栈段：0表示16位的寻址范围sp， 1表示32位寻址esp
DESC_L    equ 0_000000000000000000000b   ;第21位 选择代码的位数， 32设置0， 64设置1
DESC_AVL  equ 0_00000000000000000000b    ;第20位 对用户来说是否可用， 操作系统不关心
                                         
DESC_LIMIT_CODE2  equ  1111_0000000000000000b   ;第16-19位 段界限的最后四位 全部初始化为1 因为最大段界限*粒度必须等于0xffffffff
DESC_LIMIT_DATA2  equ  DESC_LIMIT_CODE2         ;相同的值  数据段与代码段段界限相同
DESC_LIMIT_VIDEO2 equ	0000_0000000000000000b	  ;第16-19位 显存区描述符VIDEO2 书上后面的0少打了一位 这里的全是0为高位 低位即可表示段基址
   
DESC_P            equ 	1_000000000000000b	  ;第15位  P present判断段是否存在于内存  由cpu检查
DESC_DPL_0        equ  00_0000000000000b         ;第13-14位 dpl字段 描述符特权级
DESC_DPL_1        equ  01_0000000000000b	  ;0为操作系统 权力最高 3为用户段 用于保护
DESC_DPL_2        equ  10_0000000000000b
DESC_DPL_3        equ  11_0000000000000b

DESC_S_sys        equ  0_000000000000b           ;第12位为 0为系统段， 1为非系统段
DESC_S_CODE       equ  1_000000000000b           ;第12位与type字段结合 判断是否为系统段还是数据段
DESC_S_DATA       equ  DESC_S_CODE


DESC_TYPE_CODE    equ  1000_00000000b            ;第9-11位表示该段状态 1000 可执行 不允许可读 已访问位0
;x=1 e=0 w=0 a=0
DESC_TYPE_DATA    equ  0010_00000000b            ;第9-11位type段   0010  可写  
;x=0 e=0 w=1 a=0


;代码段描述符高位4字节初始化 (0x00共8位 <<24 共32位初始化0) 
;4KB为单位 Data段32位操作数 初始化的部分段界限 最高权限操作系统代码段 P存在表示 状态 
DESC_CODE_HIGH4   equ  (0x00<<24) + DESC_G_4K + DESC_D_32 + \
DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + \
DESC_P + DESC_DPL_0 + DESC_S_CODE + DESC_TYPE_CODE + 0X00

;数据段描述符高位4字节初始化
DESC_DATA_HIGH4   equ  (0x00<<24) + DESC_G_4K + DESC_D_32 + \
DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + \
DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0X00

;显存段描述符高位4字节初始ua
DESC_VIDEO_HIGH4   equ (0x00<<24) + DESC_G_4K + DESC_D_32 + \
DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + \
DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0X0B  ;整挺好 我看书上写的0x00 结果我自己推算出来这里末尾是B 

;-------------------- 选择子属性 --------------------------------
;第0-1位 RPL 特权级比较是否允许访问  第2位TI 0表示GDT 1表示LDT    第3-15位索引值
RPL0    equ 00b
RPL1    equ 01b
RPL2    equ 10b
RPL3    equ 11b
TI_GDT  equ 000b
TI_LDT  equ 100b
