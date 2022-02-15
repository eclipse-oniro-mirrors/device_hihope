/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <net/netlink.h>
#include <net/cfg80211.h>

#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/in6.h>
#include <linux/wireless.h>


#include <securec.h>

#include "osal_mem.h"

#include "net_device.h"
#include "net_device_impl.h"
#include "net_device_adapter.h"
#include "wifi_mac80211_ops.h"
#include "hdf_wifi_cmd.h"
#include "hdf_wifi_event.h"

#include "hdf_public_ap6275s.h"
#include "hdf_mac80211_sta.h"

#define HDF_LOG_TAG BDH6Driver


#define WIFI_SCAN_EXTRA_IE_LEN_MAX      (512)

#define BDH6_POINT_CHANNEL_SIZE (8)

static struct ieee80211_channel *GetChannelByFreq(const struct wiphy *wiphy, uint16_t center_freq)
{
    enum Ieee80211Band band;
    struct ieee80211_supported_band *currentBand = NULL;
    int32_t loop;
    for (band = (enum Ieee80211Band)0; band < IEEE80211_NUM_BANDS; band++) {
        currentBand = wiphy->bands[band];
        if (currentBand == NULL) {
            continue;
        }
        for (loop = 0; loop < currentBand->n_channels; loop++) {
            if (currentBand->channels[loop].center_freq == center_freq) {
                return &currentBand->channels[loop];
            }
        }
    }
    return NULL;
}

static int32_t FillScanChannel(const struct wiphy *wiphy, int32_t channelTotal,
    enum Ieee80211Band band, struct cfg80211_scan_request *request)
{
    int32_t loop;
    int32_t count = 0;
    struct ieee80211_channel *chan = NULL;
    
    for (loop = 0; loop < (int32_t)wiphy->bands[band]->n_channels; loop++) {
        if (count >= channelTotal) {
            break;
        }
        
        chan = &wiphy->bands[band]->channels[loop];
        if ((chan->flags & WIFI_CHAN_DISABLED) != 0) {
            continue;
        }
    
        request->channels[count++] = chan;
    }

    return count;
}


static int32_t WifiScanSetChannel(const struct wiphy *wiphy, const struct WlanScanRequest *params,
    struct cfg80211_scan_request *request)
{
    int32_t loop;
    int32_t count = 0;
    enum Ieee80211Band band = IEEE80211_BAND_2GHZ;
    struct ieee80211_channel *chan = NULL;

    int32_t channelTotal = ieee80211_get_num_supported_channels((struct wiphy *)wiphy);

    if ((params->freqs == NULL) || (params->freqsCount == 0)) {
        for (band = IEEE80211_BAND_2GHZ; band <= IEEE80211_BAND_5GHZ; band++) {
            if (wiphy->bands[band] == NULL) {
                HDF_LOGE("%s: wiphy->bands[band] = NULL!\n", __func__);
                continue;
            }

            count = FillScanChannel(wiphy, channelTotal, band, request);
        }
    } else {
        for (loop = 0; loop < params->freqsCount; loop++) {
            chan = GetChannelByFreq(wiphy, (uint16_t)(params->freqs[loop]));
            if (chan == NULL) {
                HDF_LOGE("%s: freq not found!freq=%d!\n", __func__, params->freqs[loop]);
                continue;
            }

            if (count >= channelTotal) {
                break;
            }
            
            request->channels[count++] = chan;
        }
    }

    if (count == 0) {
        HDF_LOGE("%s: invalid freq info!\n", __func__);
        return HDF_FAILURE;
    }
    request->n_channels = count;

    return HDF_SUCCESS;
}

static int32_t WifiScanSetSsid(const struct WlanScanRequest *params, struct cfg80211_scan_request *request)
{
    int32_t count = 0;
    int32_t loop;

    if (params->ssidCount > WPAS_MAX_SCAN_SSIDS) {
        HDF_LOGE("%s:unexpected numSsids!numSsids=%u", __func__, params->ssidCount);
        return HDF_FAILURE;
    }

    if (params->ssidCount == 0) {
        HDF_LOGE("%s:ssid number is 0!", __func__);
        return HDF_SUCCESS;
    }

    request->ssids = (struct cfg80211_ssid *)OsalMemCalloc(params->ssidCount * sizeof(struct cfg80211_ssid));
    if (request->ssids == NULL) {
        HDF_LOGE("%s: calloc request->ssids null", __func__);
        return HDF_FAILURE;
    }

    for (loop = 0; loop < params->ssidCount; loop++) {
        if (count >= DRIVER_MAX_SCAN_SSIDS) {
            break;
        }

        if (params->ssids[loop].ssidLen > IEEE80211_MAX_SSID_LEN) {
            continue;
        }

        request->ssids[count].ssid_len = params->ssids[loop].ssidLen;
        if (memcpy_s(request->ssids[count].ssid, OAL_IEEE80211_MAX_SSID_LEN, params->ssids[loop].ssid,
            params->ssids[loop].ssidLen) != EOK) {
            continue;
        }
        count++;
    }
    request->n_ssids = count;

    return HDF_SUCCESS;
}

