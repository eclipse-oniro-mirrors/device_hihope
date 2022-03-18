/*
 * Copyright (c) 2022 Institute of Software, CAS.
 * Author : huangji@nj.iscas.ac.cn
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <linux/regmap.h>
#include "audio_accessory_base.h"
#include "gpio_if.h"
#include "linux/of_gpio.h"
#include "audio_driver_log.h"
#include "audio_stream_dispatch.h"
#include "audio_codec_base.h"
#include "rk817_codec.h"
#include "rk809_codec_impl.h"

#define HDF_LOG_TAG "rk809_codec"

/* RK809 I2C Device address  */
#define RK809_I2C_DEV_ADDR     (0x20)
#define RK809_I2C_BUS_NUMBER   (0)      // i2c0
#define RK809_I2C_WAIT_TIMES   (10)     // ms

struct Rk809TransferData {
    uint16_t i2cDevAddr;
    uint16_t i2cBusNumber;
    uint32_t CfgCtrlCount;
    struct AudioRegCfgGroupNode **RegCfgGroupNode;
    struct AudioKcontrol *Controls;
};


static const struct AudioSapmRoute g_audioRoutes[] = {
    { "SPKL", "Dacl enable", "DACL"},
    { "SPKR", "Dacr enable", "DACR"},

    { "ADCL", NULL, "LPGA"},
    { "LPGA", "LPGA MIC Switch", "MIC"},

    { "ADCR", NULL, "RPGA"},
    { "RPGA", "RPGA MIC Switch", "MIC"},
};

static const struct reg_default rk817_render_start_reg_defaults[] = {
    { RK817_CODEC_ADAC_CFG1, 0x00 },
    { RK817_CODEC_DDAC_POPD_DACST, 0x04 },
    { RK817_CODEC_DDAC_MUTE_MIXCTL, 0x10 },
};

static const struct regmap_config rk817_render_start_regmap_config = {
    .reg_defaults = rk817_render_start_reg_defaults,
    .num_reg_defaults = ARRAY_SIZE(rk817_render_start_reg_defaults),
};

static const struct reg_default rk817_render_stop_reg_defaults[] = {
    { RK817_CODEC_ADAC_CFG1, 0x0f },
    { RK817_CODEC_DDAC_POPD_DACST, 0x06 },
    { RK817_CODEC_DDAC_MUTE_MIXCTL, 0x11 },
};

static const struct regmap_config rk817_render_stop_regmap_config = {
    .reg_defaults = rk817_render_stop_reg_defaults,
    .num_reg_defaults = ARRAY_SIZE(rk817_render_stop_reg_defaults),
};

static const struct reg_default rk817_capture_start_reg_defaults[] = {
    { RK817_CODEC_DTOP_DIGEN_CLKE, 0xff },
    { RK817_CODEC_AADC_CFG0, 0x08 },
    { RK817_CODEC_DADC_SR_ACL0, 0x02 },
    { RK817_CODEC_AMIC_CFG0, 0x8f },
    { RK817_CODEC_DMIC_PGA_GAIN, 0x99 },
    { RK817_CODEC_ADAC_CFG1, 0x0f },
    { RK817_CODEC_DDAC_POPD_DACST, 0x04 },
    { RK817_CODEC_DDAC_MUTE_MIXCTL, 0x00 },
    { RK817_CODEC_DI2S_TXCR3_TXCMD, 0x88 },
};

static const struct regmap_config rk817_capture_start_regmap_config = {
    .reg_defaults = rk817_capture_start_reg_defaults,
    .num_reg_defaults = ARRAY_SIZE(rk817_capture_start_reg_defaults),
};

static const struct reg_default rk817_capture_stop_reg_defaults[] = {
    { RK817_CODEC_DTOP_DIGEN_CLKE, 0x0f },
    { RK817_CODEC_AADC_CFG0, 0xc8 },
    { RK817_CODEC_DADC_SR_ACL0, 0x00 },
    { RK817_CODEC_DDAC_POPD_DACST, 0x06 },
};

