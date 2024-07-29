%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR 		   ;保护模式下栈的初始化

jmp loader_start                      		   ;下面存放数据段 构建gdt 跳跃到下面的代码区

				       		   ;对汇编再复习 db define byte,dw define word,dd define dword
    GDT_BASE        : dd 0x00000000          		   ;刚开始的段选择子0不能使用 故用两个双字 来填充
   		       dd 0x00000000 
    
    CODE_DESC       : dd 0x0000FFFF         		   ;FFFF是与其他的几部分相连接 形成0XFFFFF段界限
    		       dd DESC_CODE_HIGH4
    
    DATA_STACK_DESC : dd 0x0000FFFF
  		       dd DESC_DATA_HIGH4
    		       
    VIDEO_DESC      : dd 0x80000007         		   ;0xB8000 到0xBFFFF为文字模式显示内存 B只能在boot.inc中出现定义了 此处不够空间了 8000刚好够
                        dd DESC_VIDEO_HIGH4     	   ;0x0007 (bFFFF-b8000)/4k = 0x7
                 
    GDT_SIZE              equ $ - GDT_BASE               ;当前位置减去GDT_BASE的地址 等于GDT的大小
    GDT_LIMIT       	   equ GDT_SIZE - 1   	           ;SIZE - 1即为最大偏移量
    
    times 59 dq 0                             	   ;预留60个 四字型 描述符
    times 5 db 0


    ; 保存内存容量
    total_mem_bytes dd 0
    ;gdt指针 2字gdt界限放在前面 4字gdt地址放在后面 lgdt 48位格式 低位16位界限 高位32位起始地址
    gdt_ptr           dw GDT_LIMIT
    		       dd GDT_BASE
    
    ; loadermsg db   'welcome to loader zone!'                  ;loadermsg 加载区显示字符
    
; 手动对齐地址 4+6+244+2 = 256  不知道为什么没有算上面的地址
    ards_buf times 244 db 0
    ards_nr dw 0            ; 记录ards结构体的数量

    SELECTOR_CODE        equ (0X0001<<3) + TI_GDT + RPL0    ;16位寄存器 4位TI RPL状态 GDT剩下的选择子
    SELECTOR_DATA	  equ (0X0002<<3) + TI_GDT + RPL0
    SELECTOR_VIDEO       equ (0X0003<<3) + TI_GDT + RPL0

loader_start:

; int 15th eax = 0000E820h, edx = 534D4150h ('SMAP') 字符串ascii值， 获取内存布局 具体注释看书p181

    xor ebx, ebx            ; ebx 置为0
    mov edx, 0x534Dd4150    ; edx 只赋值一次， 后续不会变
    mov di, ards_buf        ; ards结构体缓冲区
.e820_mem_get_loop:         ; 循环获取ards结构体
    mov eax, 0x0000e820     ; 
    mov ecx, 20
    int 0x15
    jc .e820_failed_so_try_e801  ; 判断cf位是否位1， 为1则表示发生错误用下一种方法

    add di, cx              ; 更新缓冲区指针的位置， 指向下一个空白区
    inc word [ards_nr]      ; 数量++
    cmp ebx, 0              ; 如果ebx为0 且cd不为1 说明读取完毕， 进行处理
    jnz .e820_mem_get_loop

    mov cx, [ards_nr]
    mov ebx, ards_buf
    xor ebx, ebx

.find_max_mem_area:
    mov eax, [ebx]
    add eax, [ebx+8]
    add ebx, 20
    cmp edx, eax
    jge .next_ards
    mov edx, eax

.next_ards:
    loop .find_max_mem_area
    jmp .mem_get_ok

; -------int 15h ax = E801h 第二种获取内存的方法 最大支持4g
;   返回后 ax, cx 值一样以kb为单位， bx, dx 值一样单位为64kb
;   ax和cx中为低16MB, bx和dx为16MB到4GB

.e820_failed_so_try_e801:
    mov ax, 0xe801
    int 0x15
    jc .e801_failed_so_try88

; 先计算出低15MB 的内存
; ax和cx中以KB为单位将其转换为byte单位
    mov cx, 0x400
    mul cx
    shl edx, 16
    and eax, 0x0000FFFF
    or edx, eax
    add edx, 0x100000       ;ax只是15MB 需要加上1MB
    mov esi, edx            ; esi暂存备份

; 2 将16MB以上的内存转换为byte单位
; 寄存器bx和dx中存储
    xor eax, eax
    mov ax, bx
    mov ecx, 0x10000        ; 十进制为64KB
    mul ecx                 ; 32位乘法, 默认乘数为eax, 高32为存入edx,低32为存入eax

    mov edx, esi
    add edx, eax            ; 只能测出4GB以内 所以不需要edx寄存器, 只需要加eax

    
    jmp .mem_get_ok

; ---------int 15h ah = 0x88 获取内存大小, 只能获得64MB之内
.e801_failed_so_try88:
    ; int 15 之后ax存入的是以KB为单位的内存容量
    mov ah, 0x88
    int 0x15
    jc .error_hlt
    and eax, 0x0000FFFF
; 16位乘法, 被乘数为ax, 积为32位    积的高16位在dx中, 低16位在ax中
    mov cx, 0x400       ; 十进制为1024 将单位转换为byte
    mul cx
    shl edx, 16         ; 将dx移动到高16位处
    or edx, eax         ; 将低16位移动到edx, 为32位的积
    add edx, 0x100000   ; 加上1MB 得到原本的大小


.error_hlt:
    jmp $

.mem_get_ok:                    ; 内存大小读取完毕
    mov [total_mem_bytes], edx
  
