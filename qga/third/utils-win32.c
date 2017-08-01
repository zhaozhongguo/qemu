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
#include <comdef.h>  
#include <Wbemidl.h>  
#include "utils-win32.h"


int query_system_name()
{  

    INIT_WMI_QUERY("SELECT * FROM Win32_OperatingSystem");
    
    // Get the data from the query
    IWbemClassObject *pclsObj = NULL;  
    ULONG uReturn = 0;
    while (pEnumerator)  
    {  
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);  
        if(0 == uReturn)  
        {  
            break;  
        }  
  
        VARIANT vtProp;  
        // Get the value of the Name property  
        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);  
        wcout << " OS Name : " << vtProp.bstrVal << endl;  
        VariantClear(&vtProp);  
  
        pclsObj->Release();  
    }  
  
    FINALIZE_WMI_QUERY();

    return 0;
}  