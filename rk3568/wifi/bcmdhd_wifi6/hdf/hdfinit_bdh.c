/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "hdf_wifi_product.h"
#include "wifi_mac80211_ops.h"
#include "hdf_wlan_utils.h"
#include "net_bdh_adpater.h"

#define HDF_LOG_TAG BDH6Driver


int dhd_module_init(void);
int get_dhd_priv_data_size(void);

struct NetDevice *g_hdf_netdev = NULL;

void* get_dhd_priv_data(void)
{
    return g_hdf_netdev->mlPriv;
}

// BDH Wifi6 chip driver init
int32_t InitBDH6Chip(struct HdfWlanDevice *device)
{
    (void)device;
    HDF_LOGW("bdh6: call InitBDH6Chip");
    return HDF_SUCCESS;
}

int32_t DeinitBDH6Chip(struct HdfWlanDevice *device)
{
    int32_t ret = HDF_SUCCESS;
    (void)device;
    if (ret != 0) {
        HDF_LOGE("%s:Deinit failed!ret=%d", __func__, ret);
    }
    return ret;
}

int32_t BDH6Init(struct HdfChipDriver *chipDriver, struct NetDevice *netDevice)
{
    int32_t ret = 0;
    struct HdfWifiNetDeviceData *data = NULL;
    void *netdev = NULL;
    int private_data_size = 0;
    
    (void)chipDriver;
    HDF_LOGW("bdh6: call BDH6Init");

    if (netDevice == NULL) {
        HDF_LOGE("%s netdevice is null!", __func__);
        return HDF_FAILURE;
    }

    netdev = GetLinuxInfByNetDevice(netDevice);
    if (netdev == NULL) {
        HDF_LOGE("%s net_device is null!", __func__);
        return HDF_FAILURE;
    }

    set_krn_netdev(netdev);

    data = GetPlatformData(netDevice);
    if (data == NULL) {
        HDF_LOGE("%s:netdevice data null!", __func__);
        return HDF_FAILURE;
    }

    /* set netdevice ops to netDevice */
    hdf_bdh6_netdev_init(netDevice);
    netDevice->classDriverPriv = data;

    // create bdh6 private object
    private_data_size = get_dhd_priv_data_size();
    netDevice->mlPriv = kzalloc(private_data_size, GFP_KERNEL);
    if (netDevice->mlPriv == NULL) {
        HDF_LOGE("%s:kzalloc mlPriv failed", __func__);
        return HDF_FAILURE;
    }
    g_hdf_netdev = netDevice;
    
    dhd_module_init();

    ret = hdf_bdh6_netdev_open(netDevice);
    return HDF_SUCCESS;
}

int32_t BDH6Deinit(struct HdfChipDriver *chipDriver, struct NetDevice *netDevice)
{
    (void)chipDriver;
    (void)netDevice;
    hdf_bdh6_netdev_stop(netDevice);
    return HDF_SUCCESS;
}
