/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/iio/consumer.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/spi/spi.h>
#include "analog_headset.h"
#include "securec.h"

#define HDF_LOG_TAG analog_headset_core
#define USING_HDF_ENTRY

static struct HeadsetPdata *g_pdataInfo = NULL;
static struct HdfDeviceObject *g_hdfDevice = NULL;
static void InitHeadsetPdata(struct HeadsetPdata *pdata)
{
    if (pdata == NULL) {
        HDF_LOGE("%s: pdata is NULL!", __func__);
        return;
    }
    if (g_hdfDevice == NULL) {
        HDF_LOGE("%s: g_hdfDevice is NULL!", __func__);
        return;
    }

    pdata->device = g_hdfDevice;
    pdata->hsGpioFlags = 0;
    pdata->hsGpio = 0;
    pdata->hsInsertType = (pdata->hsGpioFlags & OF_GPIO_ACTIVE_LOW) ? HEADSET_IN_LOW : HEADSET_IN_HIGH;

    /* hook */
    pdata->hookGpio = 0;
    pdata->hookDownType = 0;
    pdata->isHookAdcMode = false;

    /* mic */
#ifdef CONFIG_MODEM_MIC_SWITCH
    pdata->micGpioFlags = 0;
    pdata->hsGpio = 0;
    pdata->hpMicIoValue = LEVEL_IN_LOW;
    pdata->mainMicIoValue = LEVEL_IN_HIGH;
#endif

    pdata->hsWakeup = true;
}

static int32_t GpioDirectionInput(struct device *dev, unsigned gpio, const char *label)
{
    int32_t ret;

    if ((dev == NULL) || (label == NULL)) {
        HDF_LOGE("%s: dev or label is NULL.", __func__);
        return -EINVAL;
    }

    ret = devm_gpio_request(dev, gpio, label);
    if (ret < 0) {
        HDF_LOGE("%s: [devm_gpio_request] failed.", __func__);
        return ret;
    }
    ret = gpio_direction_input(gpio);
    if (ret < 0) {
        HDF_LOGE("%s: [gpio_direction_input] failed.", __func__);
        return ret;
    }

    return ret;
}

static int32_t TraceInfo(const struct HeadsetPdata *pdata)
{
    HDF_LOGI("%s: enter.", __func__);
    if (pdata == NULL) {
        HDF_LOGE("%s: pdata is null", __func__);
        return -EINVAL;
    }

    HDF_LOGI("\n hsGpioFlags = %d, isHookAdcMode = %d",
        pdata->hsGpioFlags, pdata->isHookAdcMode ? 1 : 0);
#ifdef CONFIG_MODEM_MIC_SWITCH
    HDF_LOGI("\n micGpioFlags = %d, micSwitchGpio = %d, hpMicIoValue = %d, mainMicIoValue = %d.",
        pdata->micGpioFlags, pdata->micSwitchGpio, pdata->hpMicIoValue, pdata->mainMicIoValue);
#endif

    HDF_LOGI("\n hsGpio = %d, hookGpio = %d, hookDownType = %d, hsGpio = %d, hsWakeup = %s.",
        pdata->hsGpio, pdata->hookGpio, pdata->hookDownType, pdata->hsGpio, pdata->hsWakeup ? "true" : "false");
    HDF_LOGI("%s: done.", __func__);

    return 0;
}

static int32_t LinuxReadMicConfig(struct device_node *node, struct HeadsetPdata *pdata)
{
#ifdef CONFIG_MODEM_MIC_SWITCH
    /* mic */
    int32_t ret;

    if ((node == NULL) || (pdata == NULL)) {
        HDF_LOGE("%s: node or pdata is NULL.", __func__);
        return -EINVAL;
    }

    ret = of_get_named_gpio_flags(node, "mic_switch_gpio", 0, &pdata->micGpioFlags);
    if (ret < 0) {
        HDF_LOGD("%s: Can not read property micSwitchGpio.", __func__);
    } else {
        pdata->hsGpio = ret;
        ret = of_property_read_u32(node, "hp_mic_io_value", &pdata->hpMicIoValue);
        if (ret < 0) {
            HDF_LOGD("%s: have not set hpMicIoValue ,so default set pull down low level.", __func__);
            pdata->hpMicIoValue = LEVEL_IN_LOW;
        }
        ret = of_property_read_u32(node, "main_mic_io_value", &pdata->mainMicIoValue);
        if (ret < 0) {
            HDF_LOGD("%s: have not set mainMicIoValue ,so default set pull down low level.", __func__);
            pdata->mainMicIoValue = LEVEL_IN_HIGH;
        }
    }
#endif

    return 0;
}

