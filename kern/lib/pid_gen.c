#include "opt-A2.h"
#include<pid_gen.h>
#include <lib.h>
#include <types.h>

#if OPT_A2
struct pid_counter{
    int pid;
    struct lock* pid_lk;
};
struct pid_counter* pidc_create(int start){
    KASSERT(start > 0);
    struct pid_counter *tmp = kmalloc(sizeof(struct pid_counter));
    tmp->pid = start;
    tmp->pid_lk = lock_create("pid_lock");
    return tmp;
}
void pidc_destroy(struct pid_counter *pidc){
    KASSERT(pidc != NULL);
    lock_destroy(pidc->pid_lk);
    kfree(pidc);
}
int pidc_count(struct pid_counter *pidc){
    KASSERT(pidc != NULL);
    int i;
    lock_acquire(pidc->pid_lk);
    i = pidc->pid;
    pidc->pid++;
    lock_release(pidc->pid_lk);
    return i;
}
#endif