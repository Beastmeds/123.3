; =============================================================================
; MyOS - IDT Handler Common
; Minimal interrupt handling stub
; =============================================================================

[BITS 32]

extern idt_handler

global idt_handler_stub

idt_handler_stub:
    ; All registers are already on the stack from the IDT
    pusha
    mov ax, ds
    push eax
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    call idt_handler
    
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa
    add esp, 8
    iret
