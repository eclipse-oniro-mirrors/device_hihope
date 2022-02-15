/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef NET_BDH_ADAPTER_H
#define NET_BDH_ADAPTER_H

#include <linux/netdevice.h>
#include "net_device.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


void set_krn_netdev(struct net_device *netdev);
struct net_device* get_krn_netdev(void);
struct wiphy* get_krn_wiphy(void);

int32_t hdf_bdh6_netdev_init(struct NetDevice *netDev);
int32_t hdf_bdh6_netdev_open(struct NetDevice *netDev);
int32_t hdf_bdh6_netdev_stop(struct NetDevice *netDev);

    
struct NetDeviceInterFace* wal_get_net_dev_ops(void);
int dhd_netdev_changemtu_wrapper(struct net_device *netdev, int mtu);
struct net_device* GetLinuxInfByNetDevice(const struct NetDevice *netDevice);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
