/*
   Copyright (c) [2022] [huangji@nj.iscas.ac.cn]
   [Software Name] is licensed under Mulan PSL v2.
   You can use this software according to the terms and conditions of the Mulan PSL v2. 
   You may obtain a copy of Mulan PSL v2 at:
               http://license.coscl.org.cn/MulanPSL2 
   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.  
   See the Mulan PSL v2 for more details.  
*/

#ifndef RK809_ACCESSORY_IMPL_H
#define RK809_ACCESSORY_IMPL_H

#include "audio_codec_if.h"
#include "audio_dai_if.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "osal_io.h"
#include "securec.h"
#include <linux/platform_device.h>
#include <linux/types.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

struct Rk809ChipData {
    struct CodecData codec;
    struct DaiData dai;
    struct HdfDeviceObject *hdev;
    struct platform_device *pdev;
	struct regmap *regmap;
};

typedef enum {
    RK809_SRT_00 = 0x00,
    RK809_SRT_01 = 0x01,
    RK809_SRT_02 = 0x02,
    RK809_SRT_03 = 0x03,
} RK809SampleRateTimes;

typedef enum {
    RK809_PREMODE_1 = 0x03,
    RK809_PREMODE_2 = 0x06,
    RK809_PREMODE_3 = 0x0C,
    RK809_PREMODE_4 = 0x18,
} RK809PLLInputCLKPreDIV;

typedef enum {
    RK809_VDW_16BITS = 0x0F,
    RK809_VDW_24BITS = 0x17,
} RK809_VDW;

struct RK809DaiParamsVal {
    uint32_t frequencyVal;
    uint32_t DataWidthVal;
};

int32_t Rk809DeviceInit(struct AudioCard *audioCard, const struct CodecDevice *device);
int32_t Rk809DeviceRegRead(uint32_t reg, uint32_t *val);
int32_t Rk809DeviceRegWrite(uint32_t reg, uint32_t value);
int32_t RK809CodecReadReg(unsigned long virtualAddress,uint32_t reg, uint32_t *val);
int32_t RK809CodecWriteReg(unsigned long virtualAddress,uint32_t reg, uint32_t value);
int32_t RK809RegBitsUpdate(struct AudioMixerControl regAttr);
int32_t Rk809DaiDeviceInit(struct AudioCard *card, const struct DaiDevice *device);
int32_t Rk809DaiStartup(const struct AudioCard *card, const struct DaiDevice *device);
int32_t Rk809DaiHwParams(const struct AudioCard *card, const struct AudioPcmHwParams *param);
int32_t RK809NormalTrigger(const struct AudioCard *card, int cmd, const struct DaiDevice *device);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
