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

#include "wifi_mac80211_ops.h"
//#include "net_adpater.h"
#include "hdf_wlan_utils.h"
#include "wifi_module.h"
#include <net/cfg80211.h>
#include <net/regulatory.h>
#include "osal_mem.h"
#include "hdf_wifi_event.h"
#include "hdf_log.h"

#include <typedefs.h>
#include <ethernet.h>
#include <bcmutils.h>
#include "net_device_adapter.h"




#define HDF_LOG_TAG bcmdhd
//#define retVal 0
//#define HDF_SUCCESS 1
#ifndef errno_t
typedef int errno_t;
#endif
struct net_device *g_save_kernel_net = NULL;
extern struct net_device_ops dhd_ops_pri;
extern struct cfg80211_ops wl_cfg80211_ops;
extern struct net_device * GetLinuxInfByNetDevice(const struct NetDevice *netDevice);
extern struct wiphy * get_linux_wiphy_ndev(struct net_device *ndev);


extern NetDevice *get_netDev(void);
extern struct net_device *get_krn_netdev(void);
extern struct wireless_dev* wrap_get_widev(void);
extern const struct ieee80211_regdomain * wrp_get_regdomain(void);
extern int32_t wl_get_all_sta(struct net_device *ndev, uint32_t *num);
extern void dhd_get_mac_address(struct net_device *dev, void **addr);
extern s32 wl_get_all_sta_info(struct net_device *ndev, char* mac, uint32_t num);
extern int32_t wal_cfg80211_cancel_remain_on_channel(struct wiphy *wiphy, struct net_device *netDev);
extern int32_t wl_cfg80211_set_wps_p2p_ie_wrp(struct net_device *ndev, char *buf, int len, int8_t type);
extern int32_t wal_cfg80211_remain_on_channel(struct wiphy *wiphy, struct net_device *netDev, int32_t freq,
                                              unsigned int duration);
extern void wl_cfg80211_add_virtual_iface_wrap(struct wiphy *wiphy, char *name, enum nl80211_iftype type, 
                                              struct vif_params *params);
extern int32_t wl_cfg80211_set_country_code(struct net_device *net, char *country_code,
                                             bool notify, bool user_enforced, int revinfo);
extern int32_t HdfWifiEventInformBssFrame(const struct NetDevice *netDev, 
                                          const struct WlanChannel *channel,
                                          const struct ScannedBssInfo *bssInfo);
extern int32_t wl_cfg80211_change_beacon_wrap(struct wiphy *wiphy, struct net_device *dev,
                                            struct cfg80211_beacon_data *info, int interval,
                                            int dtimPeriod, bool hidden_ssid);

extern errno_t memcpy_s(void *dest, size_t dest_max, const void *src, size_t count);
extern int snprintf_s(char *dest, size_t dest_max, size_t count, const char *format, ...);
extern s32 wldev_ioctl_get(struct net_device *dev, u32 cmd, void *arg, u32 len);
extern int32_t wl_get_all_sta(struct net_device *ndev, uint32_t *num);

extern int32_t HdfWifiEventDelSta(struct NetDevice *netDev, const uint8_t *macAddr, uint8_t addrLen);
extern NetDevice * GetHdfNetDeviceByLinuxInf(struct net_device *dev);


/*--------------------------------------------------------------------------------------------------*/
/* public */
/*--------------------------------------------------------------------------------------------------*/
#define oal_net_dev_priv(_pst_dev)                          GET_NET_DEV_PRIV(_pst_dev)
#define MAX_NUM_OF_ASSOCIATED_DEV		64
#define WLC_GET_ASSOCLIST		159
#define dtoh32(i) i



typedef struct maclist {
	uint32 count;			/**< number of MAC addresses */
	struct ether_addr ea[1];	/**< variable length array of MAC addresses */
} maclist_t;

/*
void ChangeStationInfo (struct station_info *info, struct StationInfo *Info) 
{
	Info->assocReqIes = info->assocReqIes;
	Info->assocReqIesLen = info->assocReqIesLen;
	
	return Info;
}*/


