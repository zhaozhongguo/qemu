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
#include "qemu/base64.h"

#pragma comment(lib, "wbemuuid.lib")


#define INIT_WMI_QUERY(query) \
    IWbemServices *pSvc = NULL;  \
    IWbemLocator *pLoc = NULL; \
    IEnumWbemClassObject* pEnumerator = NULL;  \
    HRESULT hres; \
    do \
    { \
        hres =  CoInitializeEx(0, COINIT_MULTITHREADED); \
        if (FAILED(hres)) \
        {  \
            cout << "Failed to initialize COM library. Error code = 0x" \
                << hex << hres << endl; \
            return 1; \
        } \
        \
        hres =  CoInitializeSecurity( \
            NULL, \
            -1, \
            NULL, \
            NULL, \
            RPC_C_AUTHN_LEVEL_DEFAULT, \
            RPC_C_IMP_LEVEL_IMPERSONATE, \
            NULL, \
            EOAC_NONE, \
            NULL \
            ); \
        \
        if (FAILED(hres)) \
        { \
            cout << "Failed to initialize security. Error code = 0x"<< hex << hres << endl; \
            CoUninitialize(); \
            return 1; \
        } \
        \
        hres = CoCreateInstance(CLSID_WbemLocator, 0, \
            CLSCTX_INPROC_SERVER, \
            IID_IWbemLocator, (LPVOID *) &pLoc); \
        \
        if (FAILED(hres)) \
        {  \
            cout << "Failed to create IWbemLocator object."  \
                << " Err code = 0x"  \
                << hex << hres << endl;  \
            CoUninitialize();  \
            return 1; \
        }  \
        \
        hres = pLoc->ConnectServer( \
             _bstr_t(L"ROOT\\CIMV2"), \
             NULL, \
             NULL, \
             0, \
             NULL, \
             0, \
             0, \
             &pSvc \
             ); \
        \
        if (FAILED(hres)) \
        { \
            cout << "Could not connect. Error code = 0x"<< hex << hres << endl; \
            pLoc->Release(); \
            CoUninitialize(); \
            return 1; \
        } \
        \
        hres = CoSetProxyBlanket( \
           pSvc, \
           RPC_C_AUTHN_WINNT, \
           RPC_C_AUTHZ_NONE, \
           NULL, \
           RPC_C_AUTHN_LEVEL_CALL, \
           RPC_C_IMP_LEVEL_IMPERSONATE, \
           NULL, \
           EOAC_NONE \
        ); \
        \
        if (FAILED(hres)) \
        { \
            cout << "Could not set proxy blanket. Error code = 0x" \
                << hex << hres << endl; \
            pSvc->Release(); \
            pLoc->Release(); \
            CoUninitialize(); \
            return 1; \
        } \
        \
        hres = pSvc->ExecQuery( \
            bstr_t("WQL"), \
            bstr_t(query), \
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, \
            NULL, \
            &pEnumerator); \
        \
        if (FAILED(hres)) \
        { \
            cout << "Query for operating system name failed." \
                << " Error code = 0x" \
                << hex << hres << endl; \
            pSvc->Release(); \
            pLoc->Release(); \
            CoUninitialize(); \
            return 1; \
        } \
    } while (0)
    
  
    
#define FINALIZE_WMI_QUERY() \
    do \
    { \
        pSvc->Release(); \
        pLoc->Release(); \
        pEnumerator->Release(); \
        CoUninitialize(); \
    } while (0)
        
#endif