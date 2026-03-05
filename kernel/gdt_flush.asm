; =============================================================================
; MyOS - GDT Flush
; Loads the new GDT
; =============================================================================

[BITS 32]

global gdt_flush
extern gdtr

gdt_flush:
    ; Load GDTR from memory
    lgdt [rel gdtr]
    
    ; Reload code segment with kernel code selector (0x08)
    jmp 0x08:.reload_cs
    
.reload_cs:
    ; Reload data segments with kernel data selector (0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ret
