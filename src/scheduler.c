#include "scheduler.h"

#include "memory.h"
#include "string.h"
#include "sys.h"

typedef struct {
    unsigned long ticks;
    unsigned long value;
} CounterCtx;

typedef struct {
    unsigned long ticks;
    unsigned long checksum;
} SpinnerCtx;

static Task *g_tasks;
static int g_next_id = 1;

static void counter_step(Task *self, void *ctx) {
    CounterCtx *c = (CounterCtx *)ctx;
    (void)self;
    c->ticks++;
    c->value++;
}

static void spinner_step(Task *self, void *ctx) {
    SpinnerCtx *s = (SpinnerCtx *)ctx;
    unsigned long i;
    (void)self;
    s->ticks++;
    for (i = 0; i < 4096UL; i++)
        s->checksum = (s->checksum * 1315423911UL + i) ^ (s->checksum >> 3);
}

static void task_free(Task *t) {
    if (!t)
        return;
    if (t->ctx)
        my_free(t->ctx);
    my_free(t);
}

void scheduler_init(void) {
    Task *t;
    Task *n;

    for (t = g_tasks; t; t = n) {
        n = t->next;
        task_free(t);
    }
    g_tasks = NULL;
    g_next_id = 1;
}

static void task_append(Task *t) {
    t->next = g_tasks;
    g_tasks = t;
}

void scheduler_tick(void) {
    Task *t;

    for (t = g_tasks; t; t = t->next) {
        if (!t->alive)
            continue;
        if (t->step)
            t->step(t, t->ctx);
    }
}

int scheduler_spawn_bg(const char *kind) {
    Task *t;

    t = (Task *)my_malloc(sizeof(Task));
    if (!t)
        return -1;
    my_memset(t, 0, sizeof(*t));
    t->id = g_next_id++;
    t->alive = 1;

    if (my_strcmp(kind, "counter") == 0) {
        CounterCtx *c = (CounterCtx *)my_malloc(sizeof(CounterCtx));
        if (!c) {
            my_free(t);
            return -1;
        }
        my_memset(c, 0, sizeof(*c));
        t->ctx = c;
        t->step = counter_step;
        t->name = "counter";
    } else if (my_strcmp(kind, "spinner") == 0) {
        SpinnerCtx *s = (SpinnerCtx *)my_malloc(sizeof(SpinnerCtx));
        if (!s) {
            my_free(t);
            return -1;
        }
        my_memset(s, 0, sizeof(*s));
        t->ctx = s;
        t->step = spinner_step;
        t->name = "spinner";
    } else {
        my_free(t);
        return -1;
    }

    task_append(t);
    return 0;
}

void scheduler_dump_ps(void) {
    Task *t;

    if (!g_tasks) {
        print("(no background tasks)\n");
        return;
    }

    print("ID  NAME      STATE  DETAIL\n");
    for (t = g_tasks; t; t = t->next) {
        print(" ");
        print_int((long)t->id);
        print("  ");
        print(t->name);
        print("  ");
        print(t->alive ? "run" : "off");
        print("  ");

        if (my_strcmp(t->name, "counter") == 0 && t->ctx) {
            CounterCtx *c = (CounterCtx *)t->ctx;
            print("ticks=");
            print_uint(c->ticks);
            print(" value=");
            print_uint(c->value);
        } else if (my_strcmp(t->name, "spinner") == 0 && t->ctx) {
            SpinnerCtx *s = (SpinnerCtx *)t->ctx;
            print("ticks=");
            print_uint(s->ticks);
            print(" xor=");
            print_hex(s->checksum);
        }

        print("\n");
    }
}
