/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_device_desc.h"
#include "hdf_wifi_product.h"
#include "hdf_log.h"
#include "osal_mem.h"
#include "hdf_wlan_chipdriver_manager.h"
#include "securec.h"
#include "wifi_module.h"


#define HDF_LOG_TAG BDH6Driver

int32_t InitBDH6Chip(struct HdfWlanDevice *device);
int32_t DeinitBDH6Chip(struct HdfWlanDevice *device);
int32_t BDH6Deinit(struct HdfChipDriver *chipDriver, struct NetDevice *netDevice);
int32_t BDH6Init(struct HdfChipDriver *chipDriver, struct NetDevice *netDevice);

void BDH6Mac80211Init(struct HdfChipDriver *chipDriver);


// match to "chipInst {"  in hcs
static const char * const BDH6_DRIVER_NAME = "hisi";


static struct HdfChipDriver *BuildBDH6Driver(struct HdfWlanDevice *device, uint8_t ifIndex)
{
    struct HdfChipDriver *specificDriver = NULL;
    if (device == NULL) {
        HDF_LOGE("%s fail : channel is NULL", __func__);
        return NULL;
    }
    (void)device;
    (void)ifIndex;
    specificDriver = (struct HdfChipDriver *)OsalMemCalloc(sizeof(struct HdfChipDriver));
    if (specificDriver == NULL) {
        HDF_LOGE("%s fail: OsalMemCalloc fail!", __func__);
        return NULL;
    }
    if (memset_s(specificDriver, sizeof(struct HdfChipDriver), 0, sizeof(struct HdfChipDriver)) != EOK) {
        HDF_LOGE("%s fail: memset_s fail!", __func__);
        OsalMemFree(specificDriver);
        return NULL;
    }

    if (strcpy_s(specificDriver->name, MAX_WIFI_COMPONENT_NAME_LEN, BDH6_DRIVER_NAME) != EOK) {
        HDF_LOGE("%s fail : strcpy_s fail", __func__);
        OsalMemFree(specificDriver);
        return NULL;
    }
    specificDriver->init = BDH6Init;
    specificDriver->deinit = BDH6Deinit;

    HDF_LOGW("bdh6: call BuildBDH6Driver");

    BDH6Mac80211Init(specificDriver);

    return specificDriver;
}

static void ReleaseBDH6Driver(struct HdfChipDriver *chipDriver)
{
    if (chipDriver == NULL) {
        return;
    }
    if (strcmp(chipDriver->name, BDH6_DRIVER_NAME) != 0) {
        HDF_LOGE("%s:Not my driver!", __func__);
        return;
    }
    OsalMemFree(chipDriver);
}

static uint8_t GetBDH6GetMaxIFCount(struct HdfChipDriverFactory *factory)
{
    (void)factory;
    return 1;
}

/* bdh wifi6's chip driver register */
static int32_t HDFWlanRegBDH6DriverFactory(void)
{
    static struct HdfChipDriverFactory BDH6Factory = { 0 };  // WiFi device chip driver
    struct HdfChipDriverManager *driverMgr = NULL;
    driverMgr = HdfWlanGetChipDriverMgr();
    if (driverMgr == NULL) {
        HDF_LOGE("%s fail: driverMgr is NULL!", __func__);
        return HDF_FAILURE;
    }
    BDH6Factory.driverName = BDH6_DRIVER_NAME;
    BDH6Factory.GetMaxIFCount = GetBDH6GetMaxIFCount;
    BDH6Factory.InitChip = InitBDH6Chip;
    BDH6Factory.DeinitChip = DeinitBDH6Chip;
    BDH6Factory.Build = BuildBDH6Driver;
    BDH6Factory.Release = ReleaseBDH6Driver;
    BDH6Factory.ReleaseFactory = NULL;
    if (driverMgr->RegChipDriver(&BDH6Factory) != HDF_SUCCESS) {
        HDF_LOGE("%s fail: driverMgr is NULL!", __func__);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static int32_t HdfWlanBDH6ChipDriverInit(struct HdfDeviceObject *device)
{
    (void)device;
    HDF_LOGW("bdh6: call HdfWlanBDH6ChipDriverInit");
    return HDFWlanRegBDH6DriverFactory();
}

static int HdfWlanBDH6DriverBind(struct HdfDeviceObject *dev)
{
    (void)dev;
    HDF_LOGW("bdh6: call HdfWlanBDH6DriverBind");
    return HDF_SUCCESS;
}

static void HdfWlanBDH6ChipRelease(struct HdfDeviceObject *object)
{
    (void)object;
    HDF_LOGW("bdh6: call HdfWlanBDH6ChipRelease");
}

struct HdfDriverEntry g_hdfBdh6ChipEntry = {
    .moduleVersion = 1,
    .Bind = HdfWlanBDH6DriverBind,
    .Init = HdfWlanBDH6ChipDriverInit,
    .Release = HdfWlanBDH6ChipRelease,
    .moduleName = "HDF_WLAN_CHIPS"
};

HDF_INIT(g_hdfBdh6ChipEntry);
