/* =============================================================================
 * MyOS - IDT (Interrupt Descriptor Table)
 * ============================================================================= */

#include "../include/idt.h"
#include <stdint.h>

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt[256];
static idt_ptr_t idtr;

extern void idt_handler_stub(void);

void idt_handler(void) {
    /* Handle interrupts here */
}

void idt_set_gate(uint8_t num, uint32_t handler, uint8_t flags) {
    idt[num].offset_low = (handler & 0xFFFF);
    idt[num].offset_high = (handler >> 16) & 0xFFFF;
    idt[num].selector = 0x08;
    idt[num].zero = 0;
    idt[num].type_attr = flags;
}

void pic_init(void) {
    /* Send ICW1 */
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x11), "N"((uint16_t)0x20));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x11), "N"((uint16_t)0xA0));
    
    /* Send ICW2 */
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x20), "N"((uint16_t)0x21));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x28), "N"((uint16_t)0xA1));
    
    /* Send ICW3 */
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x04), "N"((uint16_t)0x21));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x02), "N"((uint16_t)0xA1));
    
    /* Send ICW4 */
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x01), "N"((uint16_t)0x21));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0x01), "N"((uint16_t)0xA1));
    
    /* Mask interrupts */
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0xFF), "N"((uint16_t)0x21));
    __asm__ volatile("outb %0, %1" : : "a"((uint8_t)0xFF), "N"((uint16_t)0xA1));
}

void idt_init(void) {
    idtr.limit = (sizeof(idt_entry_t) * 256) - 1;
    idtr.base = (uint32_t)&idt;
    
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (uint32_t)idt_handler_stub, 0x8E);
    }
    
    __asm__ volatile("lidt (%0)" : : "r"(&idtr));
}
