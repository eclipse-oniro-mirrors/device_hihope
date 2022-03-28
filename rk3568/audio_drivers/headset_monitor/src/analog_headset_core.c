/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/iio/consumer.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include "analog_headset.h"
#include "device_resource_if.h"
#include "osal_mem.h"
#include "securec.h"

#define HDF_LOG_TAG analog_headset_core

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

    HDF_LOGD("%s: hsGpioFlags = %d, isHookAdcMode = %s",
        __func__, pdata->hsGpioFlags, pdata->isHookAdcMode ? "true" : "false");
#ifdef CONFIG_MODEM_MIC_SWITCH
    HDF_LOGD("%s: micGpioFlags = %d, micSwitchGpio = %u, hpMicIoValue = %u, mainMicIoValue = %u.",
        __func__, pdata->micGpioFlags, pdata->micSwitchGpio, pdata->hpMicIoValue, pdata->mainMicIoValue);
#endif

    HDF_LOGD("%s: hsGpio = %u, hookGpio = %u, hookDownType = %u, hsWakeup = %s.", __func__,
        pdata->hsGpio, pdata->hookGpio, pdata->hookDownType, pdata->hsWakeup ? "true" : "false");
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
    int32_t ret;
    int32_t wakeup;

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

static int32_t ReadHookModeConfig(struct DeviceResourceIface *parser,
    const struct DeviceResourceNode *node, struct HeadsetPdata *pdata)
{
    int32_t ret;

    if ((pdata == NULL) || (node == NULL) || (parser == NULL)) {
        HDF_LOGE("%s: pdata, node or parser is NULL.", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    pdata->isHookAdcMode = true;
    ret = parser->GetUint32(node, "hook_gpio", &pdata->hookGpio, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGI("%s: [GetUint32]-[hook_gpio] is null.", __func__);
        pdata->isHookAdcMode = false;
    }

    if (pdata->isHookAdcMode) { /* hook adc mode */
        HDF_LOGI("%s: headset have hook adc mode.", __func__);
        ret = parser->GetUint32(node, "adc_controller_no", &pdata->adcConfig.devNum, 0);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: [GetUint32]-[adc_controller_no] failed.", __func__);
            return ret;
        }
        ret = parser->GetUint32(node, "adc_channel", &pdata->adcConfig.chanNo, 0);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: [GetUint32]-[adc_channel] failed.", __func__);
            return ret;
        }
    } else { /* hook interrupt mode */
        ret = parser->GetUint32(node, "hook_down_type", &pdata->hookDownType, 0);
        if (ret != HDF_SUCCESS) {
            HDF_LOGE("%s: [GetUint32]-[hook_down_type] failed.", __func__);
            return ret;
        }
    }
    HDF_LOGI("%s: hook mode: %s.", __func__, pdata->isHookAdcMode ? "sar-adc" : "gpio-int");

    return HDF_SUCCESS;
}

static int32_t ReadMicConfig(struct DeviceResourceIface *parser,
    const struct DeviceResourceNode *node, struct HeadsetPdata *pdata)
{
#ifdef CONFIG_MODEM_MIC_SWITCH
    /* mic */
    int32_t ret;

    if ((pdata == NULL) || (parser == NULL) || (node == NULL)) {
        HDF_LOGE("%s: node or pdata is NULL.", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    ret = parser->GetUint32(node, "mic_switch_gpio", &pdata->hsGpio, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [mic_switch_gpio] failed.", __func__);
        return ret;
    }

    ret = parser->GetUint32(node, "hp_mic_io_value", &pdata->hpMicIoValue, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [hp_mic_io_value] failed.", __func__);
        return ret;
    }
    ret = parser->GetUint32(node, "main_mic_io_value", &pdata->mainMicIoValue, 1);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [main_mic_io_value] failed.", __func__);
        return ret;
    }
#endif

    return HDF_SUCCESS;
}

static int32_t ReadConfig(const struct DeviceResourceNode *node, struct HeadsetPdata *pdata)
{
    int32_t ret;
    int32_t temp;
    struct DeviceResourceIface *parser = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);

    if ((pdata == NULL) || (node == NULL) || (parser == NULL)) {
        HDF_LOGE("%s: pdata, node or parser is NULL.", __func__);
        return HDF_FAILURE;
    }
    ret = parser->GetString(node, "dev_name", &pdata->devName, NULL);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [GetString]-[dev_name] failed.", __func__);
        return ret;
    }
    /* headset */
    ret = parser->GetUint32(node, "headset_gpio", &pdata->hsGpio, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [headset_gpio] failed.", __func__);
        return ret;
    }
    ret = parser->GetUint32(node, "headset_gpio_flag", &pdata->hsGpioFlag, OF_GPIO_ACTIVE_LOW);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [headset_gpio_flag] failed.", __func__);
        return ret;
    }
    /* hook */
    ret = ReadHookModeConfig(parser, node, pdata);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [ReadHookModeConfig] failed.", __func__);
        return ret;
    }
    /* mic */
    (void)ReadMicConfig(parser, node, pdata);

    ret = parser->GetUint32(node, "headset_wakeup", &temp, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGW("%s: [GetUint32]-[headset_wakeup] failed.", __func__);
        temp = 1;
    }
    pdata->hsWakeup = (temp == 0) ? false : true;

    return HDF_SUCCESS;
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
    pdata = (struct HeadsetPdata *)OsalMemCalloc(sizeof(*pdata));
    if (pdata == NULL) {
        HDF_LOGE("%s: [OsalMemCalloc] failed!", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    InitHeadsetPdata(pdata);
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
    int32_t ret;
    static struct IDeviceIoService headsetService = {
        .object.objectId = 1,
    };
    const struct DeviceResourceNode *node = NULL;
    static struct HeadsetPdata pdata;

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

    node = device->property;
    ret = ReadConfig(node, &pdata);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [ReadConfig] failed.", __func__);
    }
    (void)TraceInfo(&pdata);

    g_pdataInfo->device = device;
    g_pdataInfo->ioService = headsetService;
    device->service = &g_pdataInfo->ioService;
    device->priv = (void *)g_pdataInfo;
    HDF_LOGI("%s: success.", __func__);

    return HDF_SUCCESS;
}

static void HdfHeadsetExit(struct HdfDeviceObject *device)
{
    struct HeadsetPdata *drvData = NULL;

    HDF_LOGI("%s: enter.", __func__);
    if (device == NULL) {
        HDF_LOGE("%s: device or device->service is NULL.", __func__);
        return;
    }

    platform_driver_unregister(&AudioHeadsetDriver);

    if ((device == NULL) || (device->priv == NULL)) {
        HDF_LOGE("%s: device or device->priv is NULL.", __func__);
        return;
    }
    drvData = (struct HeadsetPdata *)device->priv;
    if (drvData->chan != NULL) { // hook adc mode
        AnalogHeadsetAdcRelease(drvData);
    } else { // hook interrupt mode and not hook
        AnalogHeadsetGpioRelease(drvData);
    }
    OsalMemFree(drvData);
    device->priv = NULL;
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