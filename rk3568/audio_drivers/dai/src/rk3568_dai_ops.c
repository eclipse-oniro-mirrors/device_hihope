/*
 * Copyright (C) 2022 HiHope Open Source Organization .
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */
#include <sound/pcm_params.h>
#include <sound/dmaengine_pcm.h>
#include <linux/module.h>
#include <linux/mfd/syscon.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <linux/clk/rockchip.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/reset.h>
#include <linux/spinlock.h>

#include "audio_host.h"
#include "audio_control.h"
#include "audio_dai_if.h"
#include "audio_dai_base.h"
#include "audio_driver_log.h"
#include "audio_stream_dispatch.h"
#include "osal_io.h"
#include "rk3568_dai_linux.h"
#include "rk3568_audio_common.h"
#include "audio_platform_base.h"
#include "rk3568_dai_ops.h"

#define HDF_LOG_TAG rk3568_dai_ops

void *g_regDaiBase = NULL;

int32_t Rk3568DeviceReadReg(unsigned long regBase, uint32_t reg, uint32_t *val)
{
    AUDIO_DEVICE_LOG_DEBUG("entry");
    struct device_node *dmaOfNode = of_find_node_by_path("/i2s@fe410000");
    if (dmaOfNode == NULL) {
        AUDIO_DEVICE_LOG_ERR("of_node is NULL.");
    }
    struct platform_device *platformdev = of_find_device_by_node(dmaOfNode);
    struct rk3568_i2s_tdm_dev *i2s_tdm = dev_get_drvdata(&platformdev->dev);
    
    (void)regBase;
    if (regmap_read(i2s_tdm->regmap, reg, val)) {
        AUDIO_DEVICE_LOG_ERR("read register fail: [%04x]", reg);
        return HDF_FAILURE;
    }
    AUDIO_DEVICE_LOG_DEBUG("i2s reg: 0x%x = 0x%x ", reg, val);
    AUDIO_DEVICE_LOG_DEBUG("success");
    return HDF_SUCCESS;
}

int32_t Rk3568DeviceWriteReg(unsigned long regBase, uint32_t reg, uint32_t value)
{
    AUDIO_DEVICE_LOG_DEBUG("entry");
    struct device_node *dmaOfNode = of_find_node_by_path("/i2s@fe410000");
    if (dmaOfNode == NULL) {
        AUDIO_DEVICE_LOG_ERR("of_node is NULL.");
    }
    struct platform_device *platformdev = of_find_device_by_node(dmaOfNode);
    struct rk3568_i2s_tdm_dev *i2s_tdm = dev_get_drvdata(&platformdev->dev);
    if (regmap_write(i2s_tdm->regmap, reg, value)) {
        AUDIO_DEVICE_LOG_ERR("write register fail: [%04x] = %04x", reg, value);
        return HDF_FAILURE;
    }
    (void)regBase;
    AUDIO_DEVICE_LOG_DEBUG("i2s regBase+ reg: 0x%x value: 0x%x .", reg, value);
    AUDIO_DEVICE_LOG_DEBUG("success");
    return HDF_SUCCESS;
}


int32_t Rk3568DaiDeviceInit(struct AudioCard *card, const struct DaiDevice *dai)
{
    AUDIO_DEVICE_LOG_ERR("entry");
    int32_t ret;
    if (dai == NULL || dai->device == NULL || dai->devDaiName == NULL) {
        AUDIO_DEVICE_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_PARAM;
    }
    AUDIO_DEVICE_LOG_DEBUG("codec dai device name: %s", dai->devDaiName);

    if (dai == NULL || dai->devData == NULL) {
        AUDIO_DEVICE_LOG_ERR("dai host is NULL.");
        return HDF_FAILURE;
    }
    struct DaiData *data = dai->devData;

    if (DaiSetConfigInfo(data) != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("set config info fail.");
        return HDF_FAILURE;
    }

    ret = AudioAddControls(card, data->controls, data->numControls);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("add controls failed.");
        return HDF_FAILURE;
    }

    if (g_regDaiBase == NULL) {
        g_regDaiBase = OsalIoRemap(data->regConfig->audioIdInfo.chipIdRegister,
            data->regConfig->audioIdInfo.chipIdSize);
        if (g_regDaiBase == NULL) {
            AUDIO_DEVICE_LOG_ERR("OsalIoRemap fail.");
            return HDF_FAILURE;
        }
    }
    data->regVirtualAddr = (uintptr_t)g_regDaiBase;

    if (dai->devData->daiInitFlag == true) {
        AUDIO_DRIVER_LOG_DEBUG("dai init complete!");
        return HDF_SUCCESS;
    }

    dai->devData->daiInitFlag = true;
    AUDIO_DEVICE_LOG_DEBUG("success");
    return HDF_SUCCESS;
}

