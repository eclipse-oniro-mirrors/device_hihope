/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef RK3568_PLATFORM_OPS_H
#define RK3568_PLATFORM_OPS_H

#include "audio_core.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

#include <linux/dmaengine.h>

int32_t AudioDmaDeviceInit(const struct AudioCard *card, const struct PlatformDevice *platform);
int32_t Rk3568DmaBufAlloc(struct PlatformData *data, enum AudioStreamType streamType);
int32_t Rk3568DmaBufFree(struct PlatformData *data, enum AudioStreamType streamType);
int32_t Rk3568DmaRequestChannel(struct PlatformData *data);
int32_t Rk3568DmaConfigChannel(struct PlatformData *data);
int32_t Rk3568PcmPointer(struct PlatformData *data, uint32_t *pointer);
int32_t Rk3568DmaPrep(struct PlatformData *data);
int32_t Rk3568DmaSubmit(struct PlatformData *data);
int32_t Rk3568DmaPending(struct PlatformData *data);
int32_t Rk3568DmaPause(struct PlatformData *data);
int32_t Rk3568DmaResume(struct PlatformData *data);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* RK3568_PLATFORM_OPS_H */