static int32_t LinuxReadConfig(struct device_node *node, struct HeadsetPdata *pdata)
{
    int ret;
    int wakeup;

    if ((node == NULL) || (pdata == NULL)) {
        HDF_LOGE("%s: node or pdata is NULL.", __func__);
        return -EINVAL;
    }

    /* headset */
    ret = of_get_named_gpio_flags(node, "headset_gpio", 0, &pdata->hsGpioFlags);
    if (ret < 0) {
        HDF_LOGE("%s: Can not read property hsGpio.", __func__);
        return ret;
    }
    pdata->hsGpio = ret;
    pdata->hsInsertType = (pdata->hsGpioFlags & OF_GPIO_ACTIVE_LOW) ? HEADSET_IN_LOW : HEADSET_IN_HIGH;

    /* hook */
    ret = of_get_named_gpio_flags(node, "hook_gpio", 0, &pdata->hookGpio);
    if (ret < 0) {
        HDF_LOGW("%s: Can not read property hookGpio.", __func__);
        pdata->hookGpio = 0;
        /* adc mode */
        pdata->isHookAdcMode = true;
    } else {
        ret = of_property_read_u32(node, "hook_down_type", &pdata->hookDownType);
        if (ret < 0) {
            HDF_LOGW("%s: have not set hookDownType,set >hook< insert type low level default.", __func__);
            pdata->hookDownType = 0;
        }
        pdata->isHookAdcMode = false;
    }
    /* mic */
    (void)LinuxReadMicConfig(node, pdata);

    ret = of_property_read_u32(node, "rockchip,headset_wakeup", &wakeup);
    if (ret < 0) {
        pdata->hsWakeup = true;
    } else {
        pdata->hsWakeup = (wakeup == 0) ? false : true;
    }

    return 0;
}

static int32_t AnalogHeadsetInit(struct platform_device *pdev, struct HeadsetPdata *pdata)
{
    int32_t ret;

    if ((pdev == NULL) || (pdata == NULL)) {
        HDF_LOGE("%s: pdev or pdata is NULL.", __func__);
        return -EINVAL;
    }

    /* headset */
    ret = GpioDirectionInput(&pdev->dev, pdata->hsGpio, "headset_gpio");
    if (ret < 0) {
        HDF_LOGE("%s: [GpioDirectionInput]-[headset_gpio] failed.", __func__);
        return ret;
    }

    /* hook */
    if (pdata->isHookAdcMode) {
        pdata->chan = iio_channel_get(&pdev->dev, NULL);
        if (IS_ERR(pdata->chan)) {
            pdata->chan = NULL;
            HDF_LOGW("%s: have not set adc chan.", __func__);
        }
    } else {
        ret = GpioDirectionInput(&pdev->dev, pdata->hookGpio, "hook_gpio");
        if (ret < 0) {
            HDF_LOGW("%s: [GpioDirectionInput]-[hook_gpio] failed.", __func__);
            return ret;
        }
    }

    if (pdata->chan != NULL) { /* hook adc mode */
        HDF_LOGI("%s: headset have hook adc mode.", __func__);
        ret = AnalogHeadsetAdcInit(pdev, pdata);
        if (ret < 0) {
            HDF_LOGE("%s: [AnalogHeadsetAdcInit] failed.", __func__);
            return ret;
        }
    } else { /* hook interrupt mode and not hook */
        HDF_LOGI("%s: headset have %s mode.", __func__, pdata->hookGpio ? "interrupt hook" : "no hook");
        ret = AnalogHeadsetGpioInit(pdev, pdata);
        if (ret < 0) {
            HDF_LOGE("%s: [AnalogHeadsetGpioInit] failed.", __func__);
            return ret;
        }
    }
    return ret;
}

