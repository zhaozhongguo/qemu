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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include "utils-posix.h"
#include "qemu/osdep.h"
#include "qga/guest-agent-core.h"
#include "qemu-common.h"




int get_file_row_num(const char* filename)
{
    FILE *fp;
    char line[1024];
    if (NULL == (fp = fopen(filename, "r")))
    {
        return 0;
    }
    
    int dev = 0;
    //counting net if is simply a matter of counting the number of lines...
    while (NULL != fgets(line, sizeof(line), fp)) 
    {
        dev++;
    }
    
    fclose(fp);
    
    return dev;

}





static void read_cpu_stat(struct cpu_stat* stat, Error **errp)
{
    if (NULL == stat)
    {
        error_setg(errp, "invalid param: stat");
        return;
    }
    memset((char*)stat, 0, sizeof(struct cpu_stat));
    
    FILE *fd;
    char buff[256] = {0,};
    fd = fopen("/proc/stat", "r");
    if (!fd)
    {
        error_setg(errp, "failed to open /proc/stat.");
        return;
    }

    int ready = 0;
    while(fgets(buff, sizeof(buff), fd)) 
    {
        if(NULL != strstr(buff, "cpu")) 
        {
            ready = 1;
            break;
        }
        memset(buff, 0, sizeof(buff));
    }
    
    if(!ready)
    {
        error_setg(errp, "failed to read cpu line from /proc/stat.");
        fclose(fd);
        return;
    }
    
    int seq = 0;
    char* p = strtok(buff, " ");
    while (NULL != p)
    {
        switch (seq)
        {
            case 1:
            {
                sscanf(p, "%lld", &stat->user);
                break;
            }
            case 2:
            {
                sscanf(p, "%lld", &stat->nice);
                break;
            }
            case 3:
            {
                sscanf(p, "%lld", &stat->sys);
                break;
            }
            case 4:
            {
                sscanf(p, "%lld", &stat->idle);
                break;
            }
            case 5:
            {
                sscanf(p, "%lld", &stat->wait);
                break;
            }
            case 6:
            {
                sscanf(p, "%lld", &stat->irq);
                break;
            }
            case 7:
            {
                sscanf(p, "%lld", &stat->softirq);
                break;
            }
            case 8:
            {
                sscanf(p, "%lld", &stat->steal);
                break;
            }
            case 9:
            {
                sscanf(p, "%lld", &stat->guest);
                break;
            }
            case 10:
            {
                sscanf(p, "%lld", &stat->guest_nice);
                break;
            }
            default:
            {
                break;
            }
        }

        p = strtok(NULL, " ");
        seq++;
    }

    fclose(fd);
}


void calculate_cpu_usage(int delay, char *usage, Error **errp)
{
    if (NULL == usage)
    {
        error_setg(errp, "invalid param: usage");
        return;
    }

    Error *local_err = NULL;
    
    struct cpu_stat old_cpu_stat;
    read_cpu_stat(&old_cpu_stat, &local_err);
    if (local_err) 
    {
        error_propagate(errp, local_err);
        return;
    }

    sleep(delay);
    
    struct cpu_stat new_cpu_stat;
    read_cpu_stat(&new_cpu_stat, &local_err);
    if (local_err) 
    {
        error_propagate(errp, local_err);
        return;
    }

    long long int old_total = old_cpu_stat.user + old_cpu_stat.nice + old_cpu_stat.sys + old_cpu_stat.idle + old_cpu_stat.wait 
                + old_cpu_stat.irq + old_cpu_stat.softirq + old_cpu_stat.steal + old_cpu_stat.guest + old_cpu_stat.guest_nice;
    long long int new_total = new_cpu_stat.user + new_cpu_stat.nice + new_cpu_stat.sys + new_cpu_stat.idle + new_cpu_stat.wait 
                + new_cpu_stat.irq + new_cpu_stat.softirq + new_cpu_stat.steal + new_cpu_stat.guest + new_cpu_stat.guest_nice;
                
    long long int diff_total = new_total - old_total;
    long long int diff_busy = diff_total - (new_cpu_stat.idle - old_cpu_stat.idle);
    double cpu_usage = (double)diff_busy / (double)diff_total * 100.0;
    
    sprintf(usage, "%.1f", cpu_usage);
}



