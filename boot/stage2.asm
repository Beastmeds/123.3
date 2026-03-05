; =============================================================================
; MyOS - Stage 2 Bootloader
; Runs in 16-bit Real Mode, loads kernel, switches to Protected Mode
; =============================================================================

[BITS 16]
[ORG 0x7E00]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7E00
    sti

    ; ----- Print boot message -----
    mov si, msg_stage2
    call print_string_rm

    ; ----- Load kernel from disk at sector 6 (kernel data starts at sector 5+1) -----
    mov ah, 0x02            ; BIOS read sectors
    mov al, 64              ; Load 64 sectors (32KB) for kernel
    mov ch, 0               ; Cylinder 0
    mov cl, 6               ; Sector 6 (starts at physical byte 2560 = sector 5 in 0-indexed)
    mov dh, 0               ; Head 0
    mov dl, 0               ; floppy drive (use default)
    mov bx, 0x10000         ; Load kernel at 0x10000
    int 0x13
    jc disk_error

    mov si, msg_kernel_loaded
    call print_string_rm

    ; ----- Setup GDT -----
    mov si, msg_gdt
    call print_string_rm
    lgdt [gdt_descriptor]

    ; ----- Enable A20 line -----
    mov si, msg_a20
    call print_string_rm
    call enable_a20

    ; ----- Switch to protected mode -----
    mov si, msg_protected
    call print_string_rm
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax
    
    jmp flush_pipeline

flush_pipeline:
    ; ----- Jump to kernel code (far jump) -----
    jmp dword 0x0008:0x10000

; =============================================================================
; Error handling
; =============================================================================
disk_error:
    mov si, msg_disk_error
    call print_string_rm
    jmp $

; =============================================================================
; Enable A20 line (simple keyboard controller method)
; =============================================================================
enable_a20:
    ; Just try the keyboard controller method
    in al, 0x92
    or al, 0x02
    out 0x92, al
    ret

; =============================================================================
; Print string in real mode
; =============================================================================
print_string_rm:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bx, 0x0000
    int 0x10
    jmp print_string_rm
.done:
    mov al, 0x0D
    mov ah, 0x0E
    int 0x10
    mov al, 0x0A
    mov ah, 0x0E
    int 0x10
    ret

; =============================================================================
; GDT (Global Descriptor Table)
; =============================================================================
align 4
gdt:
    ; NULL descriptor
    dq 0x0000000000000000
    
    ; Kernel code (0x08)
    dq 0x00CF9A000000FFFF
    
    ; Kernel data (0x10)
    dq 0x00CF92000000FFFF

gdt_descriptor:
    dw (gdt_descriptor - gdt - 1)
    dd gdt

; =============================================================================
; Data
; =============================================================================
msg_stage2:     db "S2", 0
msg_kernel_loaded: db "K", 0
msg_gdt:        db "G", 0
msg_a20:        db "A", 0
msg_protected:  db "P", 0
msg_disk_error: db "E", 0