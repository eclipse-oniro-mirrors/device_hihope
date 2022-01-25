/*
   Copyright (c) [2022] [huangji@nj.iscas.ac.cn]
   [Software Name] is licensed under Mulan PSL v2.
   You can use this software according to the terms and conditions of the Mulan PSL v2. 
   You may obtain a copy of Mulan PSL v2 at:
               http://license.coscl.org.cn/MulanPSL2 
   THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.  
   See the Mulan PSL v2 for more details.  
*/

#include <linux/device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/mfd/rk808.h>
#include "rk817_codec.h"
#include "rk809_codec_impl.h"
#include "audio_accessory_base.h"
#include "audio_codec_if.h"
#include "audio_codec_base.h"
#include "audio_driver_log.h"

struct platform_device *rk817_pdev;

static const struct of_device_id rk817_codec_dt_ids[] = {
	{ .compatible = "rockchip,rk817-codec" },
	{},
};
MODULE_DEVICE_TABLE(of, rk817_codec_dt_ids);

static int rk817_platform_probe(struct platform_device *pdev) {
    rk817_pdev = pdev;
    dev_info(&pdev->dev, "got rk817-codec platform_device");
    return 0;
}

static int rk817_platform_remove(struct platform_device *pdev) {
    // TODO:
    //   unreigster it from ADM
    //   set rk817_pdev = NULL
    //   not implement it now
    panic("%s not support now", __func__);
}

static struct platform_driver rk817_codec_driver = {
	.driver = {
		   .name = "rk817-codec",
		   .of_match_table = rk817_codec_dt_ids,
		   },
	.probe = rk817_platform_probe,
	.remove = rk817_platform_remove,
};

module_platform_driver(rk817_codec_driver);

static const struct reg_default rk817_reg_defaults[] = {
	{ RK817_CODEC_DTOP_VUCTL, 0x003 },
	{ RK817_CODEC_DTOP_VUCTIME, 0x00 },
	{ RK817_CODEC_DTOP_LPT_SRST, 0x00 },
	{ RK817_CODEC_DTOP_DIGEN_CLKE, 0x00 },
	{ RK817_CODEC_AREF_RTCFG0, 0x00 },
	{ RK817_CODEC_AREF_RTCFG1, 0x06 },
	{ RK817_CODEC_AADC_CFG0, 0xc8 },
	{ RK817_CODEC_AADC_CFG1, 0x00 },
	//{ RK817_CODEC_DADC_VOLL, 0x00 },
	//{ RK817_CODEC_DADC_VOLR, 0x00 },
	{ RK817_CODEC_DADC_SR_ACL0, 0x00 },
	{ RK817_CODEC_DADC_ALC1, 0x00 },
	{ RK817_CODEC_DADC_ALC2, 0x00 },
	{ RK817_CODEC_DADC_NG, 0x00 },
	{ RK817_CODEC_DADC_HPF, 0x00 },
	{ RK817_CODEC_DADC_RVOLL, 0xff },
	{ RK817_CODEC_DADC_RVOLR, 0xff },
	{ RK817_CODEC_AMIC_CFG0, 0x70 },
	{ RK817_CODEC_AMIC_CFG1, 0x00 },
	{ RK817_CODEC_DMIC_PGA_GAIN, 0x66 },
	{ RK817_CODEC_DMIC_LMT1, 0x00 },
	{ RK817_CODEC_DMIC_LMT2, 0x00 },
	{ RK817_CODEC_DMIC_NG1, 0x00 },
	{ RK817_CODEC_DMIC_NG2, 0x00 },
	{ RK817_CODEC_ADAC_CFG0, 0x00 },
	{ RK817_CODEC_ADAC_CFG1, 0x07 },
	{ RK817_CODEC_DDAC_POPD_DACST, 0x82 },
	{ RK817_CODEC_DDAC_VOLL, 0x00 },
	{ RK817_CODEC_DDAC_VOLR, 0x00 },
	{ RK817_CODEC_DDAC_SR_LMT0, 0x00 },
	{ RK817_CODEC_DDAC_LMT1, 0x00 },
	{ RK817_CODEC_DDAC_LMT2, 0x00 },
	{ RK817_CODEC_DDAC_MUTE_MIXCTL, 0xa0 },
	{ RK817_CODEC_DDAC_RVOLL, 0xff },
	{ RK817_CODEC_DDAC_RVOLR, 0xff },
	{ RK817_CODEC_AHP_ANTI0, 0x00 },
	{ RK817_CODEC_AHP_ANTI1, 0x00 },
	{ RK817_CODEC_AHP_CFG0, 0xe0 },
	{ RK817_CODEC_AHP_CFG1, 0x1f },
	{ RK817_CODEC_AHP_CP, 0x09 },
	{ RK817_CODEC_ACLASSD_CFG1, 0x69 },
	{ RK817_CODEC_ACLASSD_CFG2, 0x44 },
	{ RK817_CODEC_APLL_CFG0, 0x04 },
	{ RK817_CODEC_APLL_CFG1, 0x00 },
	{ RK817_CODEC_APLL_CFG2, 0x30 },
	{ RK817_CODEC_APLL_CFG3, 0x19 },
	{ RK817_CODEC_APLL_CFG4, 0x65 },
	{ RK817_CODEC_APLL_CFG5, 0x01 },
	{ RK817_CODEC_DI2S_CKM, 0x01 },
	{ RK817_CODEC_DI2S_RSD, 0x00 },
	{ RK817_CODEC_DI2S_RXCR1, 0x00 },
	{ RK817_CODEC_DI2S_RXCR2, 0x17 },
	{ RK817_CODEC_DI2S_RXCMD_TSD, 0x00 },
	{ RK817_CODEC_DI2S_TXCR1, 0x00 },
	{ RK817_CODEC_DI2S_TXCR2, 0x17 },
	{ RK817_CODEC_DI2S_TXCR3_TXCMD, 0x00 },
};

