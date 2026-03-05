#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include <stddef.h>

#define PHYS_MEM_MAX    0x02000000  /* 32 MB */
#define HEAP_START      0x01000000
#define HEAP_SIZE       0x00100000  /* 1 MB */

#define PANIC(msg) kernel_panic(msg, __FILE__, __LINE__)

void kernel_panic(const char* msg, const char* file, int line);
void kernel_main(void);

/* Printf function */
int kprintf(const char* fmt, ...);

#endif
