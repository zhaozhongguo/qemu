/*
 * QEMU Guest Agent POSIX-specific utils
 *
 * Copyright IBM Corp. 2017
 *
 * Authors:
 *  zhaozhongguo
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include "qapi/error.h"

//cpu stat struct
struct cpu_stat 
{    
    long long user;
    long long nice;
    long long sys;
    long long idle;
    long long wait;
    long long irq;
    long long softirq;
    long long steal;
    long long guest;
    long long guest_nice;
};

//read cpu stat from /proc/stat
void read_cpu_stat(struct cpu_stat* stat, Error **errp);

//calculate cpu usage
void calculate_cpu_usage(int delay, char *usage, Error **errp);

