/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/extcon-provider.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/iio/consumer.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include "analog_headset.h"
#include "analog_headset_base.h"
#include "hdf_workqueue.h"
#include "osal_time.h"
#include "osal_mem.h"
#include "securec.h"

#define HDF_LOG_TAG analog_headset_adc
#define HOOK_ADC_SAMPLE_TIME 100

#define HOOK_LEVEL_HIGH 410 // 1V*1024/2.5
#define HOOK_LEVEL_LOW 204 // 0.5V*1024/2.5
#define HOOK_DEFAULT_VAL 1024

#define HEADSET_IN 1
#define HEADSET_OUT 0
#define HOOK_DOWN 1
#define HOOK_UP 0

#define HEADSET_TIMER 1
#define HOOK_TIMER 2

#define WAIT 2
#define BUSY 1
#define IDLE 0

/* headset private data */
struct HeadsetPriv {
    struct input_dev *inDev;
    struct HeadsetPdata *pdata;
    uint32_t hsStatus : 1;
    uint32_t hookStatus : 1;
    struct iio_channel *chan;
    /* headset interrupt working will not check hook key  */
    uint32_t hsIrqWorking;
    int32_t curHsStatus;
    HdfWorkQueue workQueue;
    uint32_t irq[HS_HOOK_COUNT];
    HdfWork hDelayedWork[HS_HOOK_COUNT];
    struct extcon_dev *edev;
    unsigned char *keycodes;
    HdfWork hHookWork;
    /* ms */
    uint32_t hookTime;
    bool isMic;
};

static struct HeadsetPriv *g_hsInfo = NULL;

static int ExtconSetStateSync(struct HeadsetPriv *hs, unsigned int id, bool state)
{
    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL!", __func__);
        return -EINVAL;
    }

    extcon_set_state_sync(hs->edev, id, state);
    SetStateSync(id, state);
    HDF_LOGI("%s: id = %u, state = %s.", __func__, id, state ? "in" : "out");

    return 0;
}

static void InputReportKeySync(struct HeadsetPriv *hs, unsigned int code, int value)
{
    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL!", __func__);
        return;
    }

    input_report_key(hs->inDev, code, value);
    input_sync(hs->inDev);
    SetStateSync(code, value);
    HDF_LOGI("%s: code = %u, value = %s.", __func__, code, value ? "in" : "out");
}

static void InitHeadsetPriv(struct HeadsetPriv *hs, struct HeadsetPdata *pdata)
{
    if ((hs == NULL) || (pdata == NULL)) {
        HDF_LOGE("%s: hs or pdata is NULL!", __func__);
        return;
    }

    hs->pdata = pdata;
    hs->hsStatus = HEADSET_OUT;
    hs->hsIrqWorking = IDLE;
    hs->hookStatus = HOOK_UP;
    hs->hookTime = HOOK_ADC_SAMPLE_TIME;
    hs->curHsStatus = BIT_HEADSET_NULL;
    hs->isMic = false;
    HDF_LOGD("%s, LINE = %d: isMic = %s.", __func__, __LINE__, hs->isMic ? "true" : "false");
    hs->chan = pdata->chan;
}