int ChangDelSta(struct net_device *dev,const uint8_t *macAddr, uint8_t addrLen){
	int ret;
	struct NetDevice *netDev = GetHdfNetDeviceByLinuxInf(dev);
	ret = HdfWifiEventDelSta(netDev, macAddr, 6);
	return ret;
}

int ChangNewSta(struct net_device *dev, const uint8_t *macAddr, uint8_t addrLen,
    const struct station_info *info)
{
	int ret;
	struct NetDevice *netDev = NULL;
	struct StationInfo Info = {0}; 

//    ChangeStationInfo (info, &Info); 
	Info.assocReqIes = info->assoc_req_ies;
	Info.assocReqIesLen = info->assoc_req_ies_len;
	
	netDev = GetHdfNetDeviceByLinuxInf(dev);	

	ret = HdfWifiEventNewSta(netDev,macAddr,addrLen,&Info);
	return ret;

}



int32_t wl_get_all_sta(struct net_device *ndev, uint32_t *num){
	int ret=0;
	char mac_buf[MAX_NUM_OF_ASSOCIATED_DEV *
			sizeof(struct ether_addr) + sizeof(uint)] = {0};
	struct maclist *assoc_maclist = (struct maclist *)mac_buf;
	assoc_maclist->count = MAX_NUM_OF_ASSOCIATED_DEV;
	ret = wldev_ioctl_get(ndev, WLC_GET_ASSOCLIST,
					assoc_maclist, sizeof(mac_buf));
	*num = assoc_maclist->count;
	return 0; 
}
#define MAX_NUM_OF_ASSOCLIST    64
#define CMD_ASSOC_CLIENTS "ASSOCLIST"
#define htod32(i) i




int wl_get_all_sta_info(struct net_device *ndev, char* mac, uint32_t num){

	int bytes_written = 0;
	int  error = 0;
	int len = 0;
	int i;
	char mac_buf[MAX_NUM_OF_ASSOCLIST *sizeof(struct ether_addr) + sizeof(uint)] = {0};
	struct maclist *assoc_maclist = (struct maclist *)mac_buf;
	assoc_maclist->count = htod32(MAX_NUM_OF_ASSOCLIST);
	error = wldev_ioctl_get(ndev, WLC_GET_ASSOCLIST, assoc_maclist, sizeof(mac_buf));
	if (error)  return -1;
	assoc_maclist->count = dtoh32(assoc_maclist->count);
		bytes_written = snprintf(mac, num, "%s listcount: %d Stations:",
		CMD_ASSOC_CLIENTS, assoc_maclist->count);
	for (i = 0; i < assoc_maclist->count; i++) {
	len = snprintf(mac + bytes_written, num - bytes_written, " " MACDBG,
			MAC2STRDBG(assoc_maclist->ea[i].octet));
		/* A return value of '(total_len - bytes_written)' or more means that the
		 * output was truncated
		 */
		if ((len > 0) && (len < (num - bytes_written))) {
			bytes_written += len;
		} else {
			bytes_written = -1;
			break;
		}
	}
	
	return bytes_written;

}


#if 1

static void bdh6_nl80211_check_ap_rate_selectors(struct cfg80211_ap_settings *params,
					    const u8 *rates)
{
	int i;

	if (!rates)
		return;

	for (i = 0; i < rates[1]; i++) {
		if (rates[2 + i] == BSS_MEMBERSHIP_SELECTOR_HT_PHY)
			params->ht_required = true;
		if (rates[2 + i] == BSS_MEMBERSHIP_SELECTOR_VHT_PHY)
			params->vht_required = true;
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
		if (rates[2 + i] == BSS_MEMBERSHIP_SELECTOR_HE_PHY)
			params->he_required = true;
#endif
	}
}