; --------------------------------- 设置进入保护模式 -----------------------------
; 1 打开A20 gate
; 2 加载gdt
; 3 将cr0 的 pe位置1
    
    in al,0x92                 ;端口号0x92 中 第1位变成1即可
    or al,0000_0010b
    out 0x92,al
   
    lgdt [gdt_ptr] 
    
    mov eax,cr0                ;cr0寄存器第0位设置位1  将模式标志设置为1 
    or  eax,0x00000001              
    mov cr0,eax
      
;-------------------------------- 已经打开保护模式 ---------------------------------------
    jmp dword SELECTOR_CODE:p_mode_start                       ;刷新流水线
 
 ; 将选择子加载到段选择器中
 [bits 32]
 p_mode_start: 
    mov ax,SELECTOR_DATA
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov esp,LOADER_STACK_TOP
         
     ;显示“P”到屏幕上
        mov ax, SELECTOR_VIDEO
        mov gs, ax
        mov byte [gs:160], 'P'

;-------------------------------- 加载内核 -----------------------------------------
    mov eax, KERNEL_START_SECTOR
    mov ebx,KERNEL_BIN_BASE_ADDR
    mov ecx, 200

    call rd_disk_m_32

;-------------------------------- 启动分页 -----------------------------------------
    call setup_page

    sgdt [gdt_ptr]

    mov ebx, [gdt_ptr+2]
    or dword [ebx+0x18+4], 0xc0000000

    add dword [gdt_ptr+2], 0xc0000000
    add esp, 0xc0000000

    mov eax, PAGE_DIR_TABLE_POS
    mov cr3, eax

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    lgdt [gdt_ptr]

    mov eax,SELECTOR_VIDEO
    mov gs,eax
    mov byte [gs:320],'V'
    
    jmp SELECTOR_CODE:enter_kernel

enter_kernel:

    call kernel_init
   
    mov esp, 0xc009f000
    jmp KERNEL_ENTRY_POINT

    jmp $


;-------------------------------- 创建页表 ------------------------------------------------    
; 创建页目录和页表
setup_page:                                     ;清空所用空间
    mov ecx, 4096
    mov esi, 0
.clear_page_dir:
    mov byte [PAGE_DIR_TABLE_POS + esi], 0
    inc esi
    loop .clear_page_dir

; 创建页目录项(PDE)
.create_pde:
    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x1000                             ;eax位第一个页表的位置及属性
    mov ebx, eax

    or eax, PG_US_U | PG_RW_W | PG_P

    mov [PAGE_DIR_TABLE_POS + 0x0], eax

    mov [PAGE_DIR_TABLE_POS + 0xc00], eax       ; 0xc00之上属于内核空间

    sub eax, 0x1000
    mov [PAGE_DIR_TABLE_POS + 4092], eax

    mov ecx, 256
    mov esi, 0
    mov edx, PG_US_U | PG_RW_W | PG_P

.create_pte:
    mov [ebx+esi*4], edx                    ;ebx的值为0x1010000 即第一个页表的地址
    add edx, 4096
    inc esi
    loop .create_pte

    mov eax, PAGE_DIR_TABLE_POS
    add eax, 0x2000                         ; 此时eax为第二个页表的位置
    or eax, PG_US_U | PG_RW_W | PG_P
    mov ebx, PAGE_DIR_TABLE_POS
    mov ecx, 254                            ; 这里注意循环的次数为254 避坑256!!
    mov esi, 769

.create_kernal_page:
    mov [ebx+esi*4], eax
    inc esi
    add eax, 0x1000
    loop .create_kernal_page
    ret


;-------------------- 保护模式的硬盘读取函数
rd_disk_m_32:

    mov esi, eax
    mov di, cx

    mov dx, 0x1f2
    mov al, cl
    out dx, al

    mov eax, esi

    mov dx, 0x1f3
    out dx, al

    mov cl, 8
    shr eax, cl
    mov dx, 0x1f4
    out dx, al

    shr eax, cl
    mov dx, 0x1f5
    out dx, al

    shr eax, cl
    and al, 0x0f
    or al, 0xe0
    mov dx, 0x1f6
    out dx, al

    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

.not_ready:
    nop
    in al, dx
    and al, 0x88
    cmp al, 0x08
    jnz .not_ready

    mov ax, di
    mov dx, 256
    mul dx
    mov cx, ax
    mov dx, 0x1f0

.go_on_read:
    in ax, dx
    mov [ebx], ax
    add ebx, 2
    loop .go_on_read
    ret

; ------------------- 硬盘读取函数结束

; ------------------- 将kernel.bin 中的segment拷贝到编译的地址
kernel_init:
    xor eax, eax
    xor ebx, ebx
    xor ecx, ecx
    xor edx, edx

    mov dx, [KERNEL_BIN_BASE_ADDR + 42]     
    mov ebx, [KERNEL_BIN_BASE_ADDR + 28]

    add ebx, KERNEL_BIN_BASE_ADDR
    mov cx, [KERNEL_BIN_BASE_ADDR + 44]

.each_segment:
    cmp byte [ebx+0], PT_NULL
    je .PTNULL

    push dword [ebx+16]

    mov eax, [ebx+4]
    add eax, KERNEL_BIN_BASE_ADDR

    push eax
    push dword [ebx+8]

    call mem_cpy
    add esp, 12
.PTNULL:
    add ebx, edx

    loop .each_segment
    ret

; 逐字节拷贝 mem_cpy(dst, src, size)
mem_cpy:
    cld
    push ebp
    mov ebp, esp
    push ecx

    mov edi, [ebp + 8]
    mov esi, [ebp + 12]
    mov ecx, [ebp + 16]
    rep movsb

    pop ecx
    pop ebp
    ret

; ------------------- 拷贝结束
