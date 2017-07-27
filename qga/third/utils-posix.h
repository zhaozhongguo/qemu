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


#define DEFAULT_DELAY 5

#undef isdigit
#define isdigit(ch) ( ( '0' <= (ch)  &&  (ch) >= '9')? 0: 1 )


//malloc for stat list
#define STAT_LIST_ALLOCATE(type, length, out_pList) \
    do {\
        if (length <= 0) \
        { \
            out_pList = NULL; \
            break; \
        } \
        \
        struct type##_list* t_list = (struct type##_list*)malloc(sizeof(struct type##_list)); \
        if (NULL == t_list) \
        { \
            out_pList = NULL; \
            break; \
        } \
        memset((char*)t_list, 0, sizeof(struct type##_list)); \
        \
        int size = sizeof(struct type) * length; \
        t_list->list = (struct type*) malloc(size); \
        if (NULL != t_list->list) \
        { \
            memset((char*)t_list->list, 0, size); \
            t_list->capacity = length; \
        } \
        else \
        { \
            free(t_list); \
            out_pList = NULL; \
            break; \
        } \
        \
        out_pList = t_list; \
    } while (0)


//free for stat list
#define STAT_LIST_FREE(pList) \
    do { \
        if (NULL != pList) \
        { \
            if (NULL != pList->list) \
            { \
                free(pList->list); \
            } \
            \
            free(pList); \
        } \
    } while (0)



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