static int AudioHeadsetProbe(struct platform_device *pdev)
{
    struct device_node *node = pdev->dev.of_node;
    struct HeadsetPdata *pdata;
    int32_t ret;

    HDF_LOGI("%s: enter.", __func__);
    pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
    if (pdata == NULL) {
        HDF_LOGI("%s: failed to allocate driver data.", __func__);
        return -ENOMEM;
    }
    InitHeadsetPdata(pdata); // this needs to add.
    g_pdataInfo = pdata;

    ret = LinuxReadConfig(node, pdata);
    (void)TraceInfo(pdata);
    if (ret < 0) {
        HDF_LOGE("%s: [LinuxReadConfig] failed.", __func__);
        return ret;
    }

    ret = AnalogHeadsetInit(pdev, pdata);
    if (ret < 0) {
        HDF_LOGE("%s: [AnalogHeadsetInit] failed.", __func__);
        return ret;
    }
    HDF_LOGI("%s: success.", __func__);

    return ret;
}

static int AudioHeadsetRemove(struct platform_device *pdev)
{
    (void)pdev;
    return 0;
}

static int AudioHeadsetSuspend(struct platform_device *pdev, pm_message_t state)
{
    if (g_pdataInfo->chan != NULL) {
        return AnalogHeadsetAdcSuspend(pdev, state);
    }
    return 0;
}

static int AudioHeadsetResume(struct platform_device *pdev)
{
    if (g_pdataInfo->chan != NULL) {
        return AnalogHeadsetAdcResume(pdev);
    }
    return 0;
}

static const struct of_device_id g_headsetOfMatch[] = {
    { .compatible = "rockchip_headset", },
    {},
};
MODULE_DEVICE_TABLE(of, g_headsetOfMatch);

static struct platform_driver AudioHeadsetDriver = {
    .probe = AudioHeadsetProbe,
    .remove = AudioHeadsetRemove,
    .resume = AudioHeadsetResume,
    .suspend = AudioHeadsetSuspend,
    .driver = {
        .name = "rockchip_headset",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(g_headsetOfMatch),
    },
};

#ifndef USING_HDF_ENTRY
static int __init AudioHeadsetInit(void)
{
    platform_driver_register(&AudioHeadsetDriver);
    return 0;
}

static void __exit AudioHeadsetExit(void)
{
    platform_driver_unregister(&AudioHeadsetDriver);
}

late_initcall(AudioHeadsetInit);
module_exit(AudioHeadsetExit);

MODULE_DESCRIPTION("Rockchip Headset Core Driver");
MODULE_LICENSE("GPL");
#else
static int32_t HdfHeadsetBindDriver(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        HDF_LOGE("%s: device is NULL.", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    g_hdfDevice = device;
    g_pdataInfo = NULL;

    return HDF_SUCCESS;
}

static int32_t HdfHeadsetInit(struct HdfDeviceObject *device)
{
    static struct IDeviceIoService headsetService = {
        .object.objectId = 1,
    };

    HDF_LOGI("%s: enter.", __func__);
    platform_driver_register(&AudioHeadsetDriver);

    if (device == NULL) {
        HDF_LOGE("%s:  is NULL.", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (g_pdataInfo == NULL) {
        HDF_LOGE("%s: g_pdataInfo is NULL!", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    g_pdataInfo->device = device;
    g_pdataInfo->ioService = headsetService;
    device->service = &g_pdataInfo->ioService;
    device->priv = (void *)g_pdataInfo;
    HDF_LOGI("%s: success.", __func__);

    return HDF_SUCCESS;
}

static void HdfHeadsetExit(struct HdfDeviceObject *device)
{
    HDF_LOGI("%s: enter.", __func__);
    if (device == NULL) {
        HDF_LOGE("%s: device or device->service is NULL.", __func__);
        return;
    }

    platform_driver_unregister(&AudioHeadsetDriver);
    g_pdataInfo = NULL;
    g_hdfDevice = NULL;
    HDF_LOGI("%s: done.", __func__);
}

/* HdfDriverEntry definitions */
struct HdfDriverEntry g_headsetDevEntry = {
    .moduleVersion = 1,
    .moduleName = "AUDIO_ANALOG_HEADSET",
    .Bind = HdfHeadsetBindDriver,
    .Init = HdfHeadsetInit,
    .Release = HdfHeadsetExit,
};

HDF_INIT(g_headsetDevEntry);
#endif