static void bdh6_nl80211_calculate_ap_params(struct cfg80211_ap_settings *params)
{
	const struct cfg80211_beacon_data *bcn = &params->beacon;
	size_t ies_len = bcn->tail_len;
	const u8 *ies = bcn->tail;
	const u8 *rates;
	const u8 *cap;

	rates = cfg80211_find_ie(WLAN_EID_SUPP_RATES, ies, ies_len);
    HDF_LOGE("lijg: find beacon tail WLAN_EID_SUPP_RATES=%p", rates);
	bdh6_nl80211_check_ap_rate_selectors(params, rates);

	rates = cfg80211_find_ie(WLAN_EID_EXT_SUPP_RATES, ies, ies_len);
    HDF_LOGE("lijg: find beacon tail WLAN_EID_EXT_SUPP_RATES=%p", rates);
	bdh6_nl80211_check_ap_rate_selectors(params, rates);

	cap = cfg80211_find_ie(WLAN_EID_HT_CAPABILITY, ies, ies_len);
    HDF_LOGE("lijg: find beacon tail WLAN_EID_HT_CAPABILITY=%p", cap);
	if (cap && cap[1] >= sizeof(*params->ht_cap))
		params->ht_cap = (void *)(cap + 2);
	cap = cfg80211_find_ie(WLAN_EID_VHT_CAPABILITY, ies, ies_len);
    HDF_LOGE("lijg: find beacon tail WLAN_EID_VHT_CAPABILITY=%p", cap);
	if (cap && cap[1] >= sizeof(*params->vht_cap))
		params->vht_cap = (void *)(cap + 2);
#if (LINUX_VERSION_CODE > KERNEL_VERSION(5, 10, 0))
	cap = cfg80211_find_ext_ie(WLAN_EID_EXT_HE_CAPABILITY, ies, ies_len);
	HDF_LOGE("lijg: find beacon tail WLAN_EID_EXT_HE_CAPABILITY=%p", cap);
	if (cap && cap[1] >= sizeof(*params->he_cap) + 1)
		params->he_cap = (void *)(cap + 3);
	cap = cfg80211_find_ext_ie(WLAN_EID_EXT_HE_OPERATION, ies, ies_len);
	HDF_LOGE("lijg: find beacon tail WLAN_EID_EXT_HE_OPERATION=%p", cap);
	if (cap && cap[1] >= sizeof(*params->he_oper) + 1)
		params->he_oper = (void *)(cap + 3);
#endif
}

#endif


//static struct wireless_dev g_ap_wireless_dev;
static struct ieee80211_channel g_ap_ieee80211_channel;
#define GET_DEV_CFG80211_WIRELESS(dev) ((struct wireless_dev*)((dev)->ieee80211_ptr))

static struct cfg80211_ap_settings g_ap_setting_info;
static u8 g_ap_ssid[IEEE80211_MAX_SSID_LEN];
//static struct ieee80211_channel g_ap_chan;
static int start_ap_flag = 0;

static int32_t SetupWireLessDev(struct net_device *netDev, struct WlanAPConf *apSettings)
{
    struct wireless_dev *wdev = NULL;
    struct ieee80211_channel *chan = NULL;
    
#if 0
    if (netDev->ieee80211_ptr == NULL) {
        netDev->ieee80211_ptr = &g_ap_wireless_dev;
    }

    if (GET_DEV_CFG80211_WIRELESS(netDev)->preset_chandef.chan == NULL) {
        GET_DEV_CFG80211_WIRELESS(netDev)->preset_chandef.chan = &g_ap_ieee80211_channel;
    }
#endif
    wdev = netDev->ieee80211_ptr;
    if (NULL == wdev) {
        HDF_LOGE("%s: wdev is null", __func__);
        return -1;
    }
    HDF_LOGE("%s:%p, chan=%p, channel=%u,centfreq1=%u,centfreq2=%u,band=%u,width=%u", __func__, 
        wdev, wdev->preset_chandef.chan, 
        apSettings->channel, apSettings->centerFreq1, apSettings->centerFreq2, apSettings->band, apSettings->width);

    wdev->iftype = NL80211_IFTYPE_AP;
    wdev->preset_chandef.width = (enum nl80211_channel_type)apSettings->width;
    wdev->preset_chandef.center_freq1 = apSettings->centerFreq1;
    wdev->preset_chandef.center_freq2 = apSettings->centerFreq2;
    
    chan = ieee80211_get_channel(wdev->wiphy, apSettings->centerFreq1);
    if (chan) {
        wdev->preset_chandef.chan = chan;
        HDF_LOGE("%s: use valid channel %u", __func__, chan->center_freq);
    } else if (NULL == wdev->preset_chandef.chan) {
        wdev->preset_chandef.chan = &g_ap_ieee80211_channel;
        wdev->preset_chandef.chan->hw_value = apSettings->channel;
        wdev->preset_chandef.chan->band = apSettings->band; // IEEE80211_BAND_2GHZ;
        wdev->preset_chandef.chan->center_freq = apSettings->centerFreq1;
        HDF_LOGE("%s: fill new channel %u", __func__, wdev->preset_chandef.chan->center_freq);
    }

    g_ap_setting_info.chandef = wdev->preset_chandef;
    
    return HDF_SUCCESS;
}

