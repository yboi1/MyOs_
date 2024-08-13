TI_GDT equ 0
RPL0 equ 0
SELECTOR_VIDEO equ (0x0003<<3) + TI_GDT + RPL0

section .data
put_int_buffer dq 0

[bits 32]
section .text

; ----------------- put_str 函数 打印0字符结尾的字符串
global put_str
put_str:
    push ebx
    push ecx
    xor ecx, ecx
    mov ebx, [esp+12]       ; 12: 备份了两个寄存器八字节, 返回地址四字节

.goon:
    mov cl, [ebx]           ; 读取一个字符
    cmp cl, 0
    jz .str_over
    push ecx                ; ecx 是put_char函数的传入参数
    call put_char
    add esp, 4              ; 回收参数占用的4字节内存
    inc ebx                 ; ebx+1 指向下一个字符
    jmp .goon

.str_over:                  ; 恢复现场
    pop ecx
    pop ebx
    ret


; ----------------- 将字符写入光标所在处
global put_char
put_char:
    pushad                  ; 备注32位寄存器环境 push all double

    mov ax, SELECTOR_VIDEO  ; 段寄存器不能用立即数直接赋值
    mov gs, ax

;   获取当前坐标
    ; 获取高八位
    mov dx, 0x03d4          ; 索引寄存器
    mov al, 0x0e            ; 提供光标位置的高八位
    out dx, al
    mov dx, 0x03d5          ; 通过读写端口得到
    in al, dx
    mov ah, al

    ; 获取低八位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    in al, dx

    ; 将光标存入bx
    mov bx, ax
    ; 在栈中获取待打印的字符
    mov ecx, [esp + 36]     ; pushad 压入32字节, 加上主调函数4字节返回地址, 所以esp+36

    cmp cl, 0xd             ; 删除为0x0d, 换行为0x0a
    jz .is_carriage_return
    cmp cl, 0xa
    jz .is_line_feed

    cmp cl, 0x8
    jz .is_backspace
    jmp .put_other

; -------函数实现处 处理删除, 回车
.is_backspace:
    dec bx
    shl bx, 1                   ; 光标左移一位等于*2
                                ; 表示光标对应显存中的偏移字节
    mov byte [gs:bx], 0x20      ; 将待删除的字节补为0或值空格
    inc bx
    mov byte [gs:bx], 0x07
    shr bx, 1
    jmp set_cursor

.put_other:
    shl bx, 1

    mov [gs:bx], cl             ; ASCII字符本身
    inc bx
    mov byte [gs:bx], 0x07      ; 字符属性
    shr bx, 1                   ; 恢复老的光标值
    inc bx
    cmp bx, 2000                ; 判断光标值是否超过屏幕字符数大小
    jl set_cursor

.is_line_feed:                  ; 换行LF
.is_carriage_return:            ; 回车CR

    xor dx, dx                  ; dx是被除数的高16位, 清0   
    mov ax, bx                  ; ax是被除数的低16位
    mov si, 80

    div si                      

    sub bx, dx                  ; 光标值减去除80的余数便是取整

.is_carriage_return_end:        ; 回车CR处理结束
    add bx, 80
    cmp bx, 2000    
.is_line_feed_end:              ; 若是LF(\n) 将光标移动80即可
    jl set_cursor

.roll_screen:                   ; 若超出屏幕大小, 开始滚屏
    cld                         ; 清除方向位 DF设为0
    mov ecx, 960                ; 2000-80=1920个字符需要搬运, 共1920*2=3850个字节
                                ; 一次4字节, 需要3850/4=960次
    mov esi, 0xc00b80a0         ; 第一行行首
    mov edi, 0xc00b8000         ; 第零行行首
    rep movsd

;; 将最后一行填充为空白
    mov ebx, 3840
    mov ecx, 80

.cls:
    mov word [gs:ebx], 0x0720   ; 0x0720 是黑底白字的空格键
    add ebx, 2
    loop .cls
    mov bx, 1920                ; 光标值重置为1920, 最后一行的首字符

;将光标设为bx值
global set_cursor
set_cursor:
    ; 设置高八位
    mov dx, 0x03d4              ; 索引寄存器
    mov al, 0x0e                ; 提供光标的高八位
    out dx, al
    mov dx, 0x03d5              ; 通过读写数据端口 0x3d5 来获得或设置光标的位置
    mov al, bh
    out dx, al

    ; 设置低八位
    mov dx, 0x03d4
    mov al, 0x0f
    out dx, al
    mov dx, 0x03d5
    mov al, bl
    out dx, al

.put_char_done:
    popad
    ret

; ----------将小端字节序的数字变成对应的ASCII后, 倒置
;
;
;
;
;

global put_int
put_int:
    pushad
    mov ebp, esp
    mov eax, [ebp+4*9]          ;call返回地址四个字节, pushad的8个4字节
    mov edx, eax
    mov edi, 7                  ;指定put_int_buffer 中初始化的偏移量
    mov ecx, 8                  ; 32位数字中, 16进制的数字位数为8
    mov ebx, put_int_buffer

.16based_4bits:                 ; 32位数字按照16进制的形式从低位到高位逐个处理 
                                ; 遍历每一位16进制数字
    and edx, 0x0000000F         ; 得到最后一位数字

    cmp edx, 9                  ; 将参数分为0-9 和 a-f
    jg .is_A2F
    add edx, '0'                ; ASCII8位大小, add之后 edx低8位有效
    jmp .store
.is_A2F:
    sub edx, 10                 ; A-F 减去10得到的差, 加上字符A为其对应的ASCII值
    add edx, 'A'

.store:                         ; 大端存储, 高位字符存储到低地址
    mov [ebx+edi], dl
    dec edi
    shr eax, 4
    mov edx, eax
    loop .16based_4bits

.ready_to_print:
    inc edi                     ; 经过前面的步骤, edi变成-1, 现在恢复为0, 向指向缓冲区做准备
.skip_prefix_0:
    cmp edi, 8

    je .full0

.go_on_skip:
    mov cl, [put_int_buffer+edi]
    inc edi
    cmp cl, '0'
    je .skip_prefix_0
    dec edi

    jmp .put_each_num

.full0:
    mov cl, '0'
.put_each_num:
    push ecx
    call put_char
    add esp, 4
    inc edi
    mov cl, [put_int_buffer+edi]
    cmp edi, 8
    jl .put_each_num
    popad
    ret




