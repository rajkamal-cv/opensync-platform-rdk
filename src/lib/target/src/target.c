/*
Copyright (c) 2017, Plume Design Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. Neither the name of the Plume Design Inc. nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Plume Design Inc. BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>

#include "os.h"
#include "os_nif.h"
#include "log.h"
#include "const.h"

#include "target.h"
#include "target_internal.h"
#include "osp_unit.h"

#include "osync_hal.h"

#define MODULE_ID LOG_MODULE_ID_OSA

struct ev_loop *wifihal_evloop = NULL;

/******************************************************************************
 *  TARGET definitions
 *****************************************************************************/

bool target_ready(struct ev_loop *loop)
{
    char        tmp[64];

    if (osync_hal_ready() != OSYNC_HAL_SUCCESS)
    {
        LOGW("Target not ready, OSync HAL not ready");
        return false;
    }

    // Check if we can read important entity information

    if (!osp_unit_serial_get(ARRAY_AND_SIZE(tmp)))
    {
        LOGW("Target not ready, failed to query serial number");
        return false;
    }

    if (!osp_unit_id_get(ARRAY_AND_SIZE(tmp)))
    {
        LOGW("Target not ready, failed to query id (CM MAC)");
        return false;
    }

    if (!osp_unit_model_get(ARRAY_AND_SIZE(tmp)))
    {
        LOGW("Target not ready, failed to query model number");
        return false;
    }

    if (!osp_unit_platform_version_get(ARRAY_AND_SIZE(tmp)))
    {
        LOGW("Target not ready, failed to query platform version");
        return false;
    }

    wifihal_evloop = loop;

    return true;
}

bool target_init(target_init_opt_t opt, struct ev_loop *loop)
{
    INT ret;
    CHAR str[64];  // Unfortunately, the RDK Wifi HAL doesn't specify
                   // the maximum length of version string. It is usually
                   // something like "2.0.0", so assume 64 is enough.

    if (osync_hal_init() != OSYNC_HAL_SUCCESS)
    {
        LOGE("OSync HAL init failed");
        return false;
    }

    wifihal_evloop = loop;

    ret = wifi_getHalVersion(str);
    if (ret != RETURN_OK)
    {
        LOGE("Manager %d: cannot get HAL version", opt);
        return false;
    }

    switch (opt)
    {
        case TARGET_INIT_MGR_SM:
            break;

        case TARGET_INIT_MGR_WM:
#ifndef CONFIG_RDK_DISABLE_SYNC
            sync_init(SYNC_MGR_WM, NULL);
#endif
            break;

        case TARGET_INIT_MGR_CM:
#ifndef CONFIG_RDK_DISABLE_SYNC
            sync_init(SYNC_MGR_CM, cloud_config_mode_init);
#endif
            break;

        case TARGET_INIT_MGR_BM:
            break;

        default:
            break;
    }

    return true;
}

bool target_close(target_init_opt_t opt, struct ev_loop *loop)
{
    if (osync_hal_deinit() != OSYNC_HAL_SUCCESS)
    {
        LOGW("OSync HAL deinit failed.");
    }

    switch (opt)
    {
        case TARGET_INIT_MGR_WM:
#ifndef CONFIG_RDK_DISABLE_SYNC
            sync_cleanup();
            /* fall through */
#endif

        case TARGET_INIT_MGR_SM:
            break;

        default:
            break;
    }

    target_map_close();

    return true;
}
