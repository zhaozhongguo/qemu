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
#include "qemu/base64.h"

