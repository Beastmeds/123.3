/* =============================================================================
 * MyOS - Task Scheduler
 * ============================================================================= */

#include "../include/scheduler.h"
#include "../include/memory.h"

static task_t task_table[256];
static uint32_t task_count = 0;

void sched_init(void) {
    task_count = 0;
}

task_t* task_create(const char* name, void (*entry)(void), uint32_t priority) {
    if (task_count >= 256) return 0;
    
    task_t* task = &task_table[task_count];
    task->id = task_count;
    task->priority = priority;
    task->entry = entry;
    task->state = 1;
    
    const char* s = name;
    int i = 0;
    while (*s && i < 63) {
        task->name[i++] = *s++;
    }
    task->name[i] = 0;
    
    task_count++;
    return task;
}

void task_ps(void) {
    /* List tasks */
}
