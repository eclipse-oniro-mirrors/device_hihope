/*
 * Copyright (C) 2021 HiSilicon (Shanghai) Technologies CO., LIMITED.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "hdf_wifi_product.h"
#include "wifi_mac80211_ops.h"
#include "hdf_wlan_utils.h"
#include "net_bdh_adpater.h"

#define HDF_LOG_TAG BDH6Driver


int dhd_module_init(void);
void * GetLinuxInfByNetDevice(const struct NetDevice *netDevice);
int get_dhd_priv_data_size(void);

struct NetDevice *g_hdf_netdev = NULL;

void * get_dhd_priv_data(void)
{
    return g_hdf_netdev->mlPriv;
}

// BDH Wifi6 chip driver init
int32_t InitBDH6Chip(struct HdfWlanDevice *device)
{
    (void)device;
    HDF_LOGW("bdh6: call InitBDH6Chip");
#if 0
    //uint8_t maxPortCount = 3;
    int32_t ret = 0;
    uint8_t maxRetryCount = 3;

    //if (device == NULL || device->bus == NULL) {
    if (device == NULL ) {
        HDF_LOGE("%s: device NULL ptr!", __func__);
        return -1;
    }

    do {
        if (ret != 0) {
            if (device->reset != NULL && device->reset->Reset != NULL) {
                // reset wifi device for GPIO
                device->reset->Reset(device->reset);
            }
            HDF_LOGE("%s:Retry init BDH6!last ret=%d", __func__, ret);
        }

        // chip init for driver
        //ret = bdh6_wifi_init(device->bus);
        
    } while (ret != 0 && --maxRetryCount > 0);

    if (ret != 0) {
        HDF_LOGE("%s:Init BDH6 driver failed!", __func__);
        return ret;
    }
#endif
    return HDF_SUCCESS;
}

int32_t DeinitBDH6Chip(struct HdfWlanDevice *device)
{
    int32_t ret = HDF_SUCCESS;
    (void)device;
    //ret = bdh6_wifi_deinit();
    if (ret != 0) {
        HDF_LOGE("%s:Deinit failed!ret=%d", __func__, ret);
    }
    return ret;
}

int32_t BDH6Init(struct HdfChipDriver *chipDriver, struct NetDevice *netDevice)
{
    //uint16_t mode;
    int32_t ret = 0;
    struct HdfWifiNetDeviceData *data = NULL;
    void *netdev = NULL;
    int private_data_size = 0;
    
    //nl80211_iftype_uint8 type;
    (void)chipDriver;
    //HDF_LOGW("%s: start...", __func__);
    HDF_LOGW("bdh6: call BDH6Init");

    if (netDevice == NULL) {
        HDF_LOGE("%s netdevice is null!", __func__);
        return HDF_FAILURE;
    }

    // 
    netdev = GetLinuxInfByNetDevice(netDevice);
    if (NULL == netdev) {
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
    if (NULL == netDevice->mlPriv) {
        HDF_LOGE("%s:kzalloc mlPriv failed", __func__);
        return HDF_FAILURE;
    }
    g_hdf_netdev = netDevice;
    
    dhd_module_init();

    // Call linux register_netdev()
    //ret = NetDeviceAdd(netDevice);
    //HDF_LOGE("%s:NetDeviceAdd ret=%d", __func__, ret);

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
