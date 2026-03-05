; =============================================================================
; MyOS - Kernel Entry Point (Protected Mode Entry)
; This is jumped to from Stage 2 bootloader
; =============================================================================

[BITS 32]

extern kernel_main

section .text
global _start

_start:
    ; We're now in Protected Mode
    ; Segments are already loaded by bootloader
    ; Stack pointer is already set
    
    ; Clear interrupts
    cli
    
    ; Call kernel_main (C function)
    call kernel_main
    
    ; If kernel_main returns (shouldn't happen), hang
.hang:
    hlt
    jmp .hang
