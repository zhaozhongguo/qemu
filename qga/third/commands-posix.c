/*
 * QEMU Guest Agent POSIX-specific commands implementations
 *
 * Copyright IBM Corp. 2017
 *
 * Authors:
 *  zhaozhongguo
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */


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


GuestQueryCpuUsage *qmp_guest_query_cpu_usage(int64_t delay, Error **errp)
{
    Error *local_err = NULL;
    char buf[8] = {0,};
    
    calculate_cpu_usage(delay, buf, &local_err);
    if (local_err) {
        error_propagate(errp, local_err);
        return NULL;
    }

    GuestQueryCpuUsage *gusage = g_malloc0(sizeof(*gusage));
    gusage->usage = g_strdup(buf);

    return gusage;
}