static bool rk817_volatile_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case RK817_CODEC_DTOP_LPT_SRST:
		return true;
	default:
		return false;
	}
}

static bool rk817_codec_register(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case RK817_CODEC_DTOP_VUCTL:
	case RK817_CODEC_DTOP_VUCTIME:
	case RK817_CODEC_DTOP_LPT_SRST:
	case RK817_CODEC_DTOP_DIGEN_CLKE:
	case RK817_CODEC_AREF_RTCFG0:
	case RK817_CODEC_AREF_RTCFG1:
	case RK817_CODEC_AADC_CFG0:
	case RK817_CODEC_AADC_CFG1:
	case RK817_CODEC_DADC_VOLL:
	case RK817_CODEC_DADC_VOLR:
	case RK817_CODEC_DADC_SR_ACL0:
	case RK817_CODEC_DADC_ALC1:
	case RK817_CODEC_DADC_ALC2:
	case RK817_CODEC_DADC_NG:
	case RK817_CODEC_DADC_HPF:
	case RK817_CODEC_DADC_RVOLL:
	case RK817_CODEC_DADC_RVOLR:
	case RK817_CODEC_AMIC_CFG0:
	case RK817_CODEC_AMIC_CFG1:
	case RK817_CODEC_DMIC_PGA_GAIN:
	case RK817_CODEC_DMIC_LMT1:
	case RK817_CODEC_DMIC_LMT2:
	case RK817_CODEC_DMIC_NG1:
	case RK817_CODEC_DMIC_NG2:
	case RK817_CODEC_ADAC_CFG0:
	case RK817_CODEC_ADAC_CFG1:
	case RK817_CODEC_DDAC_POPD_DACST:
	case RK817_CODEC_DDAC_VOLL:
	case RK817_CODEC_DDAC_VOLR:
	case RK817_CODEC_DDAC_SR_LMT0:
	case RK817_CODEC_DDAC_LMT1:
	case RK817_CODEC_DDAC_LMT2:
	case RK817_CODEC_DDAC_MUTE_MIXCTL:
	case RK817_CODEC_DDAC_RVOLL:
	case RK817_CODEC_DDAC_RVOLR:
	case RK817_CODEC_AHP_ANTI0:
	case RK817_CODEC_AHP_ANTI1:
	case RK817_CODEC_AHP_CFG0:
	case RK817_CODEC_AHP_CFG1:
	case RK817_CODEC_AHP_CP:
	case RK817_CODEC_ACLASSD_CFG1:
	case RK817_CODEC_ACLASSD_CFG2:
	case RK817_CODEC_APLL_CFG0:
	case RK817_CODEC_APLL_CFG1:
	case RK817_CODEC_APLL_CFG2:
	case RK817_CODEC_APLL_CFG3:
	case RK817_CODEC_APLL_CFG4:
	case RK817_CODEC_APLL_CFG5:
	case RK817_CODEC_DI2S_CKM:
	case RK817_CODEC_DI2S_RSD:
	case RK817_CODEC_DI2S_RXCR1:
	case RK817_CODEC_DI2S_RXCR2:
	case RK817_CODEC_DI2S_RXCMD_TSD:
	case RK817_CODEC_DI2S_TXCR1:
	case RK817_CODEC_DI2S_TXCR2:
	case RK817_CODEC_DI2S_TXCR3_TXCMD:
		return true;
	default:
		return false;
	}
}

