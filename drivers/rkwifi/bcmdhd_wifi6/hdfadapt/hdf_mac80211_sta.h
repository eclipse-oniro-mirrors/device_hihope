#ifndef WAL_MAC80211_STA_H_
#define WAL_MAC80211_STA_H_

#include "wifi_mac80211_ops.h"

int32_t HdfStartScan(NetDevice *netdev, struct WlanScanRequest *scanParam);
int32_t HdfAbortScan(NetDevice *netDev);
int32_t HdfConnect(NetDevice *netDev, WlanConnectParams *param);
int32_t HdfDisconnect(NetDevice *netDev, uint16_t reasonCode);
int32_t HdfSetScanningMacAddress(NetDevice *netDev, unsigned char *mac, uint32_t len);

#endif