int32_t Rk3568DaiStartup(const struct AudioCard *card, const struct DaiDevice *dai)
{
    (void)card;
    (void)dai;
    return HDF_SUCCESS;
}

int RK3568SetI2sFomartVal(const struct AudioPcmHwParams *param)
{
    AUDIO_DEVICE_LOG_DEBUG("entry");
    int32_t val;
    AUDIO_DEVICE_LOG_DEBUG("param->format = %d\r\n", param->format);
    switch (param->format) {
        case AUDIO_FORMAT_PCM_8_BIT:
            val |= I2S_TXCR_VDW(8); // 8-bit
            break;
        case AUDIO_FORMAT_PCM_16_BIT:
            val |= I2S_TXCR_VDW(16); // 16-bit
            break;
        case AUDIO_FORMAT_PCM_24_BIT:
            val |= I2S_TXCR_VDW(24); // 24-bit
            break;
        case AUDIO_FORMAT_PCM_32_BIT:
            val |= I2S_TXCR_VDW(32); // 32-bit
            break;
        default:
            return -1;
    }
    AUDIO_DEVICE_LOG_DEBUG("params_format(params) = %d\r\n", val);
    AUDIO_DEVICE_LOG_DEBUG("sucess");
    return val;
}

int32_t RK3568SetI2sChannels(struct rk3568_i2s_tdm_dev *i2s_tdm, const struct AudioPcmHwParams *param)
{
    AUDIO_DEVICE_LOG_DEBUG("entry");
    unsigned int reg_fmt, fmt;
    int32_t ret = 0;
    if (param->streamType == AUDIO_RENDER_STREAM) {
        reg_fmt = I2S_TXCR;
    } else {
        reg_fmt = I2S_RXCR;
    }
    
    regmap_read(i2s_tdm->regmap, reg_fmt, &fmt);
    AUDIO_DEVICE_LOG_DEBUG("fmt = %u", fmt);
    fmt &= I2S_TXCR_TFS_MASK;
    if (fmt == 0) { // I2S mode
        switch (param->channels) {
            case 8:               // channels is 8
                ret = I2S_CHN_8;
                break;
            case 6:               // channels is 6
                ret = I2S_CHN_6;
                break;
            case 4:               // channels is 4
                ret = I2S_CHN_4;
                break;
            case 2:               // channels is 2
                ret = I2S_CHN_2;
                break;
            default:
                ret = -EINVAL;
                break;
        }
    }
    AUDIO_DEVICE_LOG_DEBUG("success");
    return ret;
}

int32_t ConfigInfoSetToReg(struct rk3568_i2s_tdm_dev *i2s_tdm, const struct AudioPcmHwParams *param,
    unsigned int div_bclk, unsigned int div_lrck, int32_t fmt)
{
    AUDIO_DEVICE_LOG_DEBUG("entry");
    regmap_update_bits(i2s_tdm->regmap, I2S_CLKDIV,
        I2S_CLKDIV_TXM_MASK | I2S_CLKDIV_RXM_MASK,
        I2S_CLKDIV_TXM(div_bclk) | I2S_CLKDIV_RXM(div_bclk));
    regmap_update_bits(i2s_tdm->regmap, I2S_CKR,
        I2S_CKR_TSD_MASK | I2S_CKR_RSD_MASK,
        I2S_CKR_TSD(div_lrck) | I2S_CKR_RSD(div_lrck));

    if (param->streamType == AUDIO_RENDER_STREAM) {
        regmap_update_bits(i2s_tdm->regmap, I2S_TXCR,
            I2S_TXCR_VDW_MASK | I2S_TXCR_CSR_MASK,
            fmt);
    } else {
        regmap_update_bits(i2s_tdm->regmap, I2S_RXCR,
            I2S_RXCR_VDW_MASK | I2S_RXCR_CSR_MASK,
            fmt);
    }
    AUDIO_DEVICE_LOG_DEBUG("success");
    return HDF_SUCCESS;
}