static const struct regmap_config rk817_codec_regmap_config = {
	.name = "rk817-codec",
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
	.max_register = 0x4f,
	.cache_type = REGCACHE_FLAT,
	.volatile_reg = rk817_volatile_register,
	.writeable_reg = rk817_codec_register,
	.readable_reg = rk817_codec_register,
	.reg_defaults = rk817_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(rk817_reg_defaults),
};

#define HDF_LOG_TAG "rk809_codec_adapter"

struct CodecData g_rk809Data = {
    .Init = Rk809DeviceInit,
    .Read = RK809CodecReadReg,
    .Write = RK809CodecWriteReg,
};

struct AudioDaiOps g_rk809DaiDeviceOps = {
    .Startup = Rk809DaiStartup,
    .HwParams = Rk809DaiHwParams,
	.Trigger = RK809NormalTrigger,
};

struct DaiData g_rk809DaiData = {
    .DaiInit = Rk809DaiDeviceInit,
    .ops = &g_rk809DaiDeviceOps,
};

struct Rk809ChipData *g_chip;

/* HdfDriverEntry */
static int32_t GetServiceName(const struct HdfDeviceObject *device)
{
	// struct Rk809ChipData *chip = device->priv;
    const struct DeviceResourceNode *node = NULL;
    struct DeviceResourceIface *drsOps = NULL;
    int32_t ret;

    if (device == NULL) {
        AUDIO_DEVICE_LOG_ERR("input HdfDeviceObject object is nullptr.");
        return HDF_FAILURE;
    }
    node = device->property;
    if (node == NULL) {
        AUDIO_DEVICE_LOG_ERR("get drs node is nullptr.");
        return HDF_FAILURE;
    }
    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetString == NULL) {
        AUDIO_DEVICE_LOG_ERR("drsOps or drsOps getString is null!");
        return HDF_FAILURE;
    }
    ret = drsOps->GetString(node, "serviceName", &g_chip->codec.drvCodecName, 0);
    if (ret != HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("read serviceName failed.");
        return ret;
    }

    return HDF_SUCCESS;
}

/* HdfDriverEntry implementations */
static int32_t Rk809DriverBind(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("input para is NULL.");
        return HDF_FAILURE;
    }

    struct CodecHost *codecHost = (struct CodecHost *)OsalMemCalloc(sizeof(*codecHost));
    if (codecHost == NULL) {
        AUDIO_DRIVER_LOG_ERR("malloc codecHost fail!");
        return HDF_FAILURE;
    }
    codecHost->device = device;
    device->service = &codecHost->service;

    return HDF_SUCCESS;
}

