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
#include "qemu/typedefs.h"
#include "utils-common.h"


//query file row number
int get_file_row_num(const char* filename);



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
struct mem_stat 
{
    long memtotal;
    long memfree;
};

void calculate_mem_usage(char *usage, Error **errp);


//disk info struct
#define MAX_DISK_NAME_LEN 32

struct disk_stat 
{
    unsigned int major;
    unsigned int minor;
    char dk_name[MAX_DISK_NAME_LEN];
    unsigned long rd_ios;
    unsigned long rd_merges;
    unsigned long rd_sectors;
    unsigned long rd_ticks;
    unsigned long wr_ios;
    unsigned long wr_merges;
    unsigned long wr_sectors;
    unsigned long wr_ticks;
    unsigned long ios_pgr;
    unsigned long tot_ticks;
    unsigned long rq_ticks;
};


struct disk_stat_list
{
    struct disk_stat* list;
    int length;
    int capacity;
};


//read disk stats
struct disk_stat_list* read_diskstats(int length, Error **errp);


//net info struct
#define MAX_NET_NAME_LEN 32

struct net_stat 
{
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
};

struct net_stat_list
{
    struct net_stat* list;
    int length;
    int capacity;
};


//read net stats
struct net_stat_list* read_netstats(int length, Error **errp);


#endif
