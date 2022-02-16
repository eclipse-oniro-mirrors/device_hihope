/*
 * hdf_public_ap6275s.h
 *
 * ap6275s driver header
 *
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef _HDF_PUBLIC_AP6275S_H_
#define _HDF_PUBLIC_AP6275S_H_
#include "net_device.h"

void wl_cfg80211_abort_scan(struct wiphy *wiphy, struct wireless_dev *wdev);
struct net_device* GetLinuxInfByNetDevice(const struct NetDevice *netDevice);
struct NetDevice* GetHdfNetDeviceByLinuxInf(struct net_device *dev);
struct wiphy* get_linux_wiphy_ndev(struct net_device *ndev);

void dhd_get_mac_address(struct net_device *dev, unsigned char **addr);
int32_t wal_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy, struct net_device *netDev);
int32_t wl_cfg80211_set_wps_p2p_ie_wrp(struct net_device *ndev, char *buf, int len, int8_t type);
int32_t wal_cfg80211_remain_on_channel(struct wiphy *wiphy, struct net_device *netDev, int32_t freq,
    unsigned int duration);
void wl_cfg80211_add_virtual_iface_wrap(struct wiphy *wiphy, char *name, enum nl80211_iftype type,
    struct vif_params *params);
int32_t wl_cfg80211_set_country_code(struct net_device *net, char *country_code,
    bool notify, bool user_enforced, int revinfo);
int32_t HdfWifiEventInformBssFrame(const struct NetDevice *netDev,
    const struct WlanChannel *channel, const struct ScannedBssInfo *bssInfo);

s32 wldev_ioctl_get(struct net_device *dev, u32 cmd, unsigned char *arg, u32 len);
#endif