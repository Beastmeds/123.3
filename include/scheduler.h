#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#define PRIORITY_HIGHEST  0
#define PRIORITY_HIGH     1
#define PRIORITY_NORMAL   2
#define PRIORITY_LOW      3
#define PRIORITY_LOWEST   4

typedef struct task_t {
    uint32_t id;
    char name[64];
    uint32_t priority;
    void (*entry)(void);
    uint32_t esp;
    uint32_t state;
} task_t;

void sched_init(void);
task_t* task_create(const char* name, void (*entry)(void), uint32_t priority);
void task_ps(void);

#endif
