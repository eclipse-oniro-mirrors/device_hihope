/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include "net_bdh_adpater.h"
#include <net/cfg80211.h>

#include <securec.h>

#include "eapol.h"

#define HDF_LOG_TAG BDH6Driver

struct net_device *g_linux_netdev = NULL;
struct wiphy *g_linux_wiphy = NULL;

void set_krn_netdev(struct net_device *netdev)
{
    g_linux_netdev = (struct net_device *)netdev;
}

struct net_device* get_krn_netdev(void)
{
    return g_linux_netdev;
}

struct wiphy* get_krn_wiphy(void)
{
    return g_linux_wiphy;
}

void set_krn_wiphy(struct wiphy *pwiphy)
{
    g_linux_wiphy = (struct wiphy *)pwiphy;
}


int32_t hdf_bdh6_netdev_init(struct NetDevice *netDev)
{
    HDF_LOGE("%s: start...", __func__);
    if (netDev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: netDev->name:%s\n", __func__, netDev->name);
    netDev->netDeviceIf = wal_get_net_dev_ops();
    CreateEapolData(netDev);

    return HDF_SUCCESS;
}

void hdf_bdh6_netdev_deinit(struct NetDevice *netDev)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
}

int32_t hdf_bdh6_netdev_open(struct NetDevice *netDev)
{
    int32_t retVal = 0;
    const int idx0 = 0, idx1 = 1, idx2 = 2, idx3 = 3, idx4 = 4, idx5 = 5;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    if (netdev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: ndo_stop...", __func__);
    retVal = (int32_t)dhd_ops_pri.ndo_stop(netdev);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf net device stop failed! ret = %d", __func__, retVal);
    }
 
    retVal = (int32_t)dhd_ops_pri.ndo_open(netdev);
    if (retVal < 0) {
    }

    netDev->ieee80211Ptr = netdev->ieee80211_ptr;
    if (netDev->ieee80211Ptr == NULL) {
    }

    // update mac addr to NetDevice object
    memcpy_s(netDev->macAddr, MAC_ADDR_SIZE, netdev->dev_addr, netdev->addr_len);
    HDF_LOGE("%s: %02x:%02x:%02x:%02x:%02x:%02x", __func__,
        netDev->macAddr[idx0], netDev->macAddr[idx1], netDev->macAddr[idx2],
        netDev->macAddr[idx3], netDev->macAddr[idx4], netDev->macAddr[idx5]);

    return retVal;
}

int32_t hdf_bdh6_netdev_stop(struct NetDevice *netDev)
{
    int32_t retVal = 0;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    HDF_LOGE("%s: start...", __func__);

    if (netdev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return HDF_FAILURE;
    }

    retVal = (int32_t)dhd_ops_pri.ndo_stop(netdev);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf net device stop failed! ret = %d", __func__, retVal);
    }

    return retVal;
}

int32_t hdf_bdh6_netdev_xmit(struct NetDevice *netDev, NetBuf *netBuff)
{
    int32_t retVal = 0;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    if (netdev == NULL || netBuff == NULL) {
        HDF_LOGE("%s: netdev or netBuff null!", __func__);
        return HDF_FAILURE;
    }

    retVal = (int32_t)dhd_ops_pri.ndo_start_xmit((struct sk_buff *)netBuff, netdev);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf net device xmit failed! ret = %d", __func__, retVal);
    }

    return retVal;
}

int32_t hdf_bdh6_netdev_ioctl(struct NetDevice *netDev, IfReq *req, int32_t cmd)
{
    int32_t retVal = 0;
    struct ifreq dhd_req = {0};
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    HDF_LOGE("%s: start...", __func__);

    if (netdev == NULL || req == NULL) {
        HDF_LOGE("%s: netdev or req null!", __func__);
        return HDF_FAILURE;
    }

    dhd_req.ifr_ifru.ifru_data = req->ifrData;

    retVal = (int32_t)dhd_ops_pri.ndo_do_ioctl(netdev, &dhd_req, cmd);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf net device ioctl failed! ret = %d", __func__, retVal);
    }

    return retVal;
}

int32_t hdf_bdh6_netdev_setmacaddr(struct NetDevice *netDev, unsigned char *addr)
{
    int32_t retVal = 0;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    HDF_LOGE("%s: start...", __func__);

    if (netdev == NULL || addr == NULL) {
        HDF_LOGE("%s: netDev or addr null!", __func__);
        return HDF_FAILURE;
    }

    retVal = (int32_t)dhd_ops_pri.ndo_set_mac_address(netdev, addr);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf net device setmacaddr failed! ret = %d", __func__, retVal);
    }

    return retVal;
}

