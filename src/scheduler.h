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

/* Spawn a named background task. Known names: "counter", "spinner". */
int  scheduler_spawn_bg(const char *name);

/* Kill a running task by its numeric ID. Returns 0 on success, -1 if not found. */
int  scheduler_kill(int id);

void scheduler_dump_ps(void);

#endif