static long mem_search(char *src)
{
    int i;
    for (i = 0; !isdigit(src[i]) && src[i] != 0; i++);
    return atol(&src[i]);
}



static void read_mem_info(struct mem_stat* mem, Error **errp)
{
    if (NULL == mem)
    {
        error_setg(errp, "invalid param: mem.");
        return;
    }
    memset((char*)mem, 0, sizeof(*mem));
    
    char buff[64] = {0,};
    FILE *fd = fopen("/proc/meminfo", "r");
    if (!fd)
    {
        error_setg(errp, "failed to open /proc/meminfo.");
        return;
    }

    int finished = 0;
    while (fgets(buff, sizeof(buff), fd)) 
    {
        if (NULL != strstr(buff, "MemTotal")) 
        {
            mem->memtotal = mem_search(buff);
            finished++;
        }
        else if (NULL != strstr(buff, "MemFree"))
        {
            mem->memfree = mem_search(buff);
            finished++;
        }
        
        if (finished >= 2)
        {
            break;
        }
        memset(buff, 0, sizeof(buff));
    }
    
    fclose(fd);
}


void calculate_mem_usage(char *usage, Error **errp)
{
    if (NULL == usage)
    {
        error_setg(errp, "invalid param: usage.");
        return;
    }

    Error *local_err = NULL;
    struct mem_stat mem;
    read_mem_info(&mem, &local_err);
    if (local_err) 
    {
        error_propagate(errp, local_err);
        return;
    }

    double mem_usage = (double)(mem.memtotal - mem.memfree) / (double)mem.memtotal * 100.0;
    sprintf(usage, "%.1f", mem_usage);
}



//test whether given name is a device or a partition
static int is_device(char *name, int allow_virtual)
{
    char syspath[PATH_MAX];
    char *slash;

    /* Some devices may have a slash in their name (eg. cciss/c0d0...) */
    while ((slash = strchr(name, '/')))
    {
        *slash = '!';
    }
    snprintf(syspath, sizeof(syspath), "%s/%s%s", "/sys/block", name,
         allow_virtual ? "" : "/device");

    return !(access(syspath, F_OK));
}



//read disk stats
struct disk_stat_list* read_diskstats(int length, Error **errp)
{
    FILE *fp;
    if (NULL == (fp = fopen("/proc/diskstats", "r")))
    {
        error_setg(errp, "failed to open /proc/diskstats.");
        return NULL;
    }

    struct disk_stat_list* disk_list = NULL;
    STAT_LIST_ALLOCATE(disk_stat, length, disk_list);
    if (NULL == disk_list) 
    {
        error_setg(errp, "failed to malloc disk_stat_list.");
        fclose(fp);
        return NULL;
    }

    char line[256] = {0,};
    int ret = 0;
    
    unsigned int major = 0;
    unsigned int minor = 0;
    char dev_name[MAX_DISK_NAME_LEN] = {0,};
    unsigned long ios_pgr = 0;
    unsigned long tot_ticks = 0;
    unsigned long rq_ticks = 0;
    unsigned long wr_ticks = 0;
    unsigned long rd_ios = 0;
    unsigned long rd_merges_or_rd_sec = 0;
    unsigned long wr_ios = 0;
    unsigned long wr_merges = 0;
    unsigned long rd_sec_or_wr_ios = 0;
    unsigned long wr_sec = 0;
    unsigned long rd_ticks_or_wr_sec = 0;
    struct disk_stat* pStat = NULL;
    while (NULL != fgets(line, sizeof(line), fp)) 
    {
        // major minor name rio rmerge rsect ruse wio wmerge wsect wuse running use aveq
        ret = sscanf(line, "%u %u %s %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
            &major, &minor, dev_name,
            &rd_ios, &rd_merges_or_rd_sec, &rd_sec_or_wr_ios, &rd_ticks_or_wr_sec,
            &wr_ios, &wr_merges, &wr_sec, &wr_ticks, &ios_pgr, &tot_ticks, &rq_ticks);
        
        memset(line, 0, 256);

        if (ret == 14) 
        {
            //only disk device selectd
            if (!is_device(dev_name, TRUE))
            {
                continue;
            }
            
            if (disk_list->length < disk_list->capacity)
            {
                pStat = &(disk_list->list[disk_list->length++]);
                pStat->major      = major;
                pStat->minor      = minor;
                strncpy(pStat->dk_name, dev_name, MAX_DISK_NAME_LEN - 1);
                pStat->rd_ios     = rd_ios;
                pStat->rd_merges  = rd_merges_or_rd_sec;
                pStat->rd_sectors = rd_sec_or_wr_ios;
                pStat->rd_ticks   = rd_ticks_or_wr_sec;
                pStat->wr_ios     = wr_ios;
                pStat->wr_merges  = wr_merges;
                pStat->wr_sectors = wr_sec;
                pStat->wr_ticks   = wr_ticks;
                pStat->ios_pgr    = ios_pgr;
                pStat->tot_ticks  = tot_ticks;
                pStat->rq_ticks   = rq_ticks;
                
            }
            else
            {
                break;
            }
        }
    }
    
