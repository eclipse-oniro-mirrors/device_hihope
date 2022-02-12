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

#ifndef NET_BDH_ADAPTER_H
#define NET_BDH_ADAPTER_H


//#include "hmac_ext_if.h"
//#include "oam_ext_if.h"
//#include "wal_main.h"
#include "net_device.h"
#include <linux/netdevice.h>


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

void set_krn_netdev(void *netdev);
struct net_device * get_krn_netdev(void);
struct wiphy * get_krn_wiphy(void);

int32_t hdf_bdh6_netdev_init(struct NetDevice *netDev);
int32_t hdf_bdh6_netdev_open(struct NetDevice *netDev);
int32_t hdf_bdh6_netdev_stop(struct NetDevice *netDev);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
