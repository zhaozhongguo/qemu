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
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "utils-posix.h"


void read_cpu_stat(struct cpu_stat* stat, Error **errp)
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
    if (local_err) {
        error_propagate(errp, local_err);
        return;
    }

    sleep(delay);
    
    struct cpu_stat new_cpu_stat;
    read_cpu_stat(&new_cpu_stat, &local_err);
    if (local_err) {
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