struct NetDevStats *hdf_bdh6_netdev_getstats(struct NetDevice *netDev)
{
    static struct NetDevStats devStat = {0};
    struct net_device_stats *kdevStat = NULL;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);

    HDF_LOGE("%s: start...", __func__);

    if (netdev == NULL) {
        HDF_LOGE("%s: netDev null!", __func__);
        return NULL;
    }

    kdevStat = dhd_ops_pri.ndo_get_stats(netdev);
    if (kdevStat == NULL) {
        HDF_LOGE("%s: ndo_get_stats return null!", __func__);
        return NULL;
    }

    devStat.rxPackets = kdevStat->rx_packets;
    devStat.txPackets = kdevStat->tx_packets;
    devStat.rxBytes = kdevStat->rx_bytes;
    devStat.txBytes = kdevStat->tx_bytes;
    devStat.rxErrors = kdevStat->rx_errors;
    devStat.txErrors = kdevStat->tx_errors;
    devStat.rxDropped = kdevStat->rx_dropped;
    devStat.txDropped = kdevStat->tx_dropped;

    return &devStat;
}

void hdf_bdh6_netdev_setnetifstats(struct NetDevice *netDev, NetIfStatus status)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
    (void)status;
}

uint16_t hdf_bdh6_netdev_selectqueue(struct NetDevice *netDev, NetBuf *netBuff)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
    (void)netBuff;
    return HDF_SUCCESS;
}

uint32_t hdf_bdh6_netdev_netifnotify(struct NetDevice *netDev, NetDevNotify *notify)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
    (void)notify;
    return HDF_SUCCESS;
}

int32_t hdf_bdh6_netdev_changemtu(struct NetDevice *netDev, int32_t mtu)
{
    int32_t retVal = 0;
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);
    HDF_LOGE("%s: start...", __func__);

    if (netdev == NULL) {
        HDF_LOGE("%s: netdev null!", __func__);
        return HDF_FAILURE;
    }
    HDF_LOGE("%s: change mtu to %d\n", __FUNCTION__, mtu);
    // dengb fixed
    retVal = (int32_t)dhd_netdev_changemtu_wrapper(netdev, mtu);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf net device chg mtu failed! ret = %d", __func__, retVal);
    }

    return retVal;
}

void hdf_bdh6_netdev_linkstatuschanged(struct NetDevice *netDev)
{
    HDF_LOGE("%s: start...", __func__);
    (void)netDev;
}

#define BDH6_SHIFT_BIT 8

ProcessingResult hdf_bdh6_netdev_specialethertypeprocess(const struct NetDevice *netDev, NetBuf *buff)
{
    struct EtherHeader *header = NULL;
    const struct Eapol *eapolInstance = NULL;
    int ret = HDF_SUCCESS;
    uint16_t protocol;
    const int pidx0 = 12, pidx1 = 13;

    HDF_LOGE("%s: start...", __func__);

    if (netDev == NULL || buff == NULL) {
        return PROCESSING_ERROR;
    }

    header = (struct EtherHeader *)NetBufGetAddress(buff, E_DATA_BUF);

    protocol = (buff->data[pidx0] << BDH6_SHIFT_BIT) | buff->data[pidx1];
    if (protocol != ETHER_TYPE_PAE) {
        HDF_LOGE("%s: return PROCESSING_CONTINUE", __func__);
        NetBufFree(buff);
        return PROCESSING_CONTINUE;
    }
    if (netDev->specialProcPriv == NULL) {
        HDF_LOGE("%s: return PROCESSING_ERROR", __func__);
        NetBufFree(buff);
        return PROCESSING_ERROR;
    }

    eapolInstance = EapolGetInstance();
    ret = eapolInstance->eapolOp->writeEapolToQueue(netDev, buff);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: writeEapolToQueue failed", __func__);
        NetBufFree(buff);
    }
    return PROCESSING_COMPLETE;
}


struct NetDeviceInterFace g_wal_bdh6_net_dev_ops = {
    .init       = hdf_bdh6_netdev_init,
    .deInit     = hdf_bdh6_netdev_deinit,
    .open       = hdf_bdh6_netdev_open,
    .stop       = hdf_bdh6_netdev_stop,
    .xmit       = hdf_bdh6_netdev_xmit,
    .ioctl      = hdf_bdh6_netdev_ioctl,
    .setMacAddr = hdf_bdh6_netdev_setmacaddr,
    .getStats   = hdf_bdh6_netdev_getstats,
    .setNetIfStatus     = hdf_bdh6_netdev_setnetifstats,
    .selectQueue        = hdf_bdh6_netdev_selectqueue,
    .netifNotify        = hdf_bdh6_netdev_netifnotify,
    .changeMtu          = hdf_bdh6_netdev_changemtu,
    .linkStatusChanged  = hdf_bdh6_netdev_linkstatuschanged,
    .specialEtherTypeProcess  = hdf_bdh6_netdev_specialethertypeprocess,
};

struct NetDeviceInterFace *wal_get_net_dev_ops(void)
{
    return &g_wal_bdh6_net_dev_ops;
}

void wal_netif_rx_ni(struct sk_buff *skb)
{
    NetIfRxNi(g_hdf_netdev, skb);
}

void wal_netif_rx(struct sk_buff *skb)
{
    NetIfRx(g_hdf_netdev, skb);
}

NetDevice* get_bdh6_netDev(void)
{
    return g_hdf_netdev;
}

