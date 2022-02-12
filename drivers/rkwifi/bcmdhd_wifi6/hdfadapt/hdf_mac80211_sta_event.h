#ifndef WAL_MAC80211_STA_EVENT_H_
#define WAL_MAC80211_STA_EVENT_H_

//#include "wifi_mac80211_ops.h"
//#include "hdf_wifi_cmd.h"
//#include "hdf_wifi_event.h"

//typedef  WifiScanStatus  HdfWifiScanStatus

//#define ETH_ADDR_LEN   (6)
#define HDF_ETHER_ADDR_LEN (6)
#include <net/cfg80211.h>

typedef struct _HdfConnetResult {
//    uint8_t   bssid[ETH_ADDR_LEN];  /**< MAC address of the AP associated with the station */
    uint8_t   bssid[HDF_ETHER_ADDR_LEN];  /**< MAC address of the AP associated with the station */
    uint16_t  statusCode;           /**< 16-bit status code defined in the IEEE protocol */
    uint8_t  *rspIe;                /**< Association response IE */
    uint8_t  *reqIe;                /**< Association request IE */
    uint32_t  reqIeLen;             /**< Length of the association request IE */
    uint32_t  rspIeLen;             /**< Length of the association response IE */
    uint16_t  connectStatus;        /**< Connection status */
    uint16_t  freq;                 /**< Frequency of the AP */
} HdfConnetResult;

typedef enum {
    HDF_WIFI_SCAN_SUCCESS,
    HDF_WIFI_SCAN_FAILED,
    HDF_WIFI_SCAN_REFUSED,
    HDF_WIFI_SCAN_TIMEOUT
} HdfWifiScanStatus;
 
extern int32_t HdfScanEventCallback(struct net_device *ndev, HdfWifiScanStatus status);

extern int32_t HdfConnectResultEventCallback(struct net_device *ndev, uint8_t *bssid, uint8_t *reqIe,
                           uint8_t *rspIe, uint32_t reqIeLen,
                           uint32_t rspIeLen, uint16_t connectStatus, uint16_t freq);

extern void HdfInformBssFrameEventCallback(struct net_device *ndev, struct ieee80211_channel *channel, int32_t signal, 
                      int16_t freq, struct ieee80211_mgmt *mgmt, uint32_t mgmtLen);

extern int32_t HdfDisconnectedEventCallback(struct net_device *ndev, uint16_t reason, uint8_t *ie, uint32_t len);                           

#endif