static const struct regmap_config rk817_capture_stop_regmap_config = {
    .reg_defaults = rk817_capture_stop_reg_defaults,
    .num_reg_defaults = ARRAY_SIZE(rk817_capture_stop_reg_defaults),
};

unsigned int g_cuurentcmd = AUDIO_DRV_PCM_IOCTL_BUTT;

int32_t Rk809DeviceRegRead(uint32_t reg, uint32_t *val)
{
    struct Rk809ChipData *chip = GetCodecDevice();
    if (chip == NULL) {
        AUDIO_DRIVER_LOG_ERR("get codec device failed.");
        return HDF_FAILURE;
    }
    if (regmap_read(chip->regmap, reg, val)) {
        AUDIO_DRIVER_LOG_ERR("read register fail: [%04x]", reg);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t Rk809DeviceRegWrite(uint32_t reg, uint32_t value)
{
    struct Rk809ChipData *chip = GetCodecDevice();
    if (chip == NULL) {
        AUDIO_DRIVER_LOG_ERR("get codec device failed.");
        return HDF_FAILURE;
    }
    if (regmap_write(chip->regmap, reg, value)) {
        AUDIO_DRIVER_LOG_ERR("write register fail: [%04x] = %04x", reg, value);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t Rk809DeviceRegUpdatebits(uint32_t reg, uint32_t mask, uint32_t value)
{
    struct Rk809ChipData *chip = GetCodecDevice();
    if (chip == NULL) {
        AUDIO_DRIVER_LOG_ERR("get codec device failed.");
        return HDF_FAILURE;
    }
    if (regmap_update_bits(chip->regmap, reg, mask, value)) {
        AUDIO_DRIVER_LOG_ERR("update register bits fail: [%04x] = %04x", reg, value);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

// Update contrl reg bits value
int32_t Rk809RegBitsUpdate(struct AudioMixerControl regAttr)
{
    int32_t ret;
    bool updaterreg = false;

    if (regAttr.invert) {
        regAttr.value = regAttr.max - regAttr.value;
    }
    regAttr.value = regAttr.value << regAttr.shift;
    ret = Rk809DeviceRegUpdatebits(regAttr.reg, regAttr.mask, regAttr.value);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("Rk809DeviceRegUpdatebits failed.");
        return HDF_FAILURE;
    }
    regAttr.value = regAttr.value << regAttr.rshift;
    updaterreg = (regAttr.reg != regAttr.rreg) || (regAttr.shift != regAttr.rshift);
    if (updaterreg) {
        ret = Rk809DeviceRegUpdatebits(regAttr.rreg, regAttr.mask, regAttr.value);
        if (ret != HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("Rk809DeviceRegUpdatebits failed.");
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

int32_t RK809RegBitsUpdateValue(struct AudioMixerControl *regAttr, Update_Dest dest, uint32_t value)
{
    int32_t ret;

    if (regAttr->invert) {
        value = regAttr->max - value;
    }
    if (dest == UPDATE_LREG) {
        value = value << regAttr->shift;
        ret = Rk809DeviceRegUpdatebits(regAttr->reg, regAttr->mask, value);
    } else if (dest == UPDATE_RREG) {
        value = value << regAttr->rshift;
        ret = Rk809DeviceRegUpdatebits(regAttr->rreg, regAttr->mask, value);
    } else {
        ret = HDF_FAILURE;
    }

    return ret;
}

int32_t RK809RegDefaultInit(struct AudioRegCfgGroupNode **regCfgGroup)
{
    int32_t i;
    struct AudioAddrConfig *regAttr = NULL;
    
    if (regCfgGroup == NULL || regCfgGroup[AUDIO_INIT_GROUP] == NULL ||
        regCfgGroup[AUDIO_INIT_GROUP]->addrCfgItem == NULL || regCfgGroup[AUDIO_INIT_GROUP]->itemNum <= 0) {
        AUDIO_DEVICE_LOG_ERR("input invalid parameter.");
        return HDF_ERR_INVALID_PARAM;
    }
    regAttr = regCfgGroup[AUDIO_INIT_GROUP]->addrCfgItem;

    for (i = 0; i < regCfgGroup[AUDIO_INIT_GROUP]->itemNum; i++) {
        Rk809DeviceRegWrite(regAttr[i].addr, regAttr[i].value);
    }

    return HDF_SUCCESS;
}

static const RK809SampleRateTimes RK809GetSRT(const unsigned int rate)
{
    switch (rate) {
        case AUDIO_SAMPLE_RATE_8000:
            return RK809_SRT_00;
        case AUDIO_SAMPLE_RATE_16000:
            return RK809_SRT_01;
        case AUDIO_SAMPLE_RATE_32000:
        case AUDIO_SAMPLE_RATE_44100:
        case AUDIO_SAMPLE_RATE_48000:
            return RK809_SRT_02;
        case AUDIO_SAMPLE_RATE_96000:
            return RK809_SRT_03;
        default:
            AUDIO_DEVICE_LOG_DEBUG("unsupport samplerate %d\n", rate);
            return RK809_SRT_02;
    }
}

static const RK809PLLInputCLKPreDIV RK809GetPremode(const unsigned int rate)
{
    switch (rate) {
        case AUDIO_SAMPLE_RATE_8000:
            return RK809_PREMODE_1;
        case AUDIO_SAMPLE_RATE_16000:
            return RK809_PREMODE_2;
        case AUDIO_SAMPLE_RATE_32000:
        case AUDIO_SAMPLE_RATE_44100:
        case AUDIO_SAMPLE_RATE_48000:
            return RK809_PREMODE_3;
        case AUDIO_SAMPLE_RATE_96000:
            return RK809_PREMODE_4;
        default:
            AUDIO_DEVICE_LOG_DEBUG("unsupport samplerate %d\n", rate);
            return RK809_PREMODE_3;
    }
}

static const RK809_VDW RK809GetI2SDataWidth(const unsigned int bitWidth)
{
    switch (bitWidth) {
        case BIT_WIDTH16:
            return RK809_VDW_16BITS;
        case BIT_WIDTH24:
        case BIT_WIDTH32:
            return RK809_VDW_24BITS;
        default:
            AUDIO_DEVICE_LOG_ERR("unsupport sample bit width %d.\n", bitWidth);
            return RK809_VDW_16BITS;
    }
}

int32_t RK809UpdateRenderParams(struct AudioRegCfgGroupNode **regCfgGroup,
    struct RK809DaiParamsVal codecDaiParamsVal)
{
    int32_t ret;
    struct AudioMixerControl *regAttr = NULL;
    int32_t itemNum;
    int16_t i = 0;

    ret = (regCfgGroup == NULL
        || regCfgGroup[AUDIO_DAI_PATAM_GROUP] == NULL
        || regCfgGroup[AUDIO_DAI_PATAM_GROUP]->regCfgItem == NULL);
    if (ret) {
        AUDIO_DEVICE_LOG_ERR("input invalid parameter.");
        return HDF_FAILURE;
    }

    itemNum = regCfgGroup[AUDIO_DAI_PATAM_GROUP]->itemNum;
    regAttr = regCfgGroup[AUDIO_DAI_PATAM_GROUP]->regCfgItem;

    for (i = 0; i < itemNum; i++) {
        if (regAttr[i].reg == RK817_CODEC_APLL_CFG3) {
            regAttr[i].value = RK809GetPremode(codecDaiParamsVal.frequencyVal);
            ret = Rk809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("Rk809RegBitsUpdate failed.");
                return HDF_FAILURE;
            }
        } else if (regAttr[i].reg == RK817_CODEC_DDAC_SR_LMT0) {
            regAttr[i].value = RK809GetSRT(codecDaiParamsVal.frequencyVal);
            Rk809DeviceRegUpdatebits(RK817_CODEC_DTOP_DIGEN_CLKE,
                DAC_DIG_CLK_EN, 0x00); // disenable DAC digital clk
            ret = Rk809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("Rk809RegBitsUpdate failed.");
                return HDF_FAILURE;
            }
            Rk809DeviceRegUpdatebits(RK817_CODEC_DTOP_DIGEN_CLKE, DAC_DIG_CLK_EN, DAC_DIG_CLK_EN);
        } else if (regAttr[i].reg == RK817_CODEC_DI2S_RXCR2) {
            regAttr[i].value = RK809GetI2SDataWidth(codecDaiParamsVal.DataWidthVal);
            ret = Rk809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("Rk809RegBitsUpdate failed.");
                return HDF_FAILURE;
            }
        }
    }

    return HDF_SUCCESS;
}

int32_t RK809UpdateCaptureParams(struct AudioRegCfgGroupNode **regCfgGroup,
    struct RK809DaiParamsVal codecDaiParamsVal)
{
    int32_t ret;
    struct AudioMixerControl *regAttr = NULL;
    int32_t itemNum;
    int16_t i = 0;

    ret = (regCfgGroup == NULL
        || regCfgGroup[AUDIO_DAI_PATAM_GROUP] == NULL
        || regCfgGroup[AUDIO_DAI_PATAM_GROUP]->regCfgItem == NULL);
    if (ret) {
        AUDIO_DEVICE_LOG_ERR("input invalid parameter.");
        return HDF_FAILURE;
    }

    itemNum = regCfgGroup[AUDIO_DAI_PATAM_GROUP]->itemNum;
    regAttr = regCfgGroup[AUDIO_DAI_PATAM_GROUP]->regCfgItem;

    for (i = 0; i < itemNum; i++) {
        if (regAttr[i].reg == RK817_CODEC_APLL_CFG3) {
            regAttr[i].value = RK809GetPremode(codecDaiParamsVal.frequencyVal);
            ret = Rk809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("Rk809RegBitsUpdate failed.");
                return HDF_FAILURE;
            }
        } else if (regAttr[i].reg == RK817_CODEC_DADC_SR_ACL0) {
            regAttr[i].value = RK809GetSRT(codecDaiParamsVal.frequencyVal);
            Rk809DeviceRegUpdatebits(RK817_CODEC_DTOP_DIGEN_CLKE, ADC_DIG_CLK_EN, 0x00);
            ret = Rk809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("Rk809RegBitsUpdate failed.");
                return HDF_FAILURE;
            }
            Rk809DeviceRegUpdatebits(RK817_CODEC_DTOP_DIGEN_CLKE, ADC_DIG_CLK_EN, ADC_DIG_CLK_EN);
        } else if (regAttr[i].reg == RK817_CODEC_DI2S_TXCR2) {
            regAttr[i].value = RK809GetI2SDataWidth(codecDaiParamsVal.DataWidthVal);
            ret = Rk809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("Rk809RegBitsUpdate failed.");
                return HDF_FAILURE;
            }
        }
    }

    return HDF_SUCCESS;
}

int32_t RK809DaiParamsUpdate(struct AudioRegCfgGroupNode **regCfgGroup,
    struct RK809DaiParamsVal codecDaiParamsVal)
{
    int32_t ret;

    ret = (regCfgGroup == NULL
        || regCfgGroup[AUDIO_DAI_PATAM_GROUP] == NULL
        || regCfgGroup[AUDIO_DAI_PATAM_GROUP]->regCfgItem == NULL);
    if (ret) {
        AUDIO_DEVICE_LOG_ERR("input invalid parameter.");
        return HDF_FAILURE;
    }

    if (g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_RENDER_START
        || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_RENDER_PAUSE
        || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_RENDER_RESUME
        || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_RENDER_STOP) {
        ret = RK809UpdateRenderParams(regCfgGroup, codecDaiParamsVal);
        if (ret != HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("RK809UpdateRenderParams failed.");
            return HDF_FAILURE;
        }
    } else if (g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_CAPTURE_START
        || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_CAPTURE_PAUSE
        || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_CAPTURE_RESUME
        || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_CAPTURE_STOP) {
        ret = RK809UpdateCaptureParams(regCfgGroup, codecDaiParamsVal);
        if (ret != HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("RK809UpdateCaptureParams failed.");
            return HDF_FAILURE;
        }
    }
    return HDF_SUCCESS;
}

static int32_t RK809WorkStatusEnable(struct AudioRegCfgGroupNode **regCfgGroup)
{
    int ret;
    uint8_t i;
    struct AudioMixerControl *daiStartupParamsRegCfgItem = NULL;
    uint8_t daiStartupParamsRegCfgItemCount;

    ret = (regCfgGroup == NULL || regCfgGroup[AUDIO_DAI_STARTUP_PATAM_GROUP] == NULL
        || regCfgGroup[AUDIO_DAI_STARTUP_PATAM_GROUP]->regCfgItem == NULL);
    if (ret) {
        AUDIO_DEVICE_LOG_ERR("input invalid parameter.");
        return HDF_FAILURE;
    }

    daiStartupParamsRegCfgItem =
        regCfgGroup[AUDIO_DAI_STARTUP_PATAM_GROUP]->regCfgItem;
    daiStartupParamsRegCfgItemCount =
        regCfgGroup[AUDIO_DAI_STARTUP_PATAM_GROUP]->itemNum;
    for (i = 0; i < daiStartupParamsRegCfgItemCount; i++) {
        ret = Rk809RegBitsUpdate(daiStartupParamsRegCfgItem[i]);
        if (ret != HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("Rk809RegBitsUpdate fail.");
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

int32_t RK809CodecReadReg(unsigned long virtualAddress, uint32_t reg, uint32_t *val)
{
    if (val == NULL) {
        AUDIO_DRIVER_LOG_ERR("param val is null.");
        return HDF_FAILURE;
    }
    if (Rk809DeviceRegRead(reg, val)) {
        AUDIO_DRIVER_LOG_ERR("read register fail: [%04x]", reg);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t Rk809CodecWriteReg(unsigned long virtualAddress, uint32_t reg, uint32_t value)
{
    if (Rk809DeviceRegWrite(reg, value)) {
        AUDIO_DRIVER_LOG_ERR("write register fail: [%04x] = %04x", reg, value);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t RK809GetCtrlOps(const struct AudioKcontrol *kcontrol, struct AudioCtrlElemValue *elemValue)
{
    uint32_t curValue;
    uint32_t rcurValue;
    struct AudioMixerControl *mixerCtrl = NULL;

    if (kcontrol == NULL || kcontrol->privateValue <= 0 || elemValue == NULL) {
        AUDIO_DEVICE_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);
    if (mixerCtrl == NULL) {
        AUDIO_DEVICE_LOG_ERR("mixerCtrl is NULL.");
        return HDF_FAILURE;
    }
    Rk809DeviceRegRead(mixerCtrl->reg, &curValue);
    Rk809DeviceRegRead(mixerCtrl->rreg, &rcurValue);
    if (AudioGetCtrlOpsReg(elemValue, mixerCtrl, curValue) != HDF_SUCCESS ||
        AudioGetCtrlOpsRReg(elemValue, mixerCtrl, rcurValue) != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("Audio codec get kcontrol reg and rreg failed.");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t RK809SetCtrlOps(const struct AudioKcontrol *kcontrol, const struct AudioCtrlElemValue *elemValue)
{
    uint32_t value;
    uint32_t rvalue;
    bool updateRReg = false;
    struct AudioMixerControl *mixerCtrl = NULL;
    if (kcontrol == NULL || (kcontrol->privateValue <= 0) || elemValue == NULL) {
        AUDIO_DEVICE_LOG_ERR("Audio input param is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    mixerCtrl = (struct AudioMixerControl *)((volatile uintptr_t)kcontrol->privateValue);
    if (AudioSetCtrlOpsReg(kcontrol, elemValue, mixerCtrl, &value) != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioSetCtrlOpsReg is failed.");
        return HDF_ERR_INVALID_OBJECT;
    }
    RK809RegBitsUpdateValue(mixerCtrl, UPDATE_LREG, value);
    if (AudioSetCtrlOpsRReg(elemValue, mixerCtrl, &rvalue, &updateRReg) != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioSetCtrlOpsRReg is failed.");
        return HDF_ERR_INVALID_OBJECT;
    }
    if (updateRReg) {
        RK809RegBitsUpdateValue(mixerCtrl, UPDATE_RREG, rvalue);
    }

    return HDF_SUCCESS;
}

int32_t Rk809DeviceInit(struct AudioCard *audioCard, const struct CodecDevice *device)
{
    int32_t ret;

    if (audioCard == NULL || device == NULL || device->devData == NULL ||
        device->devData->sapmComponents == NULL || device->devData->controls == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }

    if (CodecSetCtlFunc(device->devData, RK809GetCtrlOps, RK809SetCtrlOps) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("AudioCodecSetCtlFunc failed.");
        return HDF_FAILURE;
    }

    ret = RK809RegDefaultInit(device->devData->regCfgGroup);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("RK809RegDefaultInit failed.");
        return HDF_FAILURE;
    }

    if (AudioAddControls(audioCard, device->devData->controls, device->devData->numControls) != HDF_SUCCESS) {
        AUDIO_DRIVER_LOG_ERR("add controls failed.");
        return HDF_FAILURE;
    }

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t Rk809DaiDeviceInit(struct AudioCard *card, const struct DaiDevice *device)
{
    if (device == NULL || device->devDaiName == NULL) {
        AUDIO_DEVICE_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }
    (void)card;
    AUDIO_DEVICE_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t Rk809DaiStartup(const struct AudioCard *card, const struct DaiDevice *device)
{
    int ret;
    (void)card;
    (void)device;

    ret = RK809WorkStatusEnable(device->devData->regCfgGroup);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("RK809WorkStatusEnable failed.");
        return HDF_FAILURE;
    }

    AUDIO_DRIVER_LOG_DEBUG("success.");
    return HDF_SUCCESS;
}

int32_t Rk809DaiHwParams(const struct AudioCard *card, const struct AudioPcmHwParams *param)
{
    int32_t ret;
    unsigned int bitWidth;
    struct RK809DaiParamsVal codecDaiParamsVal;
    (void)card;

    if (param == NULL || param->cardServiceName == NULL || card == NULL ||
        card->rtd == NULL || card->rtd->codecDai == NULL || card->rtd->codecDai->devData == NULL ||
        card->rtd->codecDai->devData->regCfgGroup == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }

    ret = AudioFormatToBitWidth(param->format, &bitWidth);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    codecDaiParamsVal.frequencyVal = param->rate;
    codecDaiParamsVal.DataWidthVal = bitWidth;

    ret =  RK809DaiParamsUpdate(card->rtd->codecDai->devData->regCfgGroup, codecDaiParamsVal);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("RK809DaiParamsUpdate failed.");
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t RK809DeviceRegConfig(const struct regmap_config reg_config)
{
    int32_t num_reg_defaults = reg_config.num_reg_defaults;
    int32_t index;

    for (index = 0; index < num_reg_defaults; index++) {
        Rk809DeviceRegWrite(reg_config.reg_defaults[index].reg, reg_config.reg_defaults[index].def);
    }

    return HDF_SUCCESS;
}


/* normal scene */
int32_t Rk809NormalTrigger(const struct AudioCard *card, int cmd, const struct DaiDevice *device)
{
    g_cuurentcmd = cmd;
    switch (cmd) {
        case AUDIO_DRV_PCM_IOCTL_RENDER_START:
        case AUDIO_DRV_PCM_IOCTL_RENDER_RESUME:
            RK809DeviceRegConfig(rk817_render_start_regmap_config);
            break;
        
        case AUDIO_DRV_PCM_IOCTL_RENDER_STOP:
        case AUDIO_DRV_PCM_IOCTL_RENDER_PAUSE:
            RK809DeviceRegConfig(rk817_render_stop_regmap_config);
            break;

        case AUDIO_DRV_PCM_IOCTL_CAPTURE_START:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_RESUME:
            RK809DeviceRegConfig(rk817_capture_start_regmap_config);
            break;

        case AUDIO_DRV_PCM_IOCTL_CAPTURE_STOP:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_PAUSE:
            RK809DeviceRegConfig(rk817_capture_stop_regmap_config);
            break;

        default:
            break;
    }

    return HDF_SUCCESS;
}
