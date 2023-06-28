#ifndef _PID_GEN_H_
#define _PID_GEN_H_
#include "opt-A2.h"

#include<synch.h>

#if OPT_A2
struct pid_counter;
struct pid_counter* pidc_create(int start);
void pidc_destroy(struct pid_counter *pidc);
int pidc_count(struct pid_counter *pidc);
#endif

#endif