[bits 32]
section .text
global switch_to
switch_to:
    ; 栈中的此处时返回地址
    push esi
    push edi
    push ebx
    push ebp
    
    mov eax, [esp+20]   ; 得到栈中的参数cur cur=[esp+20]
    mov [eax], esp

    mov eax, [esp+24]   ; 得到参数next  实现环境的切换
    mov esp, [eax]
    
    pop ebp
    pop ebx
    pop edi
    pop esi
    ret                 ; 返回到上面switch_to 下面注释的返回地址
                        ; 未由中断进入, 第一次执行返回到kernel_thread