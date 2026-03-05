#ifndef GDT_H
#define GDT_H

#include <stdint.h>

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

extern gdt_ptr_t gdtr;

void gdt_init(void);
void gdt_flush(void);

#endif
