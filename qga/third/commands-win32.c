/*
 * QEMU Guest Agent win32-specific command implementations
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
#include <wtypes.h>
#include <powrprof.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iptypes.h>
#include <iphlpapi.h>
#ifdef CONFIG_QGA_NTDDSCSI
#include <winioctl.h>
#include <ntddscsi.h>
#include <setupapi.h>
#include <initguid.h>
#endif
#include <lm.h>

#include "qga/guest-agent-core.h"
#include "qga/vss-win32.h"
#include "qga-qmp-commands.h"
#include "qapi/qmp/qerror.h"
#include "qemu/queue.h"
#include "qemu/host-utils.h"
#include "utils-win32.h"



int calculate_cpu_usage(int delay, char *usage);

int calculate_mem_usage(char *usage);

int calculate_disk_usage(int delay, disk_stat* stat);

struct net_stat_list* calculate_network_usage(int delay);


GuestQueryCpuUsage *qmp_guest_query_cpu_usage(int64_t delay, Error **errp)
{
    if (delay <= 0)
    {
        delay = DEFAULT_DELAY;
    }

    char buf[8] = {0,};
    if (FALSE == calculate_cpu_usage(delay, buf)) 
    {
        error_setg(errp, "failed to calculate_cpu_usage.");
        return NULL;
    }

    GuestQueryCpuUsage *gusage = g_malloc0(sizeof(*gusage));
    gusage->usage = g_strdup(buf);

    return gusage;
}



GuestQueryMemUsage *qmp_guest_query_mem_usage(Error **errp)
{
    char buf[8] = {0,};
    if (FALSE == calculate_mem_usage(buf)) 
    {
        error_setg(errp, "failed to calculate_mem_usage.");
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

    disk_stat stat;
    if(FALSE == calculate_disk_usage(delay, &stat))
    {
        error_setg(errp, "failed to calculate_disk_usage.");
        return NULL;
    }
    
    char str_rd_ops[64] = {0,};
    char str_wr_ops[64] = {0,};
    char str_rd_octet[64] = {0,};
    char str_wr_octet[64] = {0,};

    sprintf(str_rd_ops, "%.1f", stat.rd_ops);
    sprintf(str_wr_ops, "%.1f", stat.wr_ops);
    sprintf(str_rd_octet, "%.1f", stat.rd_octet);
    sprintf(str_wr_octet, "%.1f", stat.wr_octet);

    GuestQueryDiskStat *guest_stat = g_new0(GuestQueryDiskStat, 1);
    GuestDiskStat* guest_disk_stat = g_new0(GuestDiskStat, 1);
    guest_disk_stat->name = g_strdup("total");
    guest_disk_stat->rd_ops = g_strdup(str_rd_ops);
    guest_disk_stat->wr_ops = g_strdup(str_wr_ops);
    guest_disk_stat->rd_octet = g_strdup(str_rd_octet);
    guest_disk_stat->wr_octet = g_strdup(str_wr_octet);
    
    GuestDiskStatList* guest_disk_stat_list = g_new0(GuestDiskStatList, 1);
    guest_disk_stat_list->value = guest_disk_stat;
    guest_disk_stat_list->next = guest_stat->disk_stat;
    guest_stat->disk_stat = guest_disk_stat_list;

    return guest_stat; 
}



static GuestNetStat* find_net_stat_node(GuestQueryNetStat *guest_stat, char *if_name)
{
    GuestNetStatList* head = guest_stat->net_stat;
    GuestNetStat* stat = NULL;
    while (head)
    {
        stat = head->value;
        if (NULL != stat)
        {
            if ( 0 == strncmp(stat->name, if_name, MAX_NET_NAME_LEN - 1))
            {
                return stat;
            }
        }
        
        head = head->next;
    }
    
    return NULL;
}


GuestQueryNetStat *qmp_guest_query_net_stat(int64_t delay, Error **errp)
{
    if (delay <= 0)
    {
        delay = DEFAULT_DELAY;
    }
    
    struct net_stat_list* net_list = calculate_network_usage(delay);
    if (NULL == net_list) 
    {
        error_setg(errp, "failed to calculate_network_usage.");
        return NULL;
    }
    
    int index = 0;
    char str_receive_byte[64] = {0,};
    char str_send_byte[64] = {0,};
    struct net_stat* pStat = NULL;
    GuestQueryNetStat *guest_stat = g_new0(GuestQueryNetStat, 1);
    GuestNetStat* guest_net_stat = NULL;
    GuestNetStatList* guest_net_stat_list = NULL;
    for (; index < net_list->length; index++)
    {
        pStat = &(net_list->list[index]);
        guest_net_stat = find_net_stat_node(guest_stat, pStat->if_name);
        if (NULL == guest_net_stat)
        {
            guest_net_stat = g_new0(GuestNetStat, 1);
            guest_net_stat_list = g_new0(GuestNetStatList, 1);
            guest_net_stat_list->value = guest_net_stat;
            guest_net_stat_list->next = guest_stat->net_stat;
            guest_stat->net_stat = guest_net_stat_list;
            
            guest_net_stat->name = g_strdup(pStat->if_name);
        }
       
        if (NET_COUNTER_SENT == pStat->type)
        {
            sprintf(str_send_byte, "%.1f", pStat->value);
            guest_net_stat->send = g_strdup(str_send_byte);
        }
        else if (NET_COUNTER_RECV == pStat->type)
        {
            sprintf(str_receive_byte, "%.1f", pStat->value);
            guest_net_stat->receive = g_strdup(str_receive_byte);
        }
        else
        {
            continue;
        }
        
        memset(str_receive_byte, 0, 64);
        memset(str_send_byte, 0, 64);
    }
    
    STAT_LIST_FREE(net_list);

    return guest_stat; 
}