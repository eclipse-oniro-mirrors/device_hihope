
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/in6.h>
#include <net/netlink.h>
#include <linux/wireless.h>
#include <net/cfg80211.h>

#include "osal_mem.h"

#include "hdf_mac80211_sta_event.h"
#include "net_device.h"
#include "net_device_impl.h"
#include "net_device_adapter.h"
#include "wifi_mac80211_ops.h"
#include "hdf_wifi_cmd.h"
#include "hdf_wifi_event.h"
#include <securec.h>

#define HDF_LOG_TAG BDH6Driver

typedef unsigned char           hi_u8;

#define WIFI_SCAN_EXTRA_IE_LEN_MAX      (512)

extern struct NetDevice * GetHdfNetDeviceByLinuxInf(struct net_device *dev);

int32_t HdfScanEventCallback(struct net_device *ndev, HdfWifiScanStatus _status)
{
    int32_t ret = 0;

    NetDevice *netDev = GetHdfNetDeviceByLinuxInf(ndev);
    WifiScanStatus status = _status;

    HDF_LOGE("%s: %d, scandone!", __func__, _status);

    ret = HdfWifiEventScanDone(netDev, status);

    return ret;
}

#define HDF_ETHER_ADDR_LEN (6)
int32_t HdfConnectResultEventCallback(struct net_device *ndev, uint8_t *bssid, uint8_t *reqIe,
                           uint8_t *rspIe, uint32_t reqIeLen,
                           uint32_t rspIeLen, uint16_t connectStatus, uint16_t freq)
{
    int32_t retVal = 0;
    NetDevice *netDev = GetHdfNetDeviceByLinuxInf(ndev);
    struct ConnetResult connResult;

    HDF_LOGE("%s: enter", __func__);

    if (netDev == NULL || bssid == NULL || rspIe == NULL || reqIe == NULL) {
        HDF_LOGE("%s: netDev / bssid / rspIe / reqIe  null!", __func__);
        return -1;
    }

    memcpy(&connResult.bssid[0], bssid, HDF_ETHER_ADDR_LEN);
    HDF_LOGE("%s: connResult:%02x:%02x:%02x:%02x:%02x:%02x, freq:%d\n", __func__, 
        connResult.bssid[0], connResult.bssid[1], connResult.bssid[2],
        connResult.bssid[3], connResult.bssid[4], connResult.bssid[5], freq);

    connResult.rspIe = rspIe;
    connResult.rspIeLen = rspIeLen;
    
    connResult.reqIe = reqIe;
    connResult.reqIeLen = reqIeLen;

    connResult.connectStatus = connectStatus;

    connResult.freq = freq;
    connResult.statusCode = connectStatus;

    retVal = HdfWifiEventConnectResult(netDev, &connResult);
    if (retVal < 0) {
        HDF_LOGE("%s: hdf wifi event inform connect result failed!", __func__);
    }

    return retVal;
}

int32_t HdfDisconnectedEventCallback(struct net_device *ndev, uint16_t reason, uint8_t *ie, uint32_t len)
{
    int32_t ret = 0;

    NetDevice *netDev = GetHdfNetDeviceByLinuxInf(ndev);

    HDF_LOGE("%s: leave", __func__);

    ret = HdfWifiEventDisconnected(netDev, reason, ie, len);

    return ret;
}

void HdfInformBssFrameEventCallback(struct net_device *ndev, struct ieee80211_channel *channel, int32_t signal, 
                      int16_t freq, struct ieee80211_mgmt *mgmt, uint32_t mgmtLen)
{
    int32_t retVal = 0;
    NetDevice *netDev = GetHdfNetDeviceByLinuxInf(ndev);
    struct ScannedBssInfo bssInfo;
    struct WlanChannel hdfchannel;

    if (channel == NULL || netDev == NULL || mgmt == NULL) {
        HDF_LOGE("%s: inform_bss_frame channel = null or netDev = null!", __func__);
        return;
    }

    bssInfo.signal = signal;
    bssInfo.freq = freq;
    bssInfo.mgmtLen = mgmtLen;
    bssInfo.mgmt = (struct Ieee80211Mgmt *)mgmt;

    hdfchannel.flags = channel->flags;
    hdfchannel.channelId = channel->hw_value;
    hdfchannel.centerFreq = channel->center_freq;

/*    HDF_LOGE("%s: hdfchannel flags:%d--channelId:%d--centerFreq:%d--dstMac:%02x:%02x:%02x:%02x:%02x:%02x,!",
        __func__, hdfchannel.flags, hdfchannel.channelId, hdfchannel.centerFreq, 
        bssInfo.mgmt->bssid[0], bssInfo.mgmt->bssid[1], bssInfo.mgmt->bssid[2], 
        bssInfo.mgmt->bssid[3], bssInfo.mgmt->bssid[4], bssInfo.mgmt->bssid[5]);
*/
    retVal = HdfWifiEventInformBssFrame(netDev, &hdfchannel, &bssInfo);
    
    if (retVal < 0) {
        HDF_LOGE("%s: hdf wifi event inform bss frame failed!", __func__);
    }
}




