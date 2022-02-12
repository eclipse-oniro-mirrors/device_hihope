#include <linux/version.h>

#include "wifi_module.h"
#include "wifi_mac80211_ops.h"
#include "hdf_wlan_utils.h"
#include <net/cfg80211.h>
#include <net/regulatory.h>

#include "net_bdh_adpater.h"
#include <securec.h>

#define HDF_LOG_TAG BDH6Driver

extern struct cfg80211_ops wl_cfg80211_ops;
extern struct net_device_ops dhd_ops_pri;


typedef enum {
    WLAN_BAND_2G,
    WLAN_BAND_5G,
    WLAN_BAND_BUTT
} wlan_channel_band_enum;

#define WIFI_24G_CHANNEL_NUMS   (14)
#define WAL_MIN_CHANNEL_2G      (1)
#define WAL_MAX_CHANNEL_2G      (14)
#define WAL_MIN_FREQ_2G         (2412 + 5*(WAL_MIN_CHANNEL_2G - 1))
#define WAL_MAX_FREQ_2G         (2484)
#define WAL_FREQ_2G_INTERVAL    (5)

//#define WAL_MIN_FREQ_5G         (5180 + 20*(WAL_MIN_CHANNEL_2G - 1))
//#define WAL_MAX_FREQ_5G         (5850)
//#define WAL_FREQ_2G_INTERVAL    (5)


#define WLAN_WPS_IE_MAX_SIZE    (352) // (WLAN_MEM_EVENT_SIZE2 - 32)   /* 32表示事件自身占用的空间 */
/* Driver supports AP mode */
#define HISI_DRIVER_FLAGS_AP                         0x00000040
/* Driver supports concurrent P2P operations */
#define HISI_DRIVER_FLAGS_P2P_CONCURRENT             0x00000200

#define HISI_DRIVER_FLAGS_P2P_DEDICATED_INTERFACE    0x00000400
/* P2P capable (P2P GO or P2P Client) */
#define HISI_DRIVER_FLAGS_P2P_CAPABLE                0x00000800
/* Driver supports a dedicated interface for P2P Device */
#define HISI_DRIVER_FLAGS_DEDICATED_P2P_DEVICE       0x20000000

struct net_device * GetLinuxInfByNetDevice(const struct NetDevice *netDevice);


struct wiphy * get_linux_wiphy_ndev(struct net_device *ndev)
{
    if (NULL == ndev || NULL == ndev->ieee80211_ptr) {
        //HDF_LOGE("%s: net_device is NULL", __func__);
        return NULL;
    }

    return ndev->ieee80211_ptr->wiphy;
}

struct wiphy * get_linux_wiphy_hdfdev(NetDevice *netDev)
{
    struct net_device *ndev = GetLinuxInfByNetDevice(netDev);
    return get_linux_wiphy_ndev(ndev);
}


int32_t BDH6WalSetMode(NetDevice *netDev, enum WlanWorkMode iftype)
{
    int32_t retVal = 0;
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;
    //struct wiphy* wiphy = get_krn_wiphy();
    //struct net_device *netdev = get_krn_netdev();
    
    netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return -1;
    }
    
    wiphy = get_linux_wiphy_ndev(netdev);
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return -1;
    }

    HDF_LOGE("%s: start... iftype=%d ", __func__, iftype);
    retVal = (int32_t)wl_cfg80211_ops.change_virtual_intf(wiphy, netdev, 
                                                         (enum nl80211_iftype)iftype, 
                                                         NULL);
    if (retVal < 0) {
        HDF_LOGE("%s: set mode failed!", __func__);
    }

    return retVal;
}

int32_t BDH6WalAddKey(struct NetDevice *netDev, uint8_t keyIndex, bool pairwise, const uint8_t *macAddr,
    struct KeyParams *params)
{
    int32_t retVal = 0;
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;
    //struct wiphy* wiphy = get_krn_wiphy();
    //struct net_device *netdev = get_krn_netdev();

#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
    struct key_params keypm;
#endif

    netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return -1;
    }
    
    wiphy = get_linux_wiphy_ndev(netdev);
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return -1;
    }

    //HDF_LOGE("%s: start..., mac=%02x:%02x:%02x:%02x:%02x:%02x, keyIndex=%u,pairwise=%d, cipher=%u, seqlen=%d,keylen=%d", 
    //    __func__, macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5], keyIndex, pairwise, params->cipher, 
    //    params->seqLen, params->keyLen);
    
    HDF_LOGE("%s: start..., mac=%p, keyIndex=%u,pairwise=%d, cipher=0x%x, seqlen=%d,keylen=%d", 
            __func__, macAddr, keyIndex, pairwise, params->cipher, 
            params->seqLen, params->keyLen);
    print_hex_dump(KERN_INFO, "key: ", DUMP_PREFIX_NONE, 16, 1, params->key, params->keyLen, true);

    (void)netDev;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
    memset_s(&keypm, sizeof(struct key_params), 0, sizeof(struct key_params));
    keypm.key = params->key;
    keypm.seq = params->seq;
    keypm.key_len = params->keyLen;
    keypm.seq_len = params->seqLen;
    keypm.cipher = params->cipher;
    keypm.vlan_id = 0;
    retVal = (int32_t)wl_cfg80211_ops.add_key(wiphy, netdev, keyIndex, pairwise, macAddr, &keypm);
#else
    retVal = (int32_t)wl_cfg80211_ops.add_key(wiphy, netdev, keyIndex, pairwise, macAddr, (struct key_params *)params);
#endif
    if (retVal < 0) {
        HDF_LOGE("%s: add key failed!", __func__);
    }

    return retVal;
}

int32_t BDH6WalDelKey(struct NetDevice *netDev, uint8_t keyIndex, bool pairwise, const uint8_t *macAddr)
{
    int32_t retVal = 0;
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;
    //struct wiphy* wiphy = get_krn_wiphy();
    //struct net_device *netdev = get_krn_netdev();

    netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return -1;
    }
    
    wiphy = get_linux_wiphy_ndev(netdev);
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return -1;
    }

    HDF_LOGE("%s: start..., mac=%p, keyIndex=%u,pairwise=%d", __func__, macAddr, keyIndex, pairwise);

    (void)netDev;
    retVal = (int32_t)wl_cfg80211_ops.del_key(wiphy, netdev, keyIndex, pairwise, macAddr);
    if (retVal < 0) {
        HDF_LOGE("%s: delete key failed!", __func__);
    }

    return retVal;
}

int32_t BDH6WalSetDefaultKey(struct NetDevice *netDev, uint8_t keyIndex, bool unicast, bool multicas)
{
    int32_t retVal = 0;
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;
    //struct wiphy* wiphy = get_krn_wiphy();
    //struct net_device *netdev = get_krn_netdev();

    netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return -1;
    }
    
    wiphy = get_linux_wiphy_ndev(netdev);
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return -1;
    }

    HDF_LOGE("%s: start..., keyIndex=%u,unicast=%d, multicas=%d", __func__, keyIndex, unicast, multicas);
    
    retVal = (int32_t)wl_cfg80211_ops.set_default_key(wiphy, netdev, keyIndex, unicast, multicas);
    if (retVal < 0) {
        HDF_LOGE("%s: set default key failed!", __func__);
    }

    return retVal;
}

int32_t BDH6WalGetDeviceMacAddr(NetDevice *netDev, int32_t type, uint8_t *mac, uint8_t len)
{
    //struct net_device *netdev = get_krn_netdev();
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return -1;
    }

    (void)len;
    (void)type;
    (void)netDev;
    HDF_LOGE("%s: start...", __func__);
    
    memcpy_s(mac, len, netdev->dev_addr, netdev->addr_len);

    return HDF_SUCCESS;
}

int32_t BDH6WalSetMacAddr(NetDevice *netDev, uint8_t *mac, uint8_t len)
{
    int32_t retVal = 0;
    //struct net_device *netdev = get_krn_netdev();
    struct net_device *netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return -1;
    }

    (void)len;
    (void)netDev;
    HDF_LOGE("%s: start...", __func__);

    retVal = (int32_t)dhd_ops_pri.ndo_set_mac_address(netdev, mac);
    if (retVal < 0) {
        HDF_LOGE("%s: set mac address failed!", __func__);
    }

    return retVal;
}

