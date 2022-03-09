/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "analog_headset_base.h"
#include "analog_headset_ev.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "securec.h"
#include "event_hub.h"

#define EV_TYPE_KEY_INDEX       0
#define KEY_CODE_JACK_INDEX     3

InputDevice *g_hdfInDev = NULL;
void InputSetCapability(InputDevice *hdfInDev)
{
    if (hdfInDev == NULL) {
        HDF_LOGE("%s: hdfInDev is NULL.", __func__);
        return;
    }

    hdfInDev->abilitySet.eventType[EV_TYPE_KEY_INDEX] = SET_BIT(EV_KEY);
    hdfInDev->abilitySet.keyCode[KEY_CODE_JACK_INDEX] = SET_BIT(KEY_JACK_HOOK);
}

void SetStateSync(unsigned int id, bool state)
{
    InputDevice *hdfInDev = g_hdfInDev;
    if (hdfInDev == NULL) {
        HDF_LOGE("%s: hdfInDev is NULL.", __func__);
        return;
    }

    ReportKey(hdfInDev, id, state);
    ReportSync(hdfInDev);
}

static InputDevice *HdfInputDeviceInstance(void *hs, struct HdfDeviceObject *device)
{
    int32_t ret;
    InputDevIdentify inputDevId;
    InputDevice *hdfInDev = NULL;
    static char *tempStr = "analog_headset_input_device";

    HDF_LOGI("%s: enter.", __func__);
    if ((hs == NULL) || (device == NULL)) {
        HDF_LOGE("%s: hs or device is NULL.", __func__);
        return NULL;
    }

    hdfInDev = (InputDevice *)OsalMemCalloc(sizeof(InputDevice));
    if (hdfInDev == NULL) {
        HDF_LOGE("%s: instance input device failed", __func__);
        return NULL;
    }

    hdfInDev->hdfDevObj = device;
    hdfInDev->pvtData = hs;
    hdfInDev->devType = INDEV_TYPE_KEY;
    hdfInDev->devName = tempStr;

    inputDevId.vendor = INPUT_DEVID_VENDOR;
    inputDevId.product = INPUT_DEVID_PRODUCT;
    inputDevId.version = INPUT_DEVID_VERSION;
    hdfInDev->attrSet.id = inputDevId;

    ret = strncpy_s(hdfInDev->attrSet.devName, DEV_NAME_LEN, tempStr, DEV_NAME_LEN);
    if (ret != 0) {
        OsalMemFree(hdfInDev);
        hdfInDev = NULL;
        HDF_LOGE("%s: strncpy devName failed", __func__);
        return NULL;
    }

    return hdfInDev;
}

int32_t CreateAndRegisterHdfInputDevice(void *hs, struct HdfDeviceObject *device)
{
    int32_t ret;
    InputDevice *hdfInDev = NULL;

    HDF_LOGI("%s: enter.", __func__);
    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL.", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    hdfInDev = HdfInputDeviceInstance(hs, device);
    if (hdfInDev == NULL) {
        HDF_LOGE("%s: [HdfInputDeviceInstance] failed.", __func__);
        return HDF_FAILURE;
    }
    ret = RegisterInputDevice(hdfInDev);
    if (ret != HDF_SUCCESS) {
        OsalMemFree(hdfInDev);
        hdfInDev = NULL;
        HDF_LOGE("%s: [RegisterInputDevice] failed.", __func__);
        /* Theoretically, the return fails. In fact, two reporting systems are used.
           The registration of the input device is unsuccessful, and another system is still available. */
        return HDF_SUCCESS;
    }

    InputSetCapability(hdfInDev);
    g_hdfInDev = hdfInDev;
    HDF_LOGI("%s: done.", __func__);

    return HDF_SUCCESS;
}