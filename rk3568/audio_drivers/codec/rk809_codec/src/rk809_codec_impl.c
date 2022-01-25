/*
   Copyright (c) [2022] [huangji@nj.iscas.ac.cn]
   [Software Name] is licensed under Mulan PSL v2.
   You can use this software according to the terms and conditions of the Mulan PSL v2. 
   You may obtain a copy of Mulan PSL v2 at:
               http://license.coscl.org.cn/MulanPSL2 
   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.  
   See the Mulan PSL v2 for more details.  
*/

#include "rk809_codec_impl.h"
#include "audio_accessory_base.h"
#include "gpio_if.h"
#include <linux/regmap.h>
#include "linux/of_gpio.h"
//#include <audio-sipc.h>
#include "audio_driver_log.h"
#include "audio_stream_dispatch.h"
#include "rk817_codec.h"

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
struct Rk809TransferData g_TransferData;
extern struct Rk809ChipData *g_chip;

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
    if (regmap_read(g_chip->regmap, reg, val)) {
        AUDIO_DRIVER_LOG_ERR("read register fail: [%04x]", reg);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t Rk809DeviceRegWrite(uint32_t reg, uint32_t value) {
    if (regmap_write(g_chip->regmap, reg, value)) {
        AUDIO_DRIVER_LOG_ERR("write register fail: [%04x] = %04x", reg, value);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t Rk809DeviceRegUpdatebits(uint32_t reg, uint32_t mask, uint32_t value) {
    if (regmap_update_bits(g_chip->regmap, reg, mask, value)) {
        AUDIO_DRIVER_LOG_ERR("update register bits fail: [%04x] = %04x", reg, value);
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

// Update contrl reg bits value
int32_t RK809RegBitsUpdate(struct AudioMixerControl regAttr)
{
    int32_t ret;

    if (regAttr.invert) {
        regAttr.value = regAttr.max - regAttr.value;
    }
    Rk809DeviceRegUpdatebits(regAttr.reg, regAttr.mask, regAttr.value);

    return HDF_SUCCESS;
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
    int32_t itemNum = regCfgGroup[AUDIO_DAI_PATAM_GROUP]->itemNum;
    int16_t i = 0;

    ret = (regCfgGroup == NULL
        || regCfgGroup[AUDIO_DAI_PATAM_GROUP] == NULL
        || regCfgGroup[AUDIO_DAI_PATAM_GROUP]->regCfgItem == NULL);
    if (ret) {
        AUDIO_DEVICE_LOG_ERR("input invalid parameter.");
        return HDF_FAILURE;
    }

    regAttr = regCfgGroup[AUDIO_DAI_PATAM_GROUP]->regCfgItem;

    for (i = 0; i < itemNum; i++) {
        if (regAttr[i].reg == RK817_CODEC_APLL_CFG3) {
            regAttr[i].value = RK809GetPremode(codecDaiParamsVal.frequencyVal);
            ret = RK809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("RK809RegBitsUpdate failed.");
                return HDF_FAILURE;
            }
        }
        else if (regAttr[i].reg == RK817_CODEC_DDAC_SR_LMT0) {
            regAttr[i].value = RK809GetSRT(codecDaiParamsVal.frequencyVal);
            Rk809DeviceRegUpdatebits(RK817_CODEC_DTOP_DIGEN_CLKE,DAC_DIG_CLK_EN,0x00);
            ret = RK809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("RK809RegBitsUpdate failed.");
                return HDF_FAILURE;
            }
            Rk809DeviceRegUpdatebits(RK817_CODEC_DTOP_DIGEN_CLKE,DAC_DIG_CLK_EN,DAC_DIG_CLK_EN);
        }
        else if (regAttr[i].reg == RK817_CODEC_DI2S_RXCR2) {
            regAttr[i].value = RK809GetI2SDataWidth(codecDaiParamsVal.DataWidthVal);
            ret = RK809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("RK809RegBitsUpdate failed.");
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
    int32_t itemNum = regCfgGroup[AUDIO_DAI_PATAM_GROUP]->itemNum;
    int16_t i = 0;

    ret = (regCfgGroup == NULL
        || regCfgGroup[AUDIO_DAI_PATAM_GROUP] == NULL
        || regCfgGroup[AUDIO_DAI_PATAM_GROUP]->regCfgItem == NULL);
    if (ret) {
        AUDIO_DEVICE_LOG_ERR("input invalid parameter.");
        return HDF_FAILURE;
    }

    regAttr = regCfgGroup[AUDIO_DAI_PATAM_GROUP]->regCfgItem;

    for (i = 0; i < itemNum; i++) {
        if (regAttr[i].reg == RK817_CODEC_APLL_CFG3) {
            regAttr[i].value = RK809GetPremode(codecDaiParamsVal.frequencyVal);
            ret = RK809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("RK809RegBitsUpdate failed.");
                return HDF_FAILURE;
            }
        }
        else if (regAttr[i].reg == RK817_CODEC_DADC_SR_ACL0) {
            regAttr[i].value = RK809GetSRT(codecDaiParamsVal.frequencyVal);
            Rk809DeviceRegUpdatebits(RK817_CODEC_DTOP_DIGEN_CLKE,ADC_DIG_CLK_EN,0x00);
            ret = RK809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("RK809RegBitsUpdate failed.");
                return HDF_FAILURE;
            }
            Rk809DeviceRegUpdatebits(RK817_CODEC_DTOP_DIGEN_CLKE,ADC_DIG_CLK_EN,ADC_DIG_CLK_EN);
        }
        else if (regAttr[i].reg == RK817_CODEC_DI2S_TXCR2) {
            regAttr[i].value = RK809GetI2SDataWidth(codecDaiParamsVal.DataWidthVal);
            ret = RK809RegBitsUpdate(regAttr[i]);
            if (ret != HDF_SUCCESS) {
                AUDIO_DEVICE_LOG_ERR("RK809RegBitsUpdate failed.");
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

    if(g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_RENDER_START || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_RENDER_PAUSE
        || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_RENDER_RESUME || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_RENDER_STOP)
    {
        ret = RK809UpdateRenderParams(regCfgGroup,codecDaiParamsVal);
        if (ret != HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("RK809UpdateRenderParams failed.");
            return HDF_FAILURE;            
        }
    }
    else if(g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_CAPTURE_START || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_CAPTURE_PAUSE
        || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_CAPTURE_RESUME || g_cuurentcmd == AUDIO_DRV_PCM_IOCTL_CAPTURE_STOP)
    {
        ret = RK809UpdateCaptureParams(regCfgGroup,codecDaiParamsVal);
        if (ret != HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("RK809UpdateCaptureParams failed.");
            return HDF_FAILURE;            
        }
    }

    return HDF_SUCCESS;
}

static int32_t RK809WorkStatusEnable(void)
{
    int ret;
    uint8_t i;
    struct AudioMixerControl *daiStartupParamsRegCfgItem = NULL;
    uint8_t daiStartupParamsRegCfgItemCount;

    ret = (g_TransferData.RegCfgGroupNode == NULL
        || g_TransferData.RegCfgGroupNode[AUDIO_DAI_STARTUP_PATAM_GROUP] == NULL
        || g_TransferData.RegCfgGroupNode[AUDIO_DAI_STARTUP_PATAM_GROUP]->regCfgItem == NULL);
    if (ret) {
        AUDIO_DEVICE_LOG_ERR("g_audioRegCfgGroupNode[AUDIO_DAI_STARTUP_PATAM_GROUP] is NULL.");
        return HDF_FAILURE;
    }
    daiStartupParamsRegCfgItem =
        g_TransferData.RegCfgGroupNode[AUDIO_DAI_STARTUP_PATAM_GROUP]->regCfgItem;
    daiStartupParamsRegCfgItemCount =
        g_TransferData.RegCfgGroupNode[AUDIO_DAI_STARTUP_PATAM_GROUP]->itemNum;
    for (i = 0; i < daiStartupParamsRegCfgItemCount; i++) {
        // ret = AccessoryRegBitsUpdate(daiStartupParamsRegCfgItem[i]);
        ret = RK809RegBitsUpdate(daiStartupParamsRegCfgItem[i]);
        if (ret != HDF_SUCCESS) {
            AUDIO_DEVICE_LOG_ERR("AccessoryRegBitsUpdate fail.");
            return HDF_FAILURE;
        }
    }

    return HDF_SUCCESS;
}

// Init Function
uint16_t g_rk809i2cDevAddr, g_rk809i2cBusNumber;
struct AudioRegCfgGroupNode **g_rk809audioRegCfgGroupNode = NULL;
struct AudioKcontrol *g_rk809audioControls = NULL;

int32_t RK809CodecReadReg(unsigned long virtualAddress,uint32_t reg, uint32_t *val)
{
    if (val == NULL) {
        AUDIO_DRIVER_LOG_ERR("param val is null.");
        return HDF_FAILURE;
    }
    if (Rk809DeviceRegRead(reg, val)) {
        AUDIO_DRIVER_LOG_ERR("read register fail: [%04x]", reg);
        return HDF_FAILURE;
    }
    ADM_LOG_DEBUG("read reg 0x[%02x] = 0x[%02x]",reg,*val);
    return HDF_SUCCESS;
}

int32_t RK809CodecWriteReg(unsigned long virtualAddress,uint32_t reg, uint32_t value)
{
    if (Rk809DeviceRegWrite(reg, value)) {
        AUDIO_DRIVER_LOG_ERR("write register fail: [%04x] = %04x", reg, value);
        return HDF_FAILURE;
    }    
    ADM_LOG_DEBUG("write reg 0x[%02x] = 0x[%02x]",reg,value);
    return HDF_SUCCESS;
}

int32_t Rk809DeviceCfgGet(struct CodecData *codecData,
    struct Rk809TransferData *TransferData)
{
    int32_t ret;
    int32_t index;
    int32_t audioCfgCtrlCount;

    ret = (codecData == NULL || codecData->regConfig == NULL || TransferData == NULL);
    if (ret) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }
    g_rk809i2cDevAddr = TransferData->i2cDevAddr;
    g_rk809i2cBusNumber = TransferData->i2cBusNumber;
    g_rk809audioRegCfgGroupNode = codecData->regConfig->audioRegParams;
    ret = (g_rk809audioRegCfgGroupNode[AUDIO_CTRL_CFG_GROUP] == NULL ||
           g_rk809audioRegCfgGroupNode[AUDIO_CTRL_CFG_GROUP]->ctrlCfgItem == NULL ||
           g_rk809audioRegCfgGroupNode[AUDIO_CTRL_PATAM_GROUP] == NULL ||
           g_rk809audioRegCfgGroupNode[AUDIO_CTRL_PATAM_GROUP]->regCfgItem == NULL);
    if (ret) {
        AUDIO_DRIVER_LOG_ERR("parsing params is NULL.");
        return HDF_FAILURE;
    }
    struct AudioControlConfig *ctlcfgItem = g_rk809audioRegCfgGroupNode[AUDIO_CTRL_CFG_GROUP]->ctrlCfgItem;
    audioCfgCtrlCount = g_rk809audioRegCfgGroupNode[AUDIO_CTRL_CFG_GROUP]->itemNum;
    g_rk809audioControls = (struct AudioKcontrol *)OsalMemCalloc(audioCfgCtrlCount * sizeof(struct AudioKcontrol));
    TransferData->RegCfgGroupNode = g_rk809audioRegCfgGroupNode;
    TransferData->CfgCtrlCount = audioCfgCtrlCount;
    TransferData->Controls = g_rk809audioControls;
    for (index = 0; index < audioCfgCtrlCount; index++) {
        g_rk809audioControls[index].iface = ctlcfgItem[index].iface;
        g_rk809audioControls[index].name  = g_audioControlsList[ctlcfgItem[index].arrayIndex];
        g_rk809audioControls[index].Info  = AudioInfoCtrlOps;
        g_rk809audioControls[index].privateValue =
            (unsigned long)&g_rk809audioRegCfgGroupNode[AUDIO_CTRL_PATAM_GROUP]->regCfgItem[index];
        g_rk809audioControls[index].Get = AudioCodecGetCtrlOps;
        g_rk809audioControls[index].Set = AudioCodecSetCtrlOps;
    }

    return HDF_SUCCESS;
}


int32_t Rk809DeviceInit(struct AudioCard *audioCard, const struct CodecDevice *device)
{
    int32_t ret;

    if ((audioCard == NULL) || (device == NULL)) {
        AUDIO_DEVICE_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_OBJECT;
    }
    g_TransferData.i2cDevAddr = RK809_I2C_DEV_ADDR;
    g_TransferData.i2cBusNumber = RK809_I2C_BUS_NUMBER;
    ret = Rk809DeviceCfgGet(device->devData, &g_TransferData);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AccessoryDeviceCfgGet failed.");
        return HDF_FAILURE;
    }

    ret = RK809RegDefaultInit(g_TransferData.RegCfgGroupNode);
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

    ret = RK809WorkStatusEnable();
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

    ret = AudioFramatToBitWidth(param->format, &bitWidth);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    codecDaiParamsVal.frequencyVal = param->rate;
    codecDaiParamsVal.DataWidthVal = bitWidth;

    ret =  RK809DaiParamsUpdate(card->rtd->codecDai->devData->regCfgGroup,codecDaiParamsVal);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("CodecDaiParamsUpdate failed.");
        return HDF_FAILURE;
    }     

    return HDF_SUCCESS;
}

int32_t RK809DeviceRegConfig(const struct regmap_config reg_config) 
{
    int32_t num_reg_defaults = reg_config.num_reg_defaults;
    int32_t index;

    for (index = 0; index < num_reg_defaults; index++) {
        Rk809DeviceRegWrite(reg_config.reg_defaults[index].reg,reg_config.reg_defaults[index].def);
    }

    return HDF_SUCCESS;
}


/* normal scene */
int32_t RK809NormalTrigger(const struct AudioCard *card, int cmd, const struct DaiDevice *device)
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