int32_t BDH6WalSetTxPower(NetDevice *netDev, int32_t power)
{
    int retVal = 0;
    struct wiphy *wiphy = NULL;
    //struct wiphy* wiphy = get_krn_wiphy();

    // sync from net_device->ieee80211_ptr
    struct wireless_dev *wdev = GET_NET_DEV_CFG80211_WIRELESS(netDev);

    wiphy = get_linux_wiphy_hdfdev(netDev);
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return -1;
    }

    HDF_LOGE("%s: start...", __func__);

    retVal = (int32_t)wl_cfg80211_ops.set_tx_power(wiphy, wdev, NL80211_TX_POWER_FIXED ,power);

    if (retVal < 0) {
        HDF_LOGE("%s: set_tx_power failed!", __func__);
    }
        
    return HDF_SUCCESS;
}

const struct ieee80211_regdomain * bdh6_get_regdomain(void);


int32_t BDH6WalGetValidFreqsWithBand(NetDevice *netDev, int32_t band, int32_t *freqs, uint32_t *num)
{
    uint32_t freqIndex = 0;
    uint32_t channelNumber;
    uint32_t freqTmp;
    uint32_t minFreq;
    uint32_t maxFreq;
    
    struct wiphy* wiphy = NULL;
    struct ieee80211_supported_band *band5g = NULL;
    int32_t max5GChNum = 0;
    const struct ieee80211_regdomain *regdom = bdh6_get_regdomain();
    if (regdom == NULL) {
        HDF_LOGE("%s: wal_get_cfg_regdb failed!", __func__);
        return HDF_FAILURE;
    }

    wiphy = get_linux_wiphy_hdfdev(netDev);
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return -1;
    }
    
    (void)netDev;
    HDF_LOGE("%s: start..., band=%d", __func__, band);

    minFreq = regdom->reg_rules[0].freq_range.start_freq_khz / MHZ_TO_KHZ(1);
    maxFreq = regdom->reg_rules[0].freq_range.end_freq_khz / MHZ_TO_KHZ(1);
    switch (band) {
        case WLAN_BAND_2G:
            for (channelNumber = 1; channelNumber <= WIFI_24G_CHANNEL_NUMS; channelNumber++) {
                if (channelNumber < WAL_MAX_CHANNEL_2G) {
                    freqTmp = WAL_MIN_FREQ_2G + (channelNumber - 1) * WAL_FREQ_2G_INTERVAL;
                } else if (channelNumber == WAL_MAX_CHANNEL_2G) {
                    freqTmp = WAL_MAX_FREQ_2G;
                }
                if (freqTmp < minFreq || freqTmp > maxFreq) {
                    continue;
                }

                HDF_LOGE("bdh6 2G %u: freq=%u\n", freqIndex, freqTmp);
                freqs[freqIndex] = freqTmp;
                freqIndex++;
            }
            *num = freqIndex;
            break;

        case WLAN_BAND_5G:
            band5g = wiphy->bands[IEEE80211_BAND_5GHZ];
            if (NULL == band5g) {
                HDF_LOGE("%s: bamd5g is null!", __func__);
                return HDF_ERR_NOT_SUPPORT;
            }
            
            //band5g->n_channels;
            max5GChNum = min(band5g->n_channels, WIFI_24G_CHANNEL_NUMS);
            for (freqIndex = 0; freqIndex < max5GChNum; freqIndex++) {
                freqs[freqIndex] = band5g->channels[freqIndex].center_freq;
                HDF_LOGE("bdh6 5G %u: freq=%u\n", freqIndex, freqs[freqIndex]);
            }
            *num = freqIndex;
            break;

        /**
        case WLAN_BAND_5G:
            for (channelNumber = 1; channelNumber <= WIFI_24G_CHANNEL_NUMS; channelNumber++) {
                if (channelNumber < WAL_MAX_CHANNEL_2G) {
                    freqTmp = WAL_MIN_FREQ_2G + (channelNumber - 1) * WAL_FREQ_2G_INTERVAL;
                } else if (channelNumber == WAL_MAX_CHANNEL_2G) {
                    freqTmp = WAL_MAX_FREQ_2G;
                }
                if (freqTmp < minFreq || freqTmp > maxFreq) {
                    continue;
                }

                HDF_LOGE("bdh6 2G %u: freq=%u\n", freqIndex, freqTmp);
                freqs[freqIndex] = freqTmp;
                freqIndex++;
            }
            *num = freqIndex;
            break;
        */    
        default:
            HDF_LOGE("%s: no support band!", __func__);
            return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

void BDH6WalReleaseHwCapability(struct WlanHwCapability *self)
{
    uint8_t i;
    if (self == NULL) {
        return;
    }
    for (i = 0; i < IEEE80211_NUM_BANDS; i++) {
        if (self->bands[i] != NULL) {
            OsalMemFree(self->bands[i]);
            self->bands[i] = NULL;
        }
    }
    if (self->supportedRates != NULL) {
        OsalMemFree(self->supportedRates);
        self->supportedRates = NULL;
    }
    OsalMemFree(self);
}

int32_t BDH6WalGetHwCapability(struct NetDevice *netDev, struct WlanHwCapability **capability)
{
    uint8_t loop = 0;
    struct wiphy* wiphy = NULL;
    struct ieee80211_supported_band *band = NULL;
    struct ieee80211_supported_band *band5g = NULL;
    struct WlanHwCapability *hwCapability = NULL;
    uint16_t supportedRateCount = 0;
    //struct wiphy* wiphy = get_krn_wiphy();

    wiphy = get_linux_wiphy_hdfdev(netDev);
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return -1;
    }

    HDF_LOGE("%s: start...", __func__);
    
    band = wiphy->bands[IEEE80211_BAND_2GHZ];
    hwCapability = (struct WlanHwCapability *)OsalMemCalloc(sizeof(struct WlanHwCapability));

    //(void)netDev;
    if (hwCapability == NULL) {
        HDF_LOGE("%s: oom!\n", __func__);
        return HDF_FAILURE;
    }
    hwCapability->Release = BDH6WalReleaseHwCapability;
    
    if (hwCapability->bands[IEEE80211_BAND_2GHZ] == NULL) {
        hwCapability->bands[IEEE80211_BAND_2GHZ] =
            OsalMemCalloc(sizeof(struct WlanBand) + (sizeof(struct WlanChannel) * band->n_channels));
        if (hwCapability->bands[IEEE80211_BAND_2GHZ] == NULL) {
            HDF_LOGE("%s: oom!\n", __func__);
            BDH6WalReleaseHwCapability(hwCapability);
            return HDF_FAILURE;
        }
    }
    
    hwCapability->htCapability = band->ht_cap.cap;
    supportedRateCount = band->n_bitrates;
    
    hwCapability->bands[IEEE80211_BAND_2GHZ]->channelCount = band->n_channels;
    for (loop = 0; loop < band->n_channels; loop++) {
        hwCapability->bands[IEEE80211_BAND_2GHZ]->channels[loop].centerFreq = band->channels[loop].center_freq;
        hwCapability->bands[IEEE80211_BAND_2GHZ]->channels[loop].flags = band->channels[loop].flags;
        hwCapability->bands[IEEE80211_BAND_2GHZ]->channels[loop].channelId = band->channels[loop].hw_value;
        HDF_LOGE("bdh6 2G band %u: centerFreq=%u, channelId=%u, flags=0x%08x\n", loop, 
            hwCapability->bands[IEEE80211_BAND_2GHZ]->channels[loop].centerFreq, 
            hwCapability->bands[IEEE80211_BAND_2GHZ]->channels[loop].channelId, 
            hwCapability->bands[IEEE80211_BAND_2GHZ]->channels[loop].flags);
    }

    if (wiphy->bands[IEEE80211_BAND_5GHZ]) { // Fill 5Ghz band
        band5g = wiphy->bands[IEEE80211_BAND_5GHZ];
        hwCapability->bands[IEEE80211_BAND_5GHZ] = OsalMemCalloc(sizeof(struct WlanBand) + (sizeof(struct WlanChannel) * band5g->n_channels));
        if (hwCapability->bands[IEEE80211_BAND_5GHZ] == NULL) {
            HDF_LOGE("%s: oom!\n", __func__);
            BDH6WalReleaseHwCapability(hwCapability);
            return HDF_FAILURE;
        }

        hwCapability->bands[IEEE80211_BAND_5GHZ]->channelCount = band5g->n_channels;
        for (loop = 0; loop < band5g->n_channels; loop++) {
            hwCapability->bands[IEEE80211_BAND_5GHZ]->channels[loop].centerFreq = band5g->channels[loop].center_freq;
            hwCapability->bands[IEEE80211_BAND_5GHZ]->channels[loop].flags = band5g->channels[loop].flags;
            hwCapability->bands[IEEE80211_BAND_5GHZ]->channels[loop].channelId = band5g->channels[loop].hw_value;
            HDF_LOGE("bdh6 5G band %u: centerFreq=%u, channelId=%u, flags=0x%08x\n", loop, 
                hwCapability->bands[IEEE80211_BAND_5GHZ]->channels[loop].centerFreq, 
                hwCapability->bands[IEEE80211_BAND_5GHZ]->channels[loop].channelId, 
                hwCapability->bands[IEEE80211_BAND_5GHZ]->channels[loop].flags);
        }

        //hwCapability->htCapability = band5g->ht_cap.cap;
        supportedRateCount += band5g->n_bitrates;
    }

    HDF_LOGE("bdh6 htCapability= %u,%u; supportedRateCount= %u,%u,%u\n", hwCapability->htCapability, 
        band5g->ht_cap.cap, supportedRateCount, band->n_bitrates, band5g->n_bitrates);
    
    hwCapability->supportedRateCount = supportedRateCount;
    hwCapability->supportedRates = OsalMemCalloc(sizeof(uint16_t) * supportedRateCount);
    if (hwCapability->supportedRates == NULL) {
        HDF_LOGE("%s: oom!\n", __func__);
        BDH6WalReleaseHwCapability(hwCapability);
        return HDF_FAILURE;
    }
    
    for (loop = 0; loop < band->n_bitrates; loop++) {
        hwCapability->supportedRates[loop] = band->bitrates[loop].bitrate;
        HDF_LOGE("bdh6 2G supportedRates %u: %u\n", loop, hwCapability->supportedRates[loop]);
    }

    if (band5g) {
        for (loop = band->n_bitrates; loop < supportedRateCount; loop++) {
            hwCapability->supportedRates[loop] = band5g->bitrates[loop].bitrate;
            HDF_LOGE("bdh6 5G supportedRates %u: %u\n", loop, hwCapability->supportedRates[loop]);
        }
    }

    if (hwCapability->supportedRateCount > MAX_SUPPORTED_RATE)
        hwCapability->supportedRateCount = MAX_SUPPORTED_RATE;
    
    *capability = hwCapability;
    return HDF_SUCCESS;
}

