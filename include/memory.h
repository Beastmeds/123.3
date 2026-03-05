#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

void pmm_init(uint32_t total_memory);
void pmm_mark_free(uint32_t start, uint32_t size);
void pmm_mark_used(uint32_t start, uint32_t size);
void* pmm_alloc(uint32_t size);
void pmm_free(void* ptr, uint32_t size);
uint32_t pmm_free_blocks_count(void);
uint32_t pmm_total_blocks(void);

void heap_init(uint32_t start, uint32_t size);
void* kmalloc(size_t size);
void kfree(void* ptr);
void heap_dump(void);

#endif