static void RK809ChipRelease(void);

static int32_t Rk809DriverInit(struct HdfDeviceObject *device)
{
    int32_t ret;

    if (!rk817_pdev) {
        AUDIO_DEVICE_LOG_ERR("rk817_pdev not ready");
        return HDF_FAILURE;
    }

    g_chip = devm_kzalloc(&rk817_pdev->dev, sizeof(struct Rk809ChipData), GFP_KERNEL);
    if (!g_chip) {
        AUDIO_DEVICE_LOG_ERR("no memory");
        return HDF_ERR_MALLOC_FAIL;
    }
    g_chip->codec = g_rk809Data;
    g_chip->dai = g_rk809DaiData;
    platform_set_drvdata(rk817_pdev, g_chip);
    // device->priv = chip;
    g_chip->pdev = rk817_pdev;
    //g_chip->hdev = device;

    struct rk808 *rk808 = dev_get_drvdata(g_chip->pdev->dev.parent);
    if (!rk808) {
        AUDIO_DEVICE_LOG_ERR("%s: rk808 is NULL\n", __func__);
        ret = HDF_FAILURE;
        RK809ChipRelease();
		return ret;
    }
    g_chip->regmap = devm_regmap_init_i2c(rk808->i2c,
		&rk817_codec_regmap_config);
    if (IS_ERR(g_chip->regmap)) {
        AUDIO_DEVICE_LOG_ERR("failed to allocate regmap: %ld\n", PTR_ERR(g_chip->regmap));
        RK809ChipRelease();
		return ret;
    }

    ret = CodecGetConfigInfo(device, &(g_chip->codec));
    if (ret !=  HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("get config info failed.");
        RK809ChipRelease();
		return ret;
    }
	if (CodecSetConfigInfo(&(g_chip->codec),  &(g_chip->dai)) != HDF_SUCCESS) {
		AUDIO_DEVICE_LOG_ERR("set config info failed.");
        return HDF_FAILURE;
    }

    ret = GetServiceName(device);
    if (ret !=  HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("GetServiceName failed.");
        RK809ChipRelease();
		return ret;
    }
	if (CodecGetDaiName(device,  &(g_chip->dai.drvDaiName)) != HDF_SUCCESS) {
		AUDIO_DRIVER_LOG_INFO("hhhhh[ %s : %d ],  enter CodecGetDaiName if return error",__func__,__LINE__);
        return HDF_FAILURE;
    }

    ret = AudioRegisterCodec(device, &(g_chip->codec), &(g_chip->dai));
    if (ret !=  HDF_SUCCESS) {
        AUDIO_DEVICE_LOG_ERR("AudioRegisterAccessory failed.");
        RK809ChipRelease();
		return ret;
    }

    return HDF_SUCCESS;
}

static void RK809ChipRelease(void) 
{
    if (g_chip) {
        // device->priv = NULL;
        platform_set_drvdata(g_chip->pdev, NULL);
        if (g_chip->regmap) {
            regmap_exit(g_chip->regmap);
        }
        devm_kfree(&g_chip->pdev->dev, g_chip);
    }
	AUDIO_DEVICE_LOG_ERR("success!");
	return;
}

static void RK809DriverRelease(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        AUDIO_DRIVER_LOG_ERR("device is NULL");
        return;
    }

    if (device->priv != NULL) {
        OsalMemFree(device->priv);
    }
    struct CodecHost *codecHost = (struct CodecHost *)device->service;
    if (codecHost == NULL) {
        HDF_LOGE("CodecDriverRelease: codecHost is NULL");
        return;
    }
    OsalMemFree(codecHost);
}

/* HdfDriverEntry definitions */
struct HdfDriverEntry g_Rk809DriverEntry = {
    .moduleVersion = 1,
    .moduleName = "CODEC_RK809",
    .Bind = Rk809DriverBind,
    .Init = Rk809DriverInit,
    .Release = RK809DriverRelease,
};

HDF_INIT(g_Rk809DriverEntry);