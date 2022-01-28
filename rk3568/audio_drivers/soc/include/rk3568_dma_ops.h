/*
 * Copyright (C) 2022 HiHope Open Source Organization .
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef RK3568_PLATFORM_OPS_H
#define RK3568_PLATFORM_OPS_H

#include "audio_core.h"
#include <linux/dmaengine.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

int32_t AudioDmaDeviceInit(const struct AudioCard *card, const struct PlatformDevice *platform);
int32_t Rk3568DmaBufAlloc(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t Rk3568DmaBufFree(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t Rk3568DmaRequestChannel(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t Rk3568DmaConfigChannel(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t Rk3568PcmPointer(struct PlatformData *data, const enum AudioStreamType streamType, uint32_t *pointer);
int32_t Rk3568DmaPrep(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t Rk3568DmaSubmit(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t Rk3568DmaPending(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t Rk3568DmaPause(struct PlatformData *data, const enum AudioStreamType streamType);
int32_t Rk3568DmaResume(struct PlatformData *data, const enum AudioStreamType streamType);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* RK3568_PLATFORM_OPS_H */
