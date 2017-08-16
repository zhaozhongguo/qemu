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

#include "utils-posix.h"


GuestQueryCpuUsage *qmp_guest_query_cpu_usage(int64_t delay, Error **errp)
{
    if (delay <= 0)
    {
        delay = DEFAULT_DELAY;
    }

    Error *local_err = NULL;
    char buf[8] = {0,};
    
    calculate_cpu_usage(delay, buf, &local_err);
    if (local_err) 
    {
        error_propagate(errp, local_err);
        return NULL;
    }

    GuestQueryCpuUsage *gusage = g_malloc0(sizeof(*gusage));
    gusage->usage = g_strdup(buf);

    return gusage;
}



GuestQueryMemUsage *qmp_guest_query_mem_usage(Error **errp)
{
    Error *local_err = NULL;
    char buf[8] = {0,};

    calculate_mem_usage(buf, &local_err);
    if (local_err) 
    {
        error_propagate(errp, local_err);
        return NULL;
    }

    GuestQueryMemUsage *gusage = g_malloc0(sizeof(*gusage));
    gusage->usage = g_strdup(buf);

    return gusage;
}



GuestQueryDiskStat *qmp_guest_query_disk_stat(int64_t delay, Error **errp)
{
    if (delay <= 0)
    {
        delay = DEFAULT_DELAY;
    }

    Error *local_err = NULL;

    struct timeval tv;
    int dev_nr = get_file_row_num("/proc/diskstats");
    if (dev_nr <= 0)
    {
        error_setg(errp, "no device.");
        return NULL;
    }
    
    struct disk_stat_list* old_disk_list = read_diskstats(dev_nr, &local_err);
    if (local_err) 
    {
        error_propagate(errp, local_err);
        return NULL;
    }
    
    gettimeofday(&tv,NULL);
    double start = tv.tv_sec + tv.tv_usec/1000000.0;
    
    sleep(delay);
    
    struct disk_stat_list* new_disk_list = read_diskstats(dev_nr, &local_err);
    if (local_err)
    {
        error_propagate(errp, local_err);
        STAT_LIST_FREE(old_disk_list);
        return NULL;
    }

    gettimeofday(&tv, NULL);
    double end = tv.tv_sec + tv.tv_usec/1000000.0;
    double time_diff = end - start;
    
    if (new_disk_list->length != old_disk_list->length)
    {
        STAT_LIST_FREE(new_disk_list);
        STAT_LIST_FREE(old_disk_list);
        error_setg(errp, "disk device info changed.");
        return NULL;
    }
    
    int index = 0;
    double rd_ops = 0.0;
    double wr_ops = 0.0;
    double rd_octet = 0.0;
    double wr_octet = 0.0;
    char str_rd_ops[64] = {0,};
    char str_wr_ops[64] = {0,};
    char str_rd_octet[64] = {0,};
    char str_wr_octet[64] = {0,};
    struct disk_stat* old_stat = NULL;
    struct disk_stat* new_stat = NULL;

    GuestQueryDiskStat *guest_stat = g_new0(GuestQueryDiskStat, 1);
    GuestDiskStat* guest_disk_stat = NULL;
    GuestDiskStatList* guest_disk_stat_list = NULL;

    for (; index < new_disk_list->length; index++)
    {
        old_stat = &(old_disk_list->list[index]);
        new_stat = &(new_disk_list->list[index]);
        if (0 != strncmp(old_stat->dk_name, new_stat->dk_name, MAX_DISK_NAME_LEN))
        {
            break;
        }
        
        rd_ops = (double)(new_stat->rd_ios - old_stat->rd_ios) / time_diff;
        wr_ops = (double)(new_stat->wr_ios - old_stat->wr_ios) / time_diff;
        rd_octet = (double)(new_stat->rd_sectors - old_stat->rd_sectors) / time_diff * 512.0;  //one sector => 512byte
        wr_octet = (double)(new_stat->wr_sectors - old_stat->wr_sectors) / time_diff * 512.0;
        
        sprintf(str_rd_ops, "%.1f", rd_ops);
        sprintf(str_wr_ops, "%.1f", wr_ops);
        sprintf(str_rd_octet, "%.1f", rd_octet);
        sprintf(str_wr_octet, "%.1f", wr_octet);
 
        guest_disk_stat = g_new0(GuestDiskStat, 1);
        guest_disk_stat->name = g_strdup(new_stat->dk_name);
        guest_disk_stat->rd_ops = g_strdup(str_rd_ops);
        guest_disk_stat->wr_ops = g_strdup(str_wr_ops);
        guest_disk_stat->rd_octet = g_strdup(str_rd_octet);
        guest_disk_stat->wr_octet = g_strdup(str_wr_octet);

        guest_disk_stat_list = g_new0(GuestDiskStatList, 1);
        guest_disk_stat_list->value = guest_disk_stat;
        guest_disk_stat_list->next = guest_stat->disk_stat;
        guest_stat->disk_stat = guest_disk_stat_list;
        
        memset(str_rd_ops, 0, 64);
        memset(str_wr_ops, 0, 64);
        memset(str_rd_octet, 0, 64);
        memset(str_wr_octet, 0, 64);
    }
    
    
    STAT_LIST_FREE(new_disk_list);
    STAT_LIST_FREE(old_disk_list);

    return guest_stat; 
}




