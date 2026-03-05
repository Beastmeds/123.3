/* =============================================================================
 * MyOS - Kernel Linker Script
 * Produces a raw binary suitable for loading at 0x10000
 * ============================================================================= */

OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS
{
    /* Start at offset 0 in the binary (will be loaded at 0x10000 by bootloader) */
    . = 0;
    
    /* First, the kernel entry code */
    .text : {
        *(.text.entry)
        *(.text*)
    }
    
    /* Read-only data */
    .rodata : {
        *(.rodata*)
    }
    
    /* Initialized data */
    .data : {
        *(.data*)
    }
    
    /* Uninitialized data and common symbols */
    .bss : {
        *(.bss*)
        *(COMMON)
    }
    
    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.note*)
        *(.comment*)
        *(.rel*)
        *(.rela*)
    }
}
