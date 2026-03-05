; =============================================================================
; MyOS - Stage 1 Bootloader (MBR - 512 bytes)
; Loads Stage 2 from disk, switches to Protected Mode
; Assembled with: nasm -f bin boot.asm -o boot.bin
; =============================================================================

[BITS 16]
[ORG 0x7C00]

STAGE2_LOAD_SEG  equ 0x0000
STAGE2_LOAD_OFF  equ 0x7E00   ; Right after MBR
STAGE2_SECTORS   equ 4        ; How many sectors to load for stage2

start:
    ; -------------------------------------------------------------------------
    ; Setup segments and stack
    ; -------------------------------------------------------------------------
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00          ; Stack grows downward from boot sector
    sti

    ; Save boot drive
    mov [boot_drive], dl

    ; -------------------------------------------------------------------------
    ; Print boot message
    ; -------------------------------------------------------------------------
    mov si, msg_boot
    call print_string_rm

    ; -------------------------------------------------------------------------
    ; Load Stage 2 from disk using BIOS INT 13h
    ; -------------------------------------------------------------------------
    mov ah, 0x02            ; BIOS read sectors function
    mov al, STAGE2_SECTORS  ; Number of sectors
    mov ch, 0               ; Cylinder 0
    mov cl, 2               ; Sector 2 (sectors start at 1)
    mov dh, 0               ; Head 0
    mov dl, [boot_drive]
    mov bx, STAGE2_LOAD_OFF ; Buffer offset
    int 0x13
    jc disk_error

    mov si, msg_loaded
    call print_string_rm

    ; -------------------------------------------------------------------------
    ; Jump to Stage 2
    ; -------------------------------------------------------------------------
    jmp STAGE2_LOAD_OFF

; =============================================================================
; print_string_rm - Print null-terminated string in real mode
; SI = string pointer
; =============================================================================
print_string_rm:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print_string_rm
.done:
    ret

; =============================================================================
; disk_error - Handle disk read failure
; =============================================================================
disk_error:
    mov si, msg_disk_err
    call print_string_rm
    hlt

; =============================================================================
; Data
; =============================================================================
boot_drive  db 0
msg_boot    db "MyOS Bootloader v1.0", 0x0D, 0x0A, 0
msg_loaded  db "Stage 2 loaded. Jumping...", 0x0D, 0x0A, 0
msg_disk_err db "DISK ERROR! Halting.", 0x0D, 0x0A, 0

; =============================================================================
; Boot signature
; =============================================================================
times 510 - ($ - $$) db 0
dw 0xAA55