int32_t BDH6WalSendAction(struct NetDevice *netDev, WifiActionData *actionData)
{
    (void)netDev;
    (void)actionData;
    HDF_LOGE("%s: start...", __func__);
    return HDF_ERR_NOT_SUPPORT;     
}

int32_t BDH6WalGetIftype(struct NetDevice *netDev, uint8_t *iftype)
{
    iftype = (uint8_t *)(&(GET_NET_DEV_CFG80211_WIRELESS(netDev)->iftype));
    HDF_LOGE("%s: start...", __func__);
    return HDF_SUCCESS;
}

static struct HdfMac80211BaseOps g_bdh6_baseOps = 
{
    .SetMode = BDH6WalSetMode,
    .AddKey = BDH6WalAddKey,
    .DelKey = BDH6WalDelKey,
    .SetDefaultKey = BDH6WalSetDefaultKey,
    
    .GetDeviceMacAddr = BDH6WalGetDeviceMacAddr,
    .SetMacAddr = BDH6WalSetMacAddr,
    .SetTxPower = BDH6WalSetTxPower,
    .GetValidFreqsWithBand = BDH6WalGetValidFreqsWithBand,
    
    .GetHwCapability = BDH6WalGetHwCapability,

    /**
    .RemainOnChannel = WalRemainOnChannel,
    .CancelRemainOnChannel = WalCancelRemainOnChannel,
    .ProbeReqReport = WalProbeReqReport,
    .AddIf = WalAddIf,
    .RemoveIf = WalRemoveIf,
    .SetApWpsP2pIe = WalSetApWpsP2pIe,
    .GetDriverFlag = WalGetDriverFlag,
    */
    .SendAction = BDH6WalSendAction,
    .GetIftype = BDH6WalGetIftype,
    
};

extern struct HdfMac80211STAOps g_bdh6_staOps;
extern struct HdfMac80211APOps g_bdh6_apOps;

void BDH6Mac80211Init(struct HdfChipDriver *chipDriver)
{
    HDF_LOGE("%s: start...", __func__);

    if (chipDriver == NULL) {
        HDF_LOGE("%s: input is NULL", __func__);
        return;
    }
    
    chipDriver->ops = &g_bdh6_baseOps;
    chipDriver->staOps = &g_bdh6_staOps;
    chipDriver->apOps = &g_bdh6_apOps;
    //chipDriver->apOps = NULL;
    chipDriver->p2pOps = NULL;
}