GuestQueryNetStat *qmp_guest_query_net_stat(int64_t delay, Error **errp)
{
    if (delay <= 0)
    {
        delay = DEFAULT_DELAY;
    }

    Error *local_err = NULL;

    struct timeval tv;
    int dev_nr = get_file_row_num("/proc/net/dev");
    if (dev_nr <= 0)
    {
        error_setg(errp, "no net if.");
        return NULL;
    }
    
    struct net_stat_list* old_net_list = read_netstats(dev_nr, &local_err);
    if (local_err) 
    {
        error_propagate(errp, local_err);
        return NULL;
    }
    
    gettimeofday(&tv,NULL);
    double start = tv.tv_sec + tv.tv_usec/1000000.0;
    
    sleep(delay);
    
    struct net_stat_list* new_net_list = read_netstats(dev_nr, &local_err);
    if (local_err)
    {
        error_propagate(errp, local_err);
        STAT_LIST_FREE(old_net_list);
        return NULL;
    }

    gettimeofday(&tv, NULL);
    double end = tv.tv_sec + tv.tv_usec/1000000.0;
    double time_diff = end - start;
    
    if (new_net_list->length != old_net_list->length)
    {
        STAT_LIST_FREE(new_net_list);
        STAT_LIST_FREE(old_net_list);
        error_setg(errp, "net if info changed.");
        return NULL;
    }
    
    int index = 0;
    double receive_byte = 0.0;
    double send_byte = 0.0;
    char str_receive_byte[64] = {0,};
    char str_send_byte[64] = {0,};
    struct net_stat* old_stat = NULL;
    struct net_stat* new_stat = NULL;

    GuestQueryNetStat *guest_stat = g_new0(GuestQueryNetStat, 1);
    GuestNetStat* guest_net_stat = NULL;
    GuestNetStatList* guest_net_stat_list = NULL;
    for (; index < new_net_list->length; index++)
    {
        old_stat = &(old_net_list->list[index]);
        new_stat = &(new_net_list->list[index]);
        if (0 != strncmp(old_stat->if_name, new_stat->if_name, MAX_NET_NAME_LEN))
        {
            break;
        }
        
        receive_byte = (double)(new_stat->if_ibytes- old_stat->if_ibytes) / time_diff;
        send_byte = (double)(new_stat->if_obytes - old_stat->if_obytes) / time_diff;
        
        sprintf(str_receive_byte, "%.1f", receive_byte);
        sprintf(str_send_byte, "%.1f", send_byte);
 
        guest_net_stat = g_new0(GuestNetStat, 1);
        guest_net_stat->name = g_strdup(new_stat->if_name);
        guest_net_stat->receive = g_strdup(str_receive_byte);
        guest_net_stat->send = g_strdup(str_send_byte);

        guest_net_stat_list = g_new0(GuestNetStatList, 1);
        guest_net_stat_list->value = guest_net_stat;
        guest_net_stat_list->next = guest_stat->net_stat;
        guest_stat->net_stat = guest_net_stat_list;
        
        memset(str_receive_byte, 0, 64);
        memset(str_send_byte, 0, 64);
    }
    
    
    STAT_LIST_FREE(new_net_list);
    STAT_LIST_FREE(old_net_list);

    return guest_stat; 
}