static int32_t CheckState(struct HeadsetPriv *hs, bool *beChange)
{
    struct HeadsetPdata *pdata = NULL;
    static uint32_t oldStatus = 0;
    int32_t i;
    int32_t level = 0;

    if ((hs == NULL) || (hs->pdata == NULL) || (beChange == NULL)) {
        HDF_LOGE("%s: hs, pdata or beChange is NULL.", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    pdata = hs->pdata;
    OsalMSleep(IRQ_CONFIRM_MS150);
    for (i = 0; i < GET_GPIO_REPEAT_TIMES; i++) {
        level = gpio_get_value(pdata->hsGpio);
        if (level < 0) {
            HDF_LOGE("%s: get pin level again, pin=%u, i=%d.", __func__, pdata->hsGpio, i);
            OsalMSleep(IRQ_CONFIRM_MS1);
            continue;
        }
        break;
    }
    if (level < 0) {
        HDF_LOGE("%s: get pin level err.", __func__);
        return HDF_FAILURE;
    }

    oldStatus = hs->hsStatus;
    switch (pdata->hsInsertType) {
        case HEADSET_IN_HIGH:
            hs->hsStatus = (level > 0) ? HEADSET_IN : HEADSET_OUT;
            break;
        case HEADSET_IN_LOW:
            hs->hsStatus = (level == 0) ? HEADSET_IN : HEADSET_OUT;
            break;
        default:
            HDF_LOGE("%s: [hsInsertType] error.", __func__);
            break;
    }
    if (oldStatus == hs->hsStatus) {
        *beChange = false;
        return HDF_SUCCESS;
    }

    *beChange = true;
    HDF_LOGI("%s: (headset in is %s)headset status is %s.", __func__,
        pdata->hsInsertType ? "high" : "low", hs->hsStatus ? "in" : "out");

    return HDF_SUCCESS;
}

static int32_t ReportCurrentState(struct HeadsetPriv *hs)
{
    struct HeadsetPdata *pdata = NULL;
    uint32_t type;

    if ((hs == NULL) || (hs->pdata == NULL)) {
        HDF_LOGE("%s: hs or pdata is NULL.", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    pdata = hs->pdata;
    if (hs->hsStatus == HEADSET_IN) {
        if (pdata->chan != NULL) {
            /* detect hook key */
            (void)HdfAddDelayedWork(&hs->workQueue, &hs->hDelayedWork[HOOK], DELAY_WORK_MS200);
        } else {
            hs->isMic = false;
            HDF_LOGD("%s, LINE = %d: isMic = %s.", __func__, __LINE__, hs->isMic ? "true" : "false");
            hs->curHsStatus = BIT_HEADSET_NO_MIC;
            (void)ExtconSetStateSync(hs, KEY_JACK_HEADPHONE, true);
            HDF_LOGD("%s: notice headset status = %d(0: NULL, 1: HEADSET, 2: HEADPHONE).", __func__, hs->curHsStatus);
        }
        type = (pdata->hsInsertType == HEADSET_IN_HIGH) ? IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING;
        irq_set_irq_type(hs->irq[HEADSET], type);
    } else {
        hs->curHsStatus = BIT_HEADSET_NULL;
        HdfCancelDelayedWorkSync(&hs->hHookWork);
        if (hs->isMic) {
            if (hs->hookStatus == HOOK_DOWN) {
                hs->hookStatus = HOOK_UP;
                InputReportKeySync(hs, HOOK_KEY_CODE, hs->hookStatus);
            }
            hs->isMic = false;
            HDF_LOGD("%s, LINE = %d: isMic = %s.", __func__, __LINE__, hs->isMic ? "true" : "false");
        }

        type = (pdata->hsInsertType == HEADSET_IN_HIGH) ? IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING;
        irq_set_irq_type(hs->irq[HEADSET], type);
        // Need judge the type, it is not always microphone.
        (void)ExtconSetStateSync(hs, KEY_JACK_HEADSET, false);
        HDF_LOGD("%s: notice headset status = %d(0: NULL, 1: HEADSET, 2:HEADPHONE).", __func__, hs->curHsStatus);
    }

    return HDF_SUCCESS;
}

static irqreturn_t HeadsetInterrupt(int irq, void *devId)
{
    int32_t ret;
    struct HeadsetPriv *hs = g_hsInfo;
    bool beChange = false;

    HDF_LOGI("%s: enter.", __func__);
    (void)devId;
    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL.", __func__);
        return -EINVAL;
    }

    disable_irq_nosync(hs->irq[HEADSET]);
    if ((hs->hsIrqWorking == BUSY) ||
        (hs->hsIrqWorking == WAIT)) {
        HDF_LOGI("%s: hsIrqWorking is BUSY or WAIT.", __func__);
        return IRQ_HANDLED;
    }

    hs->hsIrqWorking = BUSY;
    ret = CheckState(hs, &beChange);
    if (ret != HDF_SUCCESS) {
        HDF_LOGD("%s: [CheckState] failed.", __func__);
        hs->hsIrqWorking = IDLE;
        enable_irq(hs->irq[HEADSET]);
        return IRQ_HANDLED;
    }
    if (!beChange) {
        HDF_LOGD("%s: read headset io level old status == now status =%u.", __func__, hs->hsStatus);
        hs->hsIrqWorking = IDLE;
        enable_irq(hs->irq[HEADSET]);
        return IRQ_HANDLED;
    }

    (void)ReportCurrentState(hs);
    hs->hsIrqWorking = IDLE;
    enable_irq(hs->irq[HEADSET]);
    return IRQ_HANDLED;
}

#ifdef TEST_FOR_CHANGE_IRQTYPE /* not actived. */
static int32_t HeadsetChangeIrqtype(int type, unsigned int irqType)
{
    int32_t ret;

    free_irq(g_hsInfo->irq[type], NULL);

    HDF_LOGD("%s: type is %s irqtype is %s.", __func__, type ? "hook" : "headset",
        (irqType == IRQF_TRIGGER_RISING) ? "RISING" : "FALLING");
    HDF_LOGD("%s: type is %s irqtype is %s.", __func__,
        type ? "hook" : "headset", (irqType == IRQF_TRIGGER_LOW) ? "LOW" : "HIGH");
    switch (type) {
        case HEADSET:
            ret = request_threaded_irq(g_hsInfo->irq[type], NULL, HeadsetInterrupt, irqType, "headset_input", NULL);
            if (ret < 0) {
                HDF_LOGD("%s: HeadsetChangeIrqtype: request irq failed.", __func__);
            }
            break;
        default:
            ret = -EINVAL;
            break;
    }
    return ret;
}
#endif

static void HookOnceWork(void *arg)
{
    int32_t ret;
    int32_t val;
    uint32_t type;
    struct HeadsetPriv *hs = (struct HeadsetPriv *)arg;

    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL.", __func__);
        return;
    }
    ret = iio_read_channel_raw(hs->chan, &val);
    if (ret < 0) {
        HDF_LOGE("%s: read HookOnceWork adc channel() error: %d.", __func__, ret);
    } else {
        HDF_LOGD("%s: HookOnceWork read adc value: %d.", __func__, val);
    }

    if (val >= 0 && val < HOOK_LEVEL_LOW) {
        hs->isMic = false;
    } else if (val >= HOOK_LEVEL_HIGH) {
        hs->isMic = true;
        (void)HdfAddDelayedWork(&hs->workQueue, &hs->hHookWork, DELAY_WORK_MS100);
    } else {
        ; // do nothing.
    }
    HDF_LOGD("%s, LINE = %d: isMic = %s.", __func__, __LINE__, g_hsInfo->isMic ? "true" : "false");
    hs->curHsStatus = hs->isMic ? BIT_HEADSET : BIT_HEADSET_NO_MIC;

    if (hs->curHsStatus != BIT_HEADSET_NULL) {
        type = (hs->isMic) ? KEY_JACK_HEADSET : KEY_JACK_HEADPHONE;
        (void)ExtconSetStateSync(hs, type, true);
    }
    HDF_LOGD("%s: notice headset status = %d(0: NULL, 1: HEADSET, 2:HEADPHONE).", __func__, hs->curHsStatus);
}

static void HookWorkCallback(void *arg)
{
    int32_t ret;
    int32_t val;
    struct HeadsetPriv *hs = (struct HeadsetPriv *)arg;
    struct HeadsetPdata *pdata = NULL;
    static uint32_t oldStatus = HOOK_UP;
    static int32_t oldVal = -1; // Invalid initial value

    if ((hs == NULL) || (hs->pdata == NULL)) {
        HDF_LOGE("%s: hs or hs->pdata is NULL.", __func__);
        return;
    }
    pdata = hs->pdata;
    ret = iio_read_channel_raw(hs->chan, &val);
    if (ret < 0) {
        HDF_LOGE("%s: read hook adc channel() error: %d.", __func__, ret);
        return;
    }
    if ((hs->hsStatus == HEADSET_OUT) ||
        (hs->hsIrqWorking == BUSY) ||
        (hs->hsIrqWorking == WAIT) ||
        (pdata->hsInsertType ? gpio_get_value(pdata->hsGpio) == 0 : gpio_get_value(pdata->hsGpio) > 0)) {
        HDF_LOGD("%s: Headset is out or waiting for headset is in or out, after same time check HOOK key.", __func__);
        return;
    }
    oldStatus = hs->hookStatus;
    if (val < HOOK_LEVEL_LOW && val >= 0) {
        hs->hookStatus = HOOK_DOWN;
    } else if (val > HOOK_LEVEL_HIGH && val < HOOK_DEFAULT_VAL) {
        hs->hookStatus = HOOK_UP;
    } else {
        ; // do nothing.
    }
    if (oldVal != val) {
        HDF_LOGD("%s: HOOK status is %s , adc value = %d, hookTime = %u.", __func__,
            hs->hookStatus ? "down" : "up", val, hs->hookTime);
        oldVal = val;
    }
    if (oldStatus == hs->hookStatus) {
        (void)HdfAddDelayedWork(&hs->workQueue, &hs->hHookWork, DELAY_WORK_MS100);
        return;
    }
    HDF_LOGD("%s: hs->hookStatus = %s, hookTime = %u.", __func__, hs->hookStatus ? "down" : "up", hs->hookTime);
    if ((hs->hsStatus == HEADSET_OUT) ||
        (hs->hsIrqWorking == BUSY) ||
        (hs->hsIrqWorking == WAIT) ||
        (pdata->hsInsertType ? gpio_get_value(pdata->hsGpio) == 0 : gpio_get_value(pdata->hsGpio) > 0)) {
        HDF_LOGI("%s: headset is out, HOOK status must discard.", __func__);
        return;
    }
    InputReportKeySync(hs, HOOK_KEY_CODE, hs->hookStatus);
    (void)HdfAddDelayedWork(&hs->workQueue, &hs->hHookWork, DELAY_WORK_MS100);
}

static int AnalogHskeyOpen(struct input_dev *dev)
{
    (void)dev;
    return 0;
}

static void AnalogHskeyClose(struct input_dev *dev)
{
    (void)dev;
}

static const unsigned int g_hsCable[] = {
    KEY_JACK_HEADSET,
    KEY_JACK_HEADPHONE,
    EXTCON_NONE,
};

static int32_t InitWorkData(struct HeadsetPriv *hs)
{
    struct HeadsetPdata *pdata = NULL;
    if ((hs == NULL) || (hs->pdata == NULL)) {
        HDF_LOGE("%s: hs or pdata is NULL.", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    pdata = hs->pdata;
    if (HdfWorkQueueInit(&hs->workQueue, HDF_HEADSET_WORK_QUEUE_NAME) != HDF_SUCCESS) {
        HDF_LOGE("%s: Init work queue failed", __func__);
        return HDF_FAILURE;
    }
    HdfDelayedWorkInit(&hs->hDelayedWork[HOOK], HookOnceWork, hs);
    if (pdata->chan != NULL) { // this is always true.
        HdfDelayedWorkInit(&hs->hHookWork, HookWorkCallback, hs);
    }

    return HDF_SUCCESS;
}

static int32_t CreateAndRegisterInputDevice(struct platform_device *pdev, struct HeadsetPriv *hs)
{
    int32_t ret;

    HDF_LOGI("%s: enter.", __func__);
    if ((hs == NULL) || (pdev == NULL)) {
        HDF_LOGE("%s: hs or pdev is NULL.", __func__);
        return -EINVAL;
    }

    hs->inDev = devm_input_allocate_device(&pdev->dev);
    if (hs->inDev == NULL) {
        HDF_LOGE("%s: failed to allocate input device.", __func__);
        ret = -ENOMEM;
        return ret;
    }

    hs->inDev->name = pdev->name;
    hs->inDev->open = AnalogHskeyOpen;
    hs->inDev->close = AnalogHskeyClose;
    hs->inDev->dev.parent = &pdev->dev;

    hs->inDev->id.vendor = INPUT_DEVID_VENDOR;
    hs->inDev->id.product = INPUT_DEVID_PRODUCT;
    hs->inDev->id.version = INPUT_DEVID_VERSION;
    // register the input device
    ret = input_register_device(hs->inDev);
    if (ret) {
        HDF_LOGE("%s: failed to register input device.", __func__);
        return ret;
    }
    input_set_capability(hs->inDev, EV_KEY, HOOK_KEY_CODE);
    HDF_LOGI("%s: done.", __func__);

    return ret;
}

static int32_t SetHeadsetIrqEnable(struct device *dev, struct HeadsetPriv *hs)
{
    int32_t ret;
    struct HeadsetPdata *pdata = NULL;
    uint32_t irqType;

    if ((hs == NULL) || (hs->pdata == NULL) || (dev == NULL)) {
        HDF_LOGE("%s: hs, pdata or dev is NULL.", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    pdata = hs->pdata;
    if (pdata->hsGpio) {
        hs->irq[HEADSET] = gpio_to_irq(pdata->hsGpio);
        irqType = (pdata->hsInsertType == HEADSET_IN_HIGH) ? IRQF_TRIGGER_HIGH : IRQF_TRIGGER_LOW;
        irqType |= IRQF_NO_SUSPEND | IRQF_ONESHOT;
        ret = devm_request_threaded_irq(dev, hs->irq[HEADSET], NULL, HeadsetInterrupt, irqType,
            "headset_input", NULL);
        if (ret != 0) {
            HDF_LOGE("%s: failed headset adc probe ret=%d.", __func__, ret);
            return ret;
        }
        if (pdata->hsWakeup) {
            enable_irq_wake(hs->irq[HEADSET]);
        }
    } else {
        HDF_LOGE("%s: failed init headset,please full hook_io_init function in board.", __func__);
        ret = -EEXIST;
        return ret;
    }

    return 0;
}

int32_t AnalogHeadsetAdcInit(struct platform_device *pdev, struct HeadsetPdata *pdata)
{
    int32_t ret;
    struct HeadsetPriv *hs;

    HDF_LOGI("%s: enter.", __func__);
    hs = (struct HeadsetPriv *)OsalMemCalloc(sizeof(*hs));
    if (hs == NULL) {
        HDF_LOGE("%s: failed to allocate driver data.", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    g_hsInfo = hs;
    InitHeadsetPriv(hs, pdata);
    hs->edev = devm_extcon_dev_allocate(&pdev->dev, g_hsCable);
    if (IS_ERR(hs->edev)) {
        HDF_LOGE("%s: failed to allocate extcon device.", __func__);
        return-ENOMEM;
    }
    ret = devm_extcon_dev_register(&pdev->dev, hs->edev);
    if (ret < 0) {
        HDF_LOGE("%s: extcon_dev_register() failed: %d.", __func__, ret);
        return ret;
    }
    ret = InitWorkData(hs);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [InitWorkData] failed", __func__);
        return ret;
    }
    // Create and register the input driver
    ret = CreateAndRegisterInputDevice(pdev, hs);
    if (ret != 0) {
        HDF_LOGE("%s: [CreateAndRegisterInputDevice] failed", __func__);
        return ret;
    }
    ret = CreateAndRegisterHdfInputDevice((void *)hs, pdata->device);
    if (ret != 0) {
        HDF_LOGE("%s: [CreateAndRegisterHdfInputDevice] failed", __func__);
        return ret;
    }
    ret = SetHeadsetIrqEnable(&pdev->dev, hs);
    if (ret != 0) {
        HDF_LOGE("%s: [SetHeadsetIrqEnable] failed", __func__);
        return ret;
    }
    HDF_LOGI("%s: success.", __func__);
    return ret;
}

int AnalogHeadsetAdcSuspend(struct platform_device *pdev, pm_message_t state)
{
    HDF_LOGD("%s: %d enter.", __func__, __LINE__);
    (void)pdev;
    (void)state;

    return 0;
}

int AnalogHeadsetAdcResume(struct platform_device *pdev)
{
    HDF_LOGD("%s: %d enter.", __func__, __LINE__);
    (void)pdev;

    return 0;
}

void AnalogHeadsetAdcRelease(struct HeadsetPdata *pdata)
{
    struct HeadsetPriv *hs = g_hsInfo;
    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL.", __func__);
        return;
    }
    (void)pdata;
    HdfWorkDestroy(&hs->hDelayedWork[HOOK]);
    HdfCancelDelayedWorkSync(&hs->hHookWork);
    HdfWorkDestroy(&hs->hHookWork);
    HdfWorkQueueDestroy(&hs->workQueue);
    DestroyHdfInputDevice();
    OsalMemFree(hs);
    g_hsInfo = NULL;
}