// i2s_tdm->mclk_tx_freq ? i2s_tdm->mclk_rx_freq ?
int32_t RK3568I2sTdmSetMclk(struct rk3568_i2s_tdm_dev *i2s_tdm, struct clk **mclk, const struct AudioPcmHwParams *param)
{
    AUDIO_DEVICE_LOG_DEBUG("entry");
    int32_t ret = 0;
    unsigned int mclk_rate, bclk_rate, div_bclk, div_lrck;
    int32_t fmt = 0;
    int32_t channels = 0;
    AUDIO_DEVICE_LOG_DEBUG("i2s_tdm->mclk_tx = %p, i2s_tdm->mclk_tx_freq = %u", i2s_tdm->mclk_tx, i2s_tdm->mclk_tx_freq);
    ret = clk_set_rate(i2s_tdm->mclk_tx, i2s_tdm->mclk_tx_freq);
    if (ret) {
        AUDIO_DEVICE_LOG_ERR(" clk_set_rate ret = %d", ret);
        return ret;
    }

    ret = clk_set_rate(i2s_tdm->mclk_rx, i2s_tdm->mclk_rx_freq);
    if (ret) {
        AUDIO_DEVICE_LOG_ERR(" clk_set_rate ret = %d", ret);
        return ret;
    }
 
    /* mclk_rx is also ok. */
    mclk_rate = clk_get_rate(i2s_tdm->mclk_tx);
    bclk_rate = i2s_tdm->bclk_fs * param->rate;
    div_bclk = DIV_ROUND_CLOSEST(mclk_rate, bclk_rate);
    div_lrck = bclk_rate / param->rate;
    fmt = RK3568SetI2sFomartVal(param);
    if (fmt < 0) {
        AUDIO_DEVICE_LOG_ERR(" RK3568SetI2sFomartVal fmt = %d", fmt);
        return HDF_FAILURE;
    }
    AUDIO_DEVICE_LOG_DEBUG(" fmt = %d", fmt);
    channels = RK3568SetI2sChannels(i2s_tdm, param);
    if (channels < 0) {
        AUDIO_DEVICE_LOG_ERR(" RK3568SetI2sFomartVal channels = %d", channels);
        return HDF_FAILURE;
    }
    fmt |= channels;
    AUDIO_DEVICE_LOG_DEBUG(" fmt = %d", fmt);
    ret = ConfigInfoSetToReg(i2s_tdm, param, div_bclk, div_lrck, fmt);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR(" ConfigInfoSetToReg ret= %d", ret);
        return HDF_FAILURE;
    }
    
    AUDIO_DEVICE_LOG_DEBUG("success");
    return HDF_SUCCESS;
}
int32_t RK3568I2sTdmSetSysClk(struct rk3568_i2s_tdm_dev *i2s_tdm, const struct AudioPcmHwParams *param)
{
    /* Put set mclk rate into rockchip_i2s_tdm_set_mclk() */
    uint32_t sampleRate = param->rate;
    uint32_t mclk_parent_freq = 0;
    switch (sampleRate) {
        case AUDIO_DEVICE_SAMPLE_RATE_8000:
        case AUDIO_DEVICE_SAMPLE_RATE_16000:
        case AUDIO_DEVICE_SAMPLE_RATE_24000:
        case AUDIO_DEVICE_SAMPLE_RATE_32000:
        case AUDIO_DEVICE_SAMPLE_RATE_48000:
        case AUDIO_DEVICE_SAMPLE_RATE_64000:
        case AUDIO_DEVICE_SAMPLE_RATE_96000:
            mclk_parent_freq = i2s_tdm->bclk_fs * AUDIO_DEVICE_SAMPLE_RATE_192000;
            break;
        case AUDIO_DEVICE_SAMPLE_RATE_11025:
        case AUDIO_DEVICE_SAMPLE_RATE_22050:
        case AUDIO_DEVICE_SAMPLE_RATE_44100:
            mclk_parent_freq = i2s_tdm->bclk_fs * AUDIO_DEVICE_SAMPLE_RATE_176400;
            break;
        default:
            AUDIO_DEVICE_LOG_ERR("Invalid LRCK freq: %u Hz\n", sampleRate);
            return HDF_FAILURE;
    }
    i2s_tdm->mclk_tx_freq = mclk_parent_freq;
    i2s_tdm->mclk_rx_freq = mclk_parent_freq;
    
    return HDF_SUCCESS;
}

