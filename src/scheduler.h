#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "minios_types.h"

typedef struct Task Task;

typedef void (*TaskStepFn)(Task *self, void *ctx);

struct Task {
    int id;
    int alive;
    const char *name;
    TaskStepFn step;
    void *ctx;
    Task *next;
};

void scheduler_init(void);
void scheduler_tick(void);

/* Returns 0 on success. Known names: "counter", "spinner". */
int scheduler_spawn_bg(const char *name);

void scheduler_dump_ps(void);

#endif
