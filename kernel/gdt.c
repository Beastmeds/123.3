/* =============================================================================
 * MyOS - GDT (Global Descriptor Table)
 * ============================================================================= */

#include "../include/gdt.h"

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t limit_high;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

static gdt_entry_t gdt[6];
gdt_ptr_t gdtr;  /* Made global for gdt_flush.asm */

extern void gdt_flush(void);

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_mid = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].limit_high = (limit >> 16) & 0x0F;
    gdt[num].access = access;
    gdt[num].limit_high |= (gran & 0xF0);
}

void gdt_init(void) {
    gdtr.limit = (sizeof(gdt_entry_t) * 6) - 1;
    gdtr.base = (uint32_t)&gdt;
    
    /* NULL descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);
    
    /* Kernel code (0x08) */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    
    /* Kernel data (0x10) */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    
    /* User code (0x18) */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    
    /* User data (0x20) */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);
    
    /* TSS (0x28) */
    gdt_set_gate(5, 0, 0, 0x89, 0x40);
    
    gdt_flush();
}