static int32_t WifiScanSetUserIe(const struct WlanScanRequest *params, struct cfg80211_scan_request *request)
{
    if (params->extraIEsLen > WIFI_SCAN_EXTRA_IE_LEN_MAX) {
        HDF_LOGE("%s:unexpected extra len!extraIesLen=%d", __func__, params->extraIEsLen);
        return HDF_FAILURE;
    }
    if ((params->extraIEs != NULL) && (params->extraIEsLen != 0)) {
        request->ie = (uint8_t *)OsalMemCalloc(params->extraIEsLen);
        if (request->ie == NULL) {
            HDF_LOGE("%s: calloc request->ie null", __func__);
            return HDF_FAILURE;
        }
        (void)memcpy_s((void *)request->ie, params->extraIEsLen, params->extraIEs, params->extraIEsLen);
        request->ie_len = params->extraIEsLen;
    }

    return HDF_SUCCESS;
}

static int32_t WifiScanSetRequest(struct NetDevice *netdev, const struct WlanScanRequest *params,
    struct cfg80211_scan_request *request)
{
    if (netdev == NULL || netdev->ieee80211Ptr == NULL) {
        return HDF_FAILURE;
    }
    request->wiphy = GET_NET_DEV_CFG80211_WIRELESS(netdev)->wiphy;
    request->wdev = GET_NET_DEV_CFG80211_WIRELESS(netdev);
    request->n_ssids = params->ssidCount;
    if (WifiScanSetChannel(GET_NET_DEV_CFG80211_WIRELESS(netdev)->wiphy, params, request)) {
        HDF_LOGE("%s: set channel failed!", __func__);
        return HDF_FAILURE;
    }
    if (WifiScanSetSsid(params, request)) {
        HDF_LOGE("%s: set ssid failed!", __func__);
        return HDF_FAILURE;
    }
    if (WifiScanSetUserIe(params, request)) {
        HDF_LOGE("%s: set user ie failed!", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static void WifiScanFree(struct cfg80211_scan_request **request)
{
    HDF_LOGE("%s: enter... !", __func__);

    if (*request != NULL) {
        if ((*request)->ie != NULL) {
            OsalMemFree((void *)(*request)->ie);
            (*request)->ie = NULL;
        }
        if ((*request)->ssids != NULL) {
            OsalMemFree((void *)(*request)->ssids);
            (*request)->ssids = NULL;
        }
        OsalMemFree((void *)*request);
        *request = NULL;
    }
}

int32_t HdfStartScan(NetDevice *netDev, struct WlanScanRequest *scanParam)
{
    int32_t ret = 0;

    struct net_device *ndev = GetLinuxInfByNetDevice(netDev);
    struct wiphy *wiphy = get_linux_wiphy_ndev(ndev);

    int32_t channelTotal = ieee80211_get_num_supported_channels(wiphy);

    struct cfg80211_scan_request *request =
        (struct cfg80211_scan_request *)OsalMemCalloc(sizeof(struct cfg80211_scan_request) +
        BDH6_POINT_CHANNEL_SIZE * channelTotal);

    HDF_LOGE("%s: enter hdfStartScan, channelTotal: %d", __func__, channelTotal);

    if (request == NULL) {
        HDF_LOGE("%s: calloc request null!\n", __func__);
        return HDF_FAILURE;
    }
    if (WifiScanSetRequest(netDev, scanParam, request) != HDF_SUCCESS) {
        WifiScanFree(&request);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: enter cfg80211_scan !", __func__);
    ret = wl_cfg80211_ops.scan(wiphy, request);

    HDF_LOGE("%s: left cfg80211_scan !", __func__);

    if (ret != HDF_SUCCESS) {
        WifiScanFree(&request);
    }

    return ret;
}

int32_t HdfAbortScan(NetDevice *netDev)
{
    struct net_device *ndev = NULL;
    struct wireless_dev *wdev = NULL;
    struct wiphy *wiphy = NULL;

    if (netDev == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: enter", __func__);

    ndev = GetLinuxInfByNetDevice(netDev);

    wiphy = get_linux_wiphy_ndev(ndev);

    if (ndev == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }

    wdev = ndev->ieee80211_ptr;

    if (!wdev || !wdev->wiphy) {
        return HDF_FAILURE;
    }

    wl_cfg80211_abort_scan(wiphy, wdev);

    return HDF_SUCCESS;
}

static struct ieee80211_channel *WalGetChannel(struct wiphy *wiphy, int32_t freq)
{
    int32_t loop;

    enum Ieee80211Band band = IEEE80211_BAND_2GHZ;
    struct ieee80211_supported_band *currentBand = NULL;

    if (wiphy == NULL) {
        HDF_LOGE("%s: capality is NULL!", __func__);
        return NULL;
    }

    for (band = (enum Ieee80211Band)0; band < IEEE80211_NUM_BANDS; band++) {
        currentBand = wiphy->bands[band];
        if (currentBand == NULL) {
            continue;
        }

        for (loop = 0; loop < currentBand->n_channels; loop++) {
            if (currentBand->channels[loop].center_freq == freq) {
                return &currentBand->channels[loop];
            }
        }
    }

    return NULL;
}

static void HdfFillConnParam(struct cfg80211_connect_params *cfg_param, WlanConnectParams *param)
{
    cfg_param->bssid = param->bssid;
    cfg_param->ssid = param->ssid;
    cfg_param->ie = param->ie;
    cfg_param->ssid_len = param->ssidLen;
    cfg_param->ie_len = param->ieLen;

    cfg_param->crypto.wpa_versions = param->crypto.wpaVersions;
    cfg_param->crypto.cipher_group = param->crypto.cipherGroup;
    cfg_param->crypto.n_ciphers_pairwise = param->crypto.n_ciphersPairwise;
                                                      
    memcpy_s(cfg_param->crypto.ciphers_pairwise,
        NL80211_MAX_NR_CIPHER_SUITES*sizeof(cfg80211_params.crypto.ciphers_pairwise[0]),
        param->crypto.ciphersPairwise, NL80211_MAX_NR_CIPHER_SUITES*sizeof(param->crypto.ciphersPairwise[0]));

    memcpy_s(cfg_param->crypto.akm_suites,
        NL80211_MAX_NR_AKM_SUITES*sizeof(cfg80211_params.crypto.akm_suites[0]),
        param->crypto.akmSuites, NL80211_MAX_NR_AKM_SUITES*sizeof(param->crypto.akmSuites[0]));

    cfg_param->crypto.n_akm_suites = param->crypto.n_akmSuites;

    if (param->crypto.controlPort) {
        cfg_param->crypto.control_port = true;
    } else {
        cfg_param->crypto.control_port = false;
    }

    cfg_param->crypto.control_port_ethertype = param->crypto.controlPortEthertype;
    cfg_param->crypto.control_port_no_encrypt = param->crypto.controlPortNoEncrypt;
    
    cfg_param->key = param->key;
    cfg_param->auth_type = (unsigned char)param->authType;
    cfg_param->privacy = param->privacy;
    cfg_param->key_len = param->keyLen;
    cfg_param->key_idx = param->keyIdx;
    cfg_param->mfp = (unsigned char)param->mfp;
}

int32_t HdfConnect(NetDevice *netDev, WlanConnectParams *param)
{
    int32_t ret = 0;
    struct net_device *ndev = NULL;
    struct wiphy *wiphy = NULL;
    struct cfg80211_connect_params cfg80211_params = { 0 };

    if (netDev == NULL || param == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }
    
    ndev = GetLinuxInfByNetDevice(netDev);
    if (ndev == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }

    wiphy = get_linux_wiphy_ndev(ndev);
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return -1;
    }

    if (param->centerFreq != WLAN_FREQ_NOT_SPECFIED) {
        cfg80211_params.channel = WalGetChannel(wiphy, param->centerFreq);
        if ((cfg80211_params.channel == NULL) || (cfg80211_params.channel->flags & WIFI_CHAN_DISABLED)) {
            HDF_LOGE("%s:illegal channel.flags=%u", __func__,
                (cfg80211_params.channel == NULL) ? 0 : cfg80211_params.channel->flags);
            return HDF_FAILURE;
        }
    }

    HdfFillConnParam(&cfg80211_params, param);
    ret = wl_cfg80211_ops.connect(wiphy, ndev, &cfg80211_params);
    if (ret < 0) {
        HDF_LOGE("%s: connect failed!\n", __func__);
    }

    return ret;
}

int32_t HdfDisconnect(NetDevice *netDev, uint16_t reasonCode)
{
    int32_t ret = 0;
    struct net_device *ndev = NULL;
    struct wiphy *wiphy = NULL;
    
    HDF_LOGE("%s: start...", __func__);
    
    if (netDev == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }
    ndev = GetLinuxInfByNetDevice(netDev);
    if (ndev == NULL) {
        HDF_LOGE("%s:NULL ptr!", __func__);
        return HDF_FAILURE;
    }
    wiphy = get_linux_wiphy_ndev(ndev);
    if (!wiphy) {
        HDF_LOGE("%s: wiphy is NULL", __func__);
        return -1;
    }
    ret = wl_cfg80211_ops.disconnect(wiphy, ndev, reasonCode);
    return ret;
}

int32_t HdfSetScanningMacAddress(NetDevice *netDev, unsigned char *mac, uint32_t len)
{
    (void)netDev;
    (void)mac;
    (void)len;
    return HDF_ERR_NOT_SUPPORT;
}

struct HdfMac80211STAOps g_bdh6_staOps = {
    .Connect = HdfConnect,
    .Disconnect = HdfDisconnect,
    .StartScan = HdfStartScan,
    .AbortScan = HdfAbortScan,
    .SetScanningMacAddress = HdfSetScanningMacAddress,
};

