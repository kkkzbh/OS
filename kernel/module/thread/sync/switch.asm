

global switch_to

[bits 32]
section .text

switch_to:
    ; 栈中 此处为返回地址
    push esi
    push edi
    push ebx
    push ebp  ; 保存内核上下文

    mov eax, [esp + 20] ; 得到栈中的参数cur
    mov [eax], esp      ; task->self_kstack = esp  保存栈顶指针

    ; 上面备份好线程环境，下面恢复下一个线程环境

    mov eax, [esp + 24] ; 得到栈中的参数next, next = [esp + 24]
    mov esp, [eax]      ; pcb的第一个成员是 self_kstack
    pop ebp
    pop ebx
    pop edi
    pop esi
    ret