/*--------------------------------------------------------------------------------------------------*/
/* HdfMac80211APOps */
/*--------------------------------------------------------------------------------------------------*/
int32_t WalConfigAp(NetDevice *netDev, struct WlanAPConf *apConf)
{
    int32_t ret = 0;
    //struct wiphy* wiphy = get_linux_wiphy_ndev();
    //struct net_device *netdev = get_krn_netdev();
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;
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

    memset(&g_ap_setting_info, 0x00, sizeof(g_ap_setting_info));
    ret = SetupWireLessDev(netdev, apConf);
    if (0 != ret) {
        HDF_LOGE("%s: set up wireless device failed!", __func__);
        return HDF_FAILURE;
    }

    HDF_LOGE("%s: before iftype = %d!", __func__, netdev->ieee80211_ptr->iftype);
    netdev->ieee80211_ptr->iftype = NL80211_IFTYPE_AP;
    HDF_LOGE("%s: after  iftype = %d!", __func__, netdev->ieee80211_ptr->iftype);

    g_ap_setting_info.ssid_len = apConf->ssidConf.ssidLen;
    memcpy(&g_ap_ssid[0], &apConf->ssidConf.ssid[0], apConf->ssidConf.ssidLen);
    g_ap_setting_info.ssid = &g_ap_ssid[0];

#if 0    
    g_ap_setting_info.chandef.center_freq1 = apConf->centerFreq1;
    g_ap_setting_info.chandef.center_freq2 = apConf->centerFreq2;
    g_ap_setting_info.chandef.width = apConf->width;

    g_ap_chan.center_freq = apConf->centerFreq1;
    g_ap_chan.hw_value = apConf->channel;
    g_ap_chan.band = apConf->band;
    g_ap_chan.band = IEEE80211_BAND_2GHZ;
    g_ap_setting_info.chandef.chan = &g_ap_chan;
#endif

    ret = (int32_t)wl_cfg80211_ops.change_virtual_intf(wiphy, netdev,
                                        (enum nl80211_iftype)netdev->ieee80211_ptr->iftype, NULL);
    if (ret < 0) {
        HDF_LOGE("%s: set mode failed!", __func__);
    }

    return HDF_SUCCESS;
}

int32_t WalSetCountryCode(NetDevice *netDev, const char *code, uint32_t len)
{
    int32_t ret = 0;
    struct net_device *netdev = NULL;
	 netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return -1;
    }   

    HDF_LOGE("%s: start...", __func__);

    ret = (int32_t)wl_cfg80211_set_country_code(netdev, (char*)code, false, true, len);
	
    if (ret < 0) {
        HDF_LOGE("%s: set_country_code failed!", __func__);
    }
	
    return ret;
}

int32_t WalStopAp(NetDevice *netDev)
{
	
    int32_t ret = 0;
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;
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

    HDF_LOGE("%s: start...", __func__);

    ret = (int32_t)wl_cfg80211_ops.stop_ap(wiphy, netdev);



    return ret;
}