int32_t Rk3568DaiHwParams(const struct AudioCard *card, const struct AudioPcmHwParams *param)
{
    int ret;
    uint32_t bitWidth;
    struct clk *mclk;
    struct device_node    *dmaOfNode;
    struct platform_device *platformdev;
    AUDIO_DEVICE_LOG_DEBUG("entry");
    dmaOfNode = of_find_node_by_path("/i2s@fe410000");
    if (dmaOfNode == NULL) {
        AUDIO_DEVICE_LOG_ERR("of_node is NULL.");
    }
    platformdev = of_find_device_by_node(dmaOfNode);
    AUDIO_DEVICE_LOG_DEBUG("dmaOfNode->name %s", dmaOfNode->full_name);

    struct rk3568_i2s_tdm_dev *i2s_tdm = dev_get_drvdata(&platformdev->dev);
    AUDIO_DEVICE_LOG_DEBUG("i2s_tdm addr = %p", i2s_tdm);
    struct DaiData *data = DaiDataFromCard(card);
    if (data == NULL) {
        AUDIO_DEVICE_LOG_ERR("platformHost is nullptr.");
        return HDF_FAILURE;
    }

    if (param == NULL || param->cardServiceName == NULL) {
        AUDIO_DEVICE_LOG_ERR("input para is NULL.");
        return HDF_ERR_INVALID_PARAM;
    }

    data->pcmInfo.channels = param->channels;

    if (AudioFramatToBitWidth(param->format, &bitWidth) != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioFramatToBitWidth error");
        return HDF_FAILURE;
    }
    data->pcmInfo.bitWidth = bitWidth;
    data->pcmInfo.rate = param->rate;
    data->pcmInfo.streamType = param->streamType;

    ret = RK3568I2sTdmSetSysClk(i2s_tdm, param);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("RK3568I2sTdmSetSysClk error");
        return HDF_FAILURE;
    }
    ret = RK3568I2sTdmSetMclk(i2s_tdm, &mclk, param);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("RK3568I2sTdmSetMclk error");
        return HDF_FAILURE;
    }
    AUDIO_DEVICE_LOG_DEBUG("success");
    return HDF_SUCCESS;
}
static int GetTriggeredFlag(int cmd)
{
    int32_t triggerFlag = false;
    AUDIO_DEVICE_LOG_DEBUG("entry");
    switch (cmd) {
        case AUDIO_DRV_PCM_IOCTL_RENDER_START:
        case AUDIO_DRV_PCM_IOCTL_RENDER_RESUME:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_START:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_RESUME:
            triggerFlag = true;
            break;

        case AUDIO_DRV_PCM_IOCTL_RENDER_STOP:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_STOP:
        case AUDIO_DRV_PCM_IOCTL_RENDER_PAUSE:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_PAUSE:
            triggerFlag = false;
            break;

        default:
            triggerFlag = false;
    }
    AUDIO_DEVICE_LOG_DEBUG("success");
    return triggerFlag;
}


static int GetStreamType(int cmd)
{
    enum AudioStreamType streamType = AUDIO_CAPTURE_STREAM;
    AUDIO_DEVICE_LOG_DEBUG("entry");
    switch (cmd) {
        case AUDIO_DRV_PCM_IOCTL_RENDER_START:
        case AUDIO_DRV_PCM_IOCTL_RENDER_RESUME:
        case AUDIO_DRV_PCM_IOCTL_RENDER_STOP:
        case AUDIO_DRV_PCM_IOCTL_RENDER_PAUSE:
            streamType = AUDIO_RENDER_STREAM;
            break;
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_START:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_RESUME:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_STOP:
        case AUDIO_DRV_PCM_IOCTL_CAPTURE_PAUSE:
            streamType = AUDIO_CAPTURE_STREAM;
            break;

        default:
            break;
    }
    AUDIO_DEVICE_LOG_DEBUG("success");
    return streamType;
}