    fclose(fp);
    
    return disk_list;
}




struct net_stat_list* read_netstats(int length, Error **errp)
{
    FILE *fp;
    if (NULL == (fp = fopen("/proc/net/dev", "r")))
    {
        error_setg(errp, "failed to open /proc/net/dev.");
        return NULL;
    }

    char line[1024] = {0,};
    //throw away the header tow lines
    if (NULL == fgets(line, 1024, fp))
    {
        error_setg(errp, "failed to open /proc/net/dev.");
        fclose(fp);
        return NULL;
    }
    
    if (NULL == fgets(line, 1024, fp))
    {
        error_setg(errp, "failed to open /proc/net/dev.");
        fclose(fp);
        return NULL;
    }
    memset(line, 0, 1024);

    struct net_stat_list* net_list = NULL;
    STAT_LIST_ALLOCATE(net_stat, length, net_list);
    if (NULL == net_list) 
    {
        error_setg(errp, "failed to malloc net_stat_list.");
        fclose(fp);
        return NULL;
    }
    
    char if_name[MAX_NET_NAME_LEN];
    unsigned long long if_ibytes;
    unsigned long long if_obytes;
    unsigned long long if_ipackets;
    unsigned long long if_opackets;
    unsigned long if_ierrs;
    unsigned long if_oerrs;
    unsigned long if_idrop;
    unsigned long if_ififo;
    unsigned long if_iframe;
    unsigned long if_odrop;
    unsigned long if_ofifo;
    unsigned long if_ocarrier;
    unsigned long if_ocolls;
    unsigned long junk;

    int ret = 0;
    struct net_stat* pStat = NULL;
    while (NULL != fgets(line, sizeof(line), fp)) 
    {
        // face bytes packets errs drop fifo frame compressed multicast bytes packets errs drop fifo colls carrier
        ret = sscanf(line, "%s %llu %llu %lu %lu %lu %lu %lu %lu %llu %llu %lu %lu %lu %lu %lu",
               if_name, &if_ibytes, &if_ipackets, &if_ierrs, &if_idrop, &if_ififo, &if_iframe, &junk, 
               &junk, &if_obytes, &if_opackets, &if_oerrs, &if_odrop, &if_ofifo, &if_ocolls, &if_ocarrier);

        memset(line, 0, 1024);
        if (ret != 16)
        {
            continue;
        }

        if (net_list->length < net_list->capacity)
        {
            pStat = &(net_list->list[net_list->length++]);
            strncpy(pStat->if_name, if_name, MAX_NET_NAME_LEN - 1);
            pStat->if_ibytes   = if_ibytes;
            pStat->if_obytes   = if_obytes;
            pStat->if_ipackets = if_ipackets;
            pStat->if_opackets = if_opackets;
            pStat->if_ierrs    = if_ierrs;
            pStat->if_oerrs    = if_oerrs;
            pStat->if_idrop    = if_idrop;
            pStat->if_ififo    = if_ififo;
            pStat->if_iframe   = if_iframe;
            pStat->if_odrop    = if_odrop;
            pStat->if_ofifo    = if_ofifo;
            pStat->if_ocarrier = if_ocarrier;
            pStat->if_ocolls   = if_ocolls;
        }
        else
        {
            break;
        }

    }
    
    fclose(fp);
    
    return net_list;

}

