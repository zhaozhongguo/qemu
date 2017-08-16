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

#define _WIN32_DCOM
#include <stdio.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <iphlpapi.h>
#include <string.h>
#include <string>
#include <list>

#include "utils-win32.h"

        
//network counter
typedef struct 
{
    HCOUNTER counter;
    char if_name[MAX_NET_NAME_LEN];
    int type;
} net_counter;



int calculate_mem_usage(char *usage)
{
    if (NULL == usage)
    {
        return FALSE;
    }
    
    MEMORYSTATUS ms;
    ::GlobalMemoryStatus(&ms);
    sprintf(usage, "%d", (int)ms.dwMemoryLoad);
    
    return TRUE;
}


int calculate_cpu_usage(int delay, char *usage)
{
    int ret = FALSE;
    if (NULL == usage)
    {
        return ret;
    }
    
    HQUERY query = NULL;
    PDH_STATUS status = PdhOpenQuery(NULL, 0, &query);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    HCOUNTER counter;
    status = PdhAddCounter(query, LPCSTR("\\Processor Information(_Total)\\% Processor Time"), 0, &counter);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }
    
    delay = delay * 1000;
    PdhCollectQueryData(query);
    Sleep(delay);
    PdhCollectQueryData(query);

    PDH_FMT_COUNTERVALUE pdhValue;
    DWORD dwValue;
    status = PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, &dwValue, &pdhValue);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    sprintf(usage, "%.1f", pdhValue.doubleValue);
    ret = TRUE;
    
cleanup:
    if (query)
    {
        PdhCloseQuery(query);
    }
    
    return ret;
}



int calculate_disk_usage(int delay, disk_stat* stat)
{
    int ret = FALSE;
    if (NULL == stat)
    {
        return ret;
    }
    
    HQUERY query = NULL;
    PDH_STATUS status = PdhOpenQuery(NULL, 0, &query);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    HCOUNTER hcWrites;
    HCOUNTER hcReads;
    HCOUNTER hcWriteOctet;
    HCOUNTER hcReadOctet;
    status = PdhAddCounter(query, LPCSTR("\\PhysicalDisk(_Total)\\Disk Writes/sec"), 0, &hcWrites);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    status = PdhAddCounter(query, LPCSTR("\\PhysicalDisk(_Total)\\Disk Reads/sec"), 0, &hcReads);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    status = PdhAddCounter(query, LPCSTR("\\PhysicalDisk(_Total)\\Disk Write Bytes/sec"), 0, &hcWriteOctet);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    status = PdhAddCounter(query, LPCSTR("\\PhysicalDisk(_Total)\\Disk Read Bytes/sec"), 0, &hcReadOctet);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    delay = delay * 1000;
    PdhCollectQueryData(query);
    Sleep(delay);
    PdhCollectQueryData(query);

    PDH_FMT_COUNTERVALUE pdhValueWrites;
    PDH_FMT_COUNTERVALUE pdhValueReads;
    PDH_FMT_COUNTERVALUE pdhValueWriteOctet;
    PDH_FMT_COUNTERVALUE pdhValueReadOctet;
    DWORD dwValue;

    status = PdhGetFormattedCounterValue(hcWrites, PDH_FMT_DOUBLE, &dwValue, &pdhValueWrites);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    status = PdhGetFormattedCounterValue(hcReads, PDH_FMT_DOUBLE, &dwValue, &pdhValueReads);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    status = PdhGetFormattedCounterValue(hcWriteOctet, PDH_FMT_DOUBLE, &dwValue, &pdhValueWriteOctet);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    status = PdhGetFormattedCounterValue(hcReadOctet, PDH_FMT_DOUBLE, &dwValue, &pdhValueReadOctet);
    if (status != ERROR_SUCCESS)
    {
        goto cleanup;
    }

    stat->rd_ops = pdhValueReads.doubleValue;
    stat->wr_ops = pdhValueWrites.doubleValue;
    stat->rd_octet = pdhValueReadOctet.doubleValue;
    stat->wr_octet = pdhValueWriteOctet.doubleValue;
    
    ret = TRUE;
    
cleanup:
    if (query)
    {
        PdhCloseQuery(query);
    }
    
    return ret;
}



static void mod_network_name(char *name)
{
    char *cur = name;
    while ('\0' != *cur)
    {
        if ('(' == *cur)
        {
            *cur = '[';
        }
        else if (')' == *cur)
        {
            *cur = ']';
        }
        
        cur++;
    }
}