int32_t WalStartAp(NetDevice *netDev)
{   

    int32_t ret = 0;
#if 1

    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;
    struct wireless_dev *wdev = NULL;

    if (start_ap_flag) {
        WalStopAp(netDev);
        HDF_LOGE("do not start up, because start_ap_flag=%d", start_ap_flag);
        //return ret;
    }
    
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
    wdev = netdev->ieee80211_ptr;

    HDF_LOGE("%s: start...", __func__);
    //GET_DEV_CFG80211_WIRELESS(netdev)->preset_chandef.chan->center_freq = 
    //                         netdev->ieee80211_ptr->preset_chandef.center_freq1;
    ret = (int32_t)wl_cfg80211_ops.start_ap(wiphy, netdev, &g_ap_setting_info);
	
    if (ret < 0) {
        HDF_LOGE("%s: start_ap failed!", __func__);
    } else {
        wdev->preset_chandef = g_ap_setting_info.chandef;
		wdev->beacon_interval = g_ap_setting_info.beacon_interval;
		wdev->chandef = g_ap_setting_info.chandef;
		wdev->ssid_len = g_ap_setting_info.ssid_len;
		memcpy(wdev->ssid, g_ap_setting_info.ssid, wdev->ssid_len);
        start_ap_flag = 1;
    }
#endif
    return ret;
}


int32_t WalChangeBeacon(NetDevice *netDev, struct WlanBeaconConf *param)
{
	
    int32_t ret = 0;
    struct cfg80211_beacon_data info;
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;
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

    HDF_LOGE("%s: start...", __func__);

    memset(&info, 0x00, sizeof(info));

#if 1
    if (g_ap_setting_info.beacon.head != NULL) {
        OsalMemFree((uint8_t *)g_ap_setting_info.beacon.head);
        g_ap_setting_info.beacon.head = NULL;
    }
    if (g_ap_setting_info.beacon.tail != NULL) {
        OsalMemFree((uint8_t *)g_ap_setting_info.beacon.tail);
        g_ap_setting_info.beacon.tail = NULL;
    }

    if (param->headIEs && param->headIEsLength > 0) {
        g_ap_setting_info.beacon.head = OsalMemCalloc(param->headIEsLength);
        memcpy((uint8_t *)g_ap_setting_info.beacon.head, param->headIEs, param->headIEsLength);
        g_ap_setting_info.beacon.head_len = param->headIEsLength;

        //info.head = g_ap_setting_info.beacon.head;
        //info.head_len = g_ap_setting_info.beacon.head_len;
    }

    if (param->tailIEs && param->tailIEsLength > 0) {
        g_ap_setting_info.beacon.tail = OsalMemCalloc(param->tailIEsLength);
        memcpy((uint8_t *)g_ap_setting_info.beacon.tail, param->tailIEs, param->tailIEsLength);
        g_ap_setting_info.beacon.tail_len = param->tailIEsLength;

        //info.tail = g_ap_setting_info.beacon.tail;
        //info.tail_len = g_ap_setting_info.beacon.tail_len;
    }
#endif    
    
    info.head = param->headIEs;
    info.head_len = (size_t)param->headIEsLength;
    info.tail = param->tailIEs;
    info.tail_len = (size_t)param->tailIEsLength;

    info.beacon_ies = NULL;
    info.proberesp_ies = NULL;
    info.assocresp_ies = NULL;
    info.probe_resp = NULL;
    //info.lci = NULL;
    //info.civicloc = NULL;
    //info.ftm_responder = 0X00;
    info.beacon_ies_len = 0X00;
    info.proberesp_ies_len = 0X00;
    info.assocresp_ies_len = 0X00;
    info.probe_resp_len = 0X00;
    //info.lci_len = 0X00;
    //info.civicloc_len = 0X00;

    /* add beacon data for start ap*/
    g_ap_setting_info.dtim_period = param->DTIMPeriod;
    g_ap_setting_info.hidden_ssid = param->hiddenSSID;
    g_ap_setting_info.beacon_interval = param->interval;
    HDF_LOGE("%s: dtim_period:%d---hidden_ssid:%d---beacon_interval:%d!",
        __func__, g_ap_setting_info.dtim_period, g_ap_setting_info.hidden_ssid, g_ap_setting_info.beacon_interval);
	
    //g_ap_setting_info.beacon.head = param->headIEs;
    //g_ap_setting_info.beacon.head_len = param->headIEsLength;
    //g_ap_setting_info.beacon.tail = param->tailIEs;
    //g_ap_setting_info.beacon.tail_len = param->tailIEsLength;

    g_ap_setting_info.beacon.beacon_ies = NULL;
    g_ap_setting_info.beacon.proberesp_ies = NULL;
    g_ap_setting_info.beacon.assocresp_ies = NULL;
    g_ap_setting_info.beacon.probe_resp = NULL;
    //g_ap_setting_info.beacon.lci = NULL;
    // g_ap_setting_info.beacon.civicloc = NULL;
    // g_ap_setting_info.beacon.ftm_responder = 0X00;
    g_ap_setting_info.beacon.beacon_ies_len = 0X00;
    g_ap_setting_info.beacon.proberesp_ies_len = 0X00;
    g_ap_setting_info.beacon.assocresp_ies_len = 0X00;
    g_ap_setting_info.beacon.probe_resp_len = 0X00;
    // g_ap_setting_info.beacon.lci_len = 0X00;
    // g_ap_setting_info.beacon.civicloc_len = 0X00;

    // call nl80211_calculate_ap_params(&params);
    bdh6_nl80211_calculate_ap_params(&g_ap_setting_info);

    HDF_LOGE("%s: headIEsLen:%d---tailIEsLen:%d!", __func__, param->headIEsLength, param->tailIEsLength);
    ret = WalStartAp(netDev);
    HDF_LOGE("call start_ap ret=%d", ret);

    ret = (int32_t)wl_cfg80211_ops.change_beacon(wiphy, netdev, &info);
	
    if (ret < 0) {
        HDF_LOGE("%s: change_beacon failed!", __func__);
    }
	
    return HDF_SUCCESS;
}

