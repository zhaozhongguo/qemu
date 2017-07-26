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
#ifndef __UTILS_POSIX_H
#define __UTILS_POSIX_H

#include "qemu/osdep.h"
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <dirent.h>
#include "qga/guest-agent-core.h"
#include "qga-qmp-commands.h"
#include "qapi/qmp/qerror.h"
#include "qemu/queue.h"
#include "qemu/host-utils.h"
#include "qemu/sockets.h"
#include "qemu/base64.h"
#include "qemu/cutils.h"
#include "utils-posix.h"

#ifndef CONFIG_HAS_ENVIRON
#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern char **environ;
#endif
#endif

#if defined(__linux__)
#include <mntent.h>
#include <linux/fs.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <net/if.h>
#endif


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

void calculate_cpu_usage(int delay, char *usage, Error **errp);


//mem info struct
struct mem_stat {
    long memtotal;
    long memfree;
};

void calculate_mem_usage(char *usage, Error **errp);


//disk info struct


#endif