struct net_stat_list* calculate_network_usage(int delay)
{
    //query network interface name
    PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
    unsigned long stSize = sizeof(IP_ADAPTER_INFO);
    int nRel = GetAdaptersInfo(pIpAdapterInfo,&stSize);
    if (ERROR_BUFFER_OVERFLOW == nRel)
    {
        delete pIpAdapterInfo;
        pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
        nRel = GetAdaptersInfo(pIpAdapterInfo,&stSize);  
    }

    std::list<std::string> name_list;
    if (ERROR_SUCCESS == nRel)
    {
        while (pIpAdapterInfo)
        {
            mod_network_name(pIpAdapterInfo->Description);
            name_list.push_back(std::string(pIpAdapterInfo->Description));
            pIpAdapterInfo = pIpAdapterInfo->Next;
        }
    }
    else
    {
        if (pIpAdapterInfo)
        {
            delete[] pIpAdapterInfo;
        }
        return NULL;
    }

    if (pIpAdapterInfo)
    {
        delete[] pIpAdapterInfo;
    }

    //open pdh query
    HQUERY query = NULL;
    PDH_STATUS status = PdhOpenQuery(NULL, 0, &query);
    if (status != ERROR_SUCCESS)
    {
        if (query)
        {
            PdhCloseQuery(query);
        }

        return NULL;
    }

    //add pdh counter
    std::string prefix = "\\Network Interface(";
    std::string sent_suffix = ")\\Bytes Sent/sec";
    std::string recv_suffix = ")\\Bytes Received/sec";
    std::list<net_counter*> counter_list;
    std::list<std::string>::iterator iter = name_list.begin();
    for (; iter != name_list.end(); iter++)
    {
        //sent counter path
        std::string sent_path = prefix + *iter + sent_suffix;
        net_counter* pSent_counter = new net_counter();
        memset((char*)pSent_counter, 0, sizeof(net_counter));
        status = PdhAddCounter(query, LPCSTR(sent_path.c_str()), 0, &pSent_counter->counter);
        if (status == ERROR_SUCCESS)
        {
            counter_list.push_back(pSent_counter);
            strncpy(pSent_counter->if_name, (*iter).c_str(), MAX_NET_NAME_LEN - 1);
            pSent_counter->type = NET_COUNTER_SENT;
        }
        else
        {
            delete pSent_counter;
        }

        //recv counter path
        std::string recv_path = prefix + *iter + recv_suffix;
        net_counter* pRecv_counter = new net_counter();
        memset((char*)pRecv_counter, 0, sizeof(net_counter));
        status = PdhAddCounter(query, LPCSTR(recv_path.c_str()), 0, &pRecv_counter->counter);
        if (status == ERROR_SUCCESS)
        {
            counter_list.push_back(pRecv_counter);
            strncpy(pRecv_counter->if_name, (*iter).c_str(), MAX_NET_NAME_LEN - 1);
            pRecv_counter->type = NET_COUNTER_RECV;
        }
        else
        {
            delete pRecv_counter;
        }
    }

    //query data
    delay = delay * 1000;
    PdhCollectQueryData(query);
    Sleep(delay);
    PdhCollectQueryData(query);

    //get query result
    int length = (int)counter_list.size();
    struct net_stat_list* net_stat_list = NULL;
    STAT_LIST_ALLOCATE(net_stat, length, net_stat_list);
    if (NULL == net_stat_list)
    {
        return NULL;
    }
    
    PDH_FMT_COUNTERVALUE pdhValue;
    DWORD dwValue = 0;
    std::list<net_counter*>::iterator it = counter_list.begin();
    for (; it != counter_list.end(); it++)
    {
        status = PdhGetFormattedCounterValue((*it)->counter, PDH_FMT_DOUBLE, &dwValue, &pdhValue);
        if (status == ERROR_SUCCESS)
        {
            if (net_stat_list->length < net_stat_list->capacity)
            {
                net_stat* pStat = &(net_stat_list->list[net_stat_list->length++]);
                strncpy(pStat->if_name, (*it)->if_name, MAX_NET_NAME_LEN - 1);
                pStat->type = (*it)->type;
                pStat->value = pdhValue.doubleValue;
            }
        }

        delete *it;
    }

    //close pdh query
    if (query)
    {
        PdhCloseQuery(query);
    }
    
    return net_stat_list;
}


