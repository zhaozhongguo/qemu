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
    if (delay <= 0)
    {
        delay = DEFAULT_DELAY;
    }

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



GuestQueryMemUsage *qmp_guest_query_mem_usage(Error **errp)
{
    Error *local_err = NULL;
    char buf[8] = {0,};

    calculate_mem_usage(buf, &local_err);
    if (local_err) {
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
    int dev_nr = get_diskstats_dev_nr();
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
        free_disk_list(old_disk_list);
        return NULL;
    }

    gettimeofday(&tv, NULL);
    double end = tv.tv_sec + tv.tv_usec/1000000.0;
    double time_diff = end - start;
    
    if (new_disk_list->length != old_disk_list->length)
    {
        free_disk_list(new_disk_list);
        free_disk_list(old_disk_list);
        error_setg(errp, "disk device info changed.");
        return NULL;
    }
    
    int index = 0;
    double rd_ops = 0.0;
    double wr_ops = 0.0;
    double rd_kbs = 0.0;
    double wr_kbs = 0.0;
    char str_rd_ops[64] = {0,};
    char str_wr_ops[64] = {0,};
    char str_rd_kbs[64] = {0,};
    char str_wr_kbs[64] = {0,};
    struct disk_stat* old_stat = NULL;
    struct disk_stat* new_stat = NULL;

    GuestQueryDiskStat *guest_stat = g_new0(GuestQueryDiskStat, 1);
    GuestDiskStat* guest_disk_stat = NULL;
    GuestDiskStatList* guest_disk_stat_list = NULL;

    for (; index < new_disk_list->length; index++)
    {
        old_stat = &(old_disk_list->list[index]);
        new_stat = &(new_disk_list->list[index]);
        if (0 != strncmp(old_stat->dk_name, new_stat->dk_name, MAX_NAME_LEN))
        {
            break;
        }
        
        rd_ops = (double)(new_stat->rd_ios - old_stat->rd_ios) / time_diff;
        wr_ops = (double)(new_stat->wr_ios - old_stat->wr_ios) / time_diff;
        rd_kbs = (double)(new_stat->rd_sectors - old_stat->rd_sectors) / time_diff / 2.0;  //one sector => 512byte
        wr_kbs = (double)(new_stat->wr_sectors - old_stat->wr_sectors) / time_diff / 2.0;
        
        sprintf(str_rd_ops, "%.2f", rd_ops);
        sprintf(str_wr_ops, "%.2f", wr_ops);
        sprintf(str_rd_kbs, "%.2f", rd_kbs);
        sprintf(str_wr_kbs, "%.2f", wr_kbs);
 
        guest_disk_stat = g_new0(GuestDiskStat, 1);
        guest_disk_stat->name = g_strdup(new_stat->dk_name);
        guest_disk_stat->rd_ops = g_strdup(str_rd_ops);
        guest_disk_stat->wr_ops = g_strdup(str_wr_ops);
        guest_disk_stat->rd_kbs = g_strdup(str_rd_kbs);
        guest_disk_stat->wr_kbs = g_strdup(str_wr_kbs);

        guest_disk_stat_list = g_new0(GuestDiskStatList, 1);
        guest_disk_stat_list->value = guest_disk_stat;
        guest_disk_stat_list->next = guest_stat->disk_stat;
        guest_stat->disk_stat = guest_disk_stat_list;
        
        memset(str_rd_ops, 0, 64);
        memset(str_wr_ops, 0, 64);
        memset(str_rd_kbs, 0, 64);
        memset(str_wr_kbs, 0, 64);
    }
    
    
    free_disk_list(new_disk_list);
    free_disk_list(old_disk_list);

    return guest_stat; 
}
