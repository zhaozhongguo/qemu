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
#include "utils-posix.h"


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


