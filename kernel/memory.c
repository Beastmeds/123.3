/* =============================================================================
 * MyOS - Physical Memory Manager
 * ============================================================================= */

#include "../include/memory.h"

#define BLOCK_SIZE 4096
#define BLOCKS_MAX 8192

static uint32_t* pmm_bitmap;
static uint32_t total_mem;
static uint32_t used_blocks;

void pmm_init(uint32_t total_memory) {
    total_mem = total_memory;
    used_blocks = total_mem / BLOCK_SIZE;
    
    pmm_bitmap = (uint32_t*)0x00101000;
    for (uint32_t i = 0; i < (used_blocks / 32 + 1); i++) {
        pmm_bitmap[i] = 0xFFFFFFFF;
    }
}

void pmm_mark_free(uint32_t start, uint32_t size) {
    uint32_t align = start / BLOCK_SIZE;
    uint32_t blocks = size / BLOCK_SIZE;
    
    for (uint32_t i = 0; i < blocks; i++) {
        uint32_t bit = (align + i) % 32;
        uint32_t idx = (align + i) / 32;
        pmm_bitmap[idx] &= ~(1 << bit);
    }
}

void pmm_mark_used(uint32_t start, uint32_t size) {
    uint32_t align = start / BLOCK_SIZE;
    uint32_t blocks = size / BLOCK_SIZE;
    
    for (uint32_t i = 0; i < blocks; i++) {
        uint32_t bit = (align + i) % 32;
        uint32_t idx = (align + i) / 32;
        pmm_bitmap[idx] |= (1 << bit);
    }
}

void* pmm_alloc(uint32_t size) {
    uint32_t blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    for (uint32_t i = 0; i < (total_mem / BLOCK_SIZE); i++) {
        uint32_t idx = i / 32;
        uint32_t bit = i % 32;
        
        if ((pmm_bitmap[idx] & (1 << bit)) == 0) {
            uint32_t found = 1;
            for (uint32_t j = 1; j < blocks; j++) {
                uint32_t idx2 = (i + j) / 32;
                uint32_t bit2 = (i + j) % 32;
                if (pmm_bitmap[idx2] & (1 << bit2)) {
                    found = 0;
                    break;
                }
            }
            
            if (found) {
                pmm_mark_used(i * BLOCK_SIZE, size);
                return (void*)(i * BLOCK_SIZE);
            }
        }
    }
    
    return 0;
}

void pmm_free(void* ptr, uint32_t size) {
    pmm_mark_free((uint32_t)ptr, size);
}

uint32_t pmm_free_blocks_count(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < (total_mem / BLOCK_SIZE); i++) {
        uint32_t idx = i / 32;
        uint32_t bit = i % 32;
        if ((pmm_bitmap[idx] & (1 << bit)) == 0) {
            count++;
        }
    }
    return count;
}

uint32_t pmm_total_blocks(void) {
    return total_mem / BLOCK_SIZE;
}

/* ===== Heap Allocator ===== */

typedef struct {
    uint32_t size;
    uint32_t used;
    void* start;
} heap_t;

static heap_t heap;

void heap_init(uint32_t start, uint32_t size) {
    heap.start = (void*)start;
    heap.size = size;
    heap.used = 0;
}

void* kmalloc(size_t size) {
    if (heap.used + size > heap.size) return 0;
    
    void* ptr = (void*)((uint32_t)heap.start + heap.used);
    heap.used += size;
    return ptr;
}

void kfree(void* ptr) {
    /* Simplified - no free */
}

void heap_dump(void) {
    /* Simplified */
}