int32_t WalDelStation(NetDevice *netDev, const uint8_t *macAddr)
{
	int32_t ret = 0;
    struct net_device *netdev = NULL;
    struct wiphy *wiphy = NULL;
	struct station_del_parameters del_param = {macAddr, 10, 0};
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

   

    //(void)netDev;
    (void)macAddr;
    HDF_LOGE("%s: start...", __func__);

    ret = (int32_t)wl_cfg80211_ops.del_station(wiphy, netdev, &del_param);
	
    if (ret < 0) {
        HDF_LOGE("%s: del_station failed!", __func__);
    }
	
    return ret;
}

int32_t WalGetAssociatedStasCount(NetDevice *netDev, uint32_t *num)
{


	int32_t ret = 0;
#if 1
    struct net_device *netdev = NULL;
	 netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return -1;
    }

    HDF_LOGE("%s: start...", __func__);

    ret = (int32_t)wl_get_all_sta(netdev, num);
   
    if (ret < 0) {
        HDF_LOGE("%s: wl_get_all_sta failed!", __func__);
    } 
    
#endif
    return ret;       
}

int32_t WalGetAssociatedStasInfo(NetDevice *netDev, WifiStaInfo *staInfo, uint32_t num) 
{
	int32_t ret = 0;
    struct net_device *netdev = NULL;
	 netdev = GetLinuxInfByNetDevice(netDev);
    if (!netdev) {
        HDF_LOGE("%s: net_device is NULL", __func__);
        return -1;
    }
    

    HDF_LOGE("%s: start...", __func__);

    ret = (int32_t)wl_get_all_sta_info(netdev, staInfo->mac, num);
	
    if (ret < 0) {
        HDF_LOGE("%s: wl_get_all_sta_info failed!", __func__);
    }
	
    return ret;        
}
/*--------------------------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------------------------------*/


struct HdfMac80211APOps g_bdh6_apOps = 
{
    .ConfigAp = WalConfigAp,
    .StartAp = WalStartAp,
    .StopAp = WalStopAp,
    .ConfigBeacon = WalChangeBeacon,
    .DelStation = WalDelStation,
    .SetCountryCode = WalSetCountryCode,
    .GetAssociatedStasCount = WalGetAssociatedStasCount,
    .GetAssociatedStasInfo = WalGetAssociatedStasInfo
};


