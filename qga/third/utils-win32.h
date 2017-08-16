/*
 * QEMU Guest Agent win32-specific utils
 *
 * Copyright IBM Corp. 2017
 *
 * Authors:
 *  zhaozhongguo
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */
#ifndef __UTILS_WIN32_H
#define __UTILS_WIN32_H
#include "utils-common.h"


//disk stat struct
typedef struct 
{
    double rd_ops;
    double wr_ops;
    double rd_octet;
    double wr_octet;
} disk_stat;



//network pdh counter type
enum NET_COUNTER_TYPE
{
    NET_COUNTER_SENT,
    NET_COUNTER_RECV,
};



//network name length
#define MAX_NET_NAME_LEN 128


//network stat struct
struct net_stat
{
    char if_name[MAX_NET_NAME_LEN];
    int type;
    double value;
};


//network stat list struct
struct net_stat_list
{
    struct net_stat* list;
    int length;
    int capacity;
};


#endif