static void Rk3568TxAndRxSetReg(struct rk3568_i2s_tdm_dev *i2s_tdm,
    enum AudioStreamType streamType, int on)
{
    AUDIO_DEVICE_LOG_DEBUG("entry");
    unsigned int val = 0;
    int retry = 10; // retry 10 times
    
    if (on) {
        if (streamType == AUDIO_RENDER_STREAM) {
            clk_prepare_enable(i2s_tdm->mclk_tx);
            regmap_update_bits(i2s_tdm->regmap, I2S_DMACR,
                I2S_DMACR_TDE_ENABLE, I2S_DMACR_TDE_ENABLE);
        } else {
            clk_prepare_enable(i2s_tdm->mclk_rx);
            regmap_update_bits(i2s_tdm->regmap, I2S_DMACR,
                I2S_DMACR_RDE_ENABLE, I2S_DMACR_RDE_ENABLE);
            if (regmap_read(i2s_tdm->regmap, I2S_DMACR, &val)) {
                AUDIO_DEVICE_LOG_ERR("read register fail: [%04x]", I2S_DMACR);
                return ;
            }
        }

        if (atomic_inc_return(&i2s_tdm->refcount) == 1) {
            regmap_update_bits(i2s_tdm->regmap, I2S_XFER,
                I2S_XFER_TXS_START | I2S_XFER_RXS_START, I2S_XFER_TXS_START | I2S_XFER_RXS_START);
            if (regmap_read(i2s_tdm->regmap, I2S_XFER, &val)) {
                AUDIO_DEVICE_LOG_ERR("read register fail: [%04x]", I2S_XFER);
                return ;
            }
        }
    } else {
        if (streamType == AUDIO_RENDER_STREAM) {
            clk_disable_unprepare(i2s_tdm->mclk_tx);
            regmap_update_bits(i2s_tdm->regmap, I2S_DMACR,
                I2S_DMACR_TDE_ENABLE, I2S_DMACR_TDE_DISABLE);
        } else {
            clk_disable_unprepare(i2s_tdm->mclk_rx);
            regmap_update_bits(i2s_tdm->regmap, I2S_DMACR,
                I2S_DMACR_RDE_ENABLE, I2S_DMACR_RDE_DISABLE);
        }

        if (atomic_dec_and_test(&i2s_tdm->refcount)) {
            regmap_update_bits(i2s_tdm->regmap, I2S_XFER,
                I2S_XFER_TXS_START | I2S_XFER_RXS_START, I2S_XFER_TXS_STOP | I2S_XFER_RXS_STOP);
            udelay(150);
            regmap_update_bits(i2s_tdm->regmap, I2S_CLR,
                I2S_CLR_TXC | I2S_CLR_RXC, I2S_CLR_TXC | I2S_CLR_RXC);
            regmap_read(i2s_tdm->regmap, I2S_CLR, &val);
            /* Should wait for clear operation to finish */
            while (val && retry > 0) {
                regmap_read(i2s_tdm->regmap, I2S_CLR, &val);
                retry--;
            }
        }
    }
    
    AUDIO_DEVICE_LOG_DEBUG("success");
    return;
}

/* normal scene */
int32_t Rk3568NormalTrigger(const struct AudioCard *card, int cmd, const struct DaiDevice *device)
{
    AUDIO_DEVICE_LOG_DEBUG("entry");
    struct device_node    *dmaOfNode;
    struct platform_device *platformdev;
    dmaOfNode = of_find_node_by_path("/i2s@fe410000");
    if (dmaOfNode == NULL) {
        pr_err("of_node is NULL.");
    }
    platformdev = of_find_device_by_node(dmaOfNode);
    struct rk3568_i2s_tdm_dev *i2s_tdm = dev_get_drvdata(&platformdev->dev);
    int32_t triggerFlag = GetTriggeredFlag(cmd);
    enum AudioStreamType streamType = GetStreamType(cmd);

    Rk3568TxAndRxSetReg(i2s_tdm, streamType, triggerFlag);
    AUDIO_DEVICE_LOG_DEBUG("success");
    return HDF_SUCCESS;
}