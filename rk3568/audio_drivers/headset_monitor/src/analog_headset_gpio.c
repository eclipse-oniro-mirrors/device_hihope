/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#include <asm/atomic.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/extcon-provider.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include "analog_headset.h"
#include "analog_headset_base.h"
#include "hdf_log.h"
#include "osal_mem.h"

#define HDF_LOG_TAG analog_headset_gpio

#define HEADSET                 0
#define HOOK                    1

#define HEADSET_IN              1
#define HEADSET_OUT             0
#define HOOK_DOWN               1
#define HOOK_UP                 0
#define ENABLE_FLAG             1
#define DISABLE_FLAG            0

/* headset private data */
struct HeadsetPriv {
    struct input_dev *inDev;
    struct HeadsetPdata *pdata;
    unsigned int hsStatus : 1;
    unsigned int hookStatus : 1;
    bool isMic;
    unsigned int ishookIrq : 1;
    int curHsStatus;
    unsigned int irq[2];
    unsigned int irqType[2];
    struct delayed_work hDelayedWork[2];
    struct extcon_dev *edev;
    struct mutex mutexLk[2];
    struct timer_list hsTimer;
    unsigned char *keycodes;
};

static struct HeadsetPriv *g_hsInfo = NULL;

#ifdef CONFIG_MODEM_MIC_SWITCH
#define HP_MIC 0
#define MAIN_MIC 1

void ModemMicSwitch(int value)
{
    struct HeadsetPriv *hs = g_hsInfo;
    struct HeadsetPdata *pdata = NULL;

    if ((hs == NULL) || (hs->pdata == NULL)) {
        HDF_LOGE("%s: hs or hs->pdata is NULL.", __func__);
        return;
    }
    pdata = hs->pdata;
    if (value == HP_MIC) {
        gpio_set_value(pdata->micSwitchGpio, pdata->hpMicIoValue);
    } else if (value == MAIN_MIC) {
        gpio_set_value(pdata->micSwitchGpio, pdata->mainMicIoValue);
    } else {
        ; // do nothing.
    }
}

void ModemMicRelease(void)
{
    struct HeadsetPriv *hs = g_hsInfo;
    struct HeadsetPdata *pdata = NULL;

    if ((hs == NULL) || (hs->pdata == NULL)) {
        HDF_LOGE("%s: hs or hs->pdata is NULL.", __func__);
        return;
    }
    pdata = hs->pdata;
    if (hs->curHsStatus == BIT_HEADSET) {
        gpio_set_value(pdata->micSwitchGpio, pdata->hpMicIoValue);
    } else {
        gpio_set_value(pdata->micSwitchGpio, pdata->mainMicIoValue);
    }
}
#endif

static int ExtconSetStateSync(struct HeadsetPriv *hs, unsigned int id, bool state)
{
    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL!", __func__);
        return -EINVAL;
    }

    extcon_set_state_sync(hs->edev, id, state);
    SetStateSync(id, state);
    HDF_LOGI("%s: id = %d, state = %s.", __func__, id, state ? "in" : "out");

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
    HDF_LOGI("%s: code = %d, value = %s.", __func__, code, value ? "in" : "out");
}

static void InitHeadsetPriv(struct HeadsetPriv *hs, struct HeadsetPdata *pdata)
{
    if ((hs == NULL) || (pdata == NULL)) {
        HDF_LOGE("%s: hs or pdata is NULL!", __func__);
        return;
    }

    hs->pdata = pdata;
    hs->hsStatus = HEADSET_OUT;
    hs->hookStatus = HOOK_UP;
    hs->ishookIrq = DISABLE_FLAG;
    hs->curHsStatus = BIT_HEADSET_NULL;
    hs->isMic = false;
}

static int ReadGpio(int gpio)
{
    int i;
    int level;

    for (i = 0; i < GET_GPIO_REPEAT_TIMES; i++) {
        level = gpio_get_value(gpio);
        if (level < 0) {
            HDF_LOGW("%s: get pin level again,pin=%d,i=%d.", __func__, gpio, i);
            msleep(IRQ_CONFIRM_MS1);
            continue;
        }
        break;
    }
    if (level < 0) {
        HDF_LOGE("%s: get pin level err.", __func__);
    }
    return level;
}

static irqreturn_t HeadsetInterrupt(int irq, void *devId)
{
    struct HeadsetPriv *hs = g_hsInfo;

    HDF_LOGD("%s: enter.", __func__);
    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL.", __func__);
        return -EINVAL;
    }

    (void)irq;
    (void)devId;
    schedule_delayed_work(&hs->hDelayedWork[HEADSET], msecs_to_jiffies(DELAY_WORK_MS50));
    return IRQ_HANDLED;
}

static irqreturn_t HookInterrupt(int irq, void *devId)
{
    struct HeadsetPriv *hs = g_hsInfo;

    HDF_LOGD("%s: enter.", __func__);
    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL.", __func__);
        return -EINVAL;
    }

    (void)irq;
    (void)devId;
    schedule_delayed_work(&hs->hDelayedWork[HOOK], msecs_to_jiffies(DELAY_WORK_MS100));
    return IRQ_HANDLED;
}

static int32_t CheckState(struct HeadsetPriv *hs, bool *beChange)
{
    int level = 0;
    int level2 = 0;
    struct HeadsetPdata *pdata = NULL;
    static unsigned int oldStatus = 0;

    HDF_LOGI("%s: enter.", __func__);
    if ((hs == NULL) || (hs->pdata == NULL) || (beChange == NULL)) {
        HDF_LOGE("%s: hs, pdata or beChange is NULL.", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    pdata = hs->pdata;

    level = ReadGpio(pdata->hsGpio);
    if (level < 0) {
        return HDF_FAILURE;
    }
    msleep(IRQ_CONFIRM_MS100);
    level2 = ReadGpio(pdata->hsGpio);
    if (level2 < 0) {
        return HDF_FAILURE;
    }
    if (level2 != level) {
        return HDF_FAILURE;
    }
    oldStatus = hs->hsStatus;
    if (pdata->hsInsertType == HEADSET_IN_HIGH) {
        hs->hsStatus = level ? HEADSET_IN : HEADSET_OUT;
    } else {
        hs->hsStatus = level ? HEADSET_OUT : HEADSET_IN;
    }

    if (oldStatus == hs->hsStatus) {
        HDF_LOGW("%s: oldStatus == hs->hsStatus.", __func__);
        *beChange = false;
        return HDF_SUCCESS;
    }
    *beChange = true;
    HDF_LOGD("%s: (headset in is %s)headset status is %s.", __func__,
        pdata->hsInsertType ? "high level" : "low level",
        hs->hsStatus ? "in" : "out");

    return HDF_SUCCESS;
}

static int32_t ReportCurrentState(struct HeadsetPriv *hs)
{
    struct HeadsetPdata *pdata = NULL;
    unsigned int type;
    bool bePlugIn;

    HDF_LOGI("%s: enter.", __func__);
    if ((hs == NULL) || (hs->pdata == NULL)) {
        HDF_LOGE("%s: hs or pdata is NULL.", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    pdata = hs->pdata;

    if (hs->hsStatus == HEADSET_IN) {
        hs->curHsStatus = BIT_HEADSET_NO_MIC;
        type = (pdata->hsInsertType == HEADSET_IN_HIGH) ? IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING;
        irq_set_irq_type(hs->irq[HEADSET], type);
        if (pdata->hookGpio) {
            /* Start the timer, wait for switch to the headphone channel */
            del_timer(&hs->hsTimer);
            hs->hsTimer.expires = jiffies + TIMER_EXPIRES_JIFFIES;
            add_timer(&hs->hsTimer);
            return HDF_SUCCESS;
        }
    } else {
        hs->hookStatus = HOOK_UP;
        if (hs->ishookIrq == ENABLE_FLAG) {
            HDF_LOGD("%s: disable hsHook irq.", __func__);
            hs->ishookIrq = DISABLE_FLAG;
            disable_irq(hs->irq[HOOK]);
        }
        hs->curHsStatus = BIT_HEADSET_NULL;
        type = (pdata->hsInsertType == HEADSET_IN_HIGH) ? IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING;
        irq_set_irq_type(hs->irq[HEADSET], type);
    }
    bePlugIn = (hs->curHsStatus != BIT_HEADSET_NULL) ? true : false;
    (void)ExtconSetStateSync(hs, EXTCON_JACK_HEADPHONE, bePlugIn);
    HDF_LOGD("%s: curHsStatus = %d.", __func__, hs->curHsStatus);

    return HDF_SUCCESS;
}

static void HeadsetObserveWork(struct work_struct *work)
{
    int32_t ret;
    struct HeadsetPriv *hs = g_hsInfo;
    bool beChange = false;

    HDF_LOGI("%s: enter.", __func__);
    (void)work;
    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL.", __func__);
        return;
    }

    mutex_lock(&hs->mutexLk[HEADSET]);
    ret = CheckState(hs, &beChange);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [CheckState] failed.", __func__);
        mutex_unlock(&hs->mutexLk[HEADSET]);
        return;
    }
    if (!beChange) {
        HDF_LOGE("%s: read headset io level old status == now status =%d.", __func__, hs->hsStatus);
        mutex_unlock(&hs->mutexLk[HEADSET]);
        return;
    }

    ret = ReportCurrentState(hs);
    if (ret != HDF_SUCCESS) {
        HDF_LOGD("%s: [ReportCurentState] failed.", __func__);
    }
    mutex_unlock(&hs->mutexLk[HEADSET]);
}

static void HookWorkCallback(struct work_struct *work)
{
    int level = 0;
    struct HeadsetPdata *pdata = g_hsInfo->pdata;
    static unsigned int oldStatus = HOOK_UP;
    unsigned int type;

    (void)work;
    mutex_lock(&g_hsInfo->mutexLk[HOOK]);
    if (g_hsInfo->hsStatus == HEADSET_OUT) {
        HDF_LOGD("%s: Headset is out.", __func__);
        mutex_unlock(&g_hsInfo->mutexLk[HOOK]);
        return;
    }
    level = ReadGpio(pdata->hookGpio);
    if (level < 0) {
        HDF_LOGD("%s: [mutex_unlock] failed.", __func__);
        mutex_unlock(&g_hsInfo->mutexLk[HOOK]);
        return;
    }
    oldStatus = g_hsInfo->hookStatus;
    HDF_LOGD("%s: Hook_work -- level = %d.", __func__, level);
    if (level == 0) {
        g_hsInfo->hookStatus = (pdata->hookDownType == HOOK_DOWN_HIGH) ? HOOK_UP : HOOK_DOWN;
    } else if (level > 0) {
        g_hsInfo->hookStatus = (pdata->hookDownType == HOOK_DOWN_HIGH) ? HOOK_DOWN : HOOK_UP;
    } else {
        ; // do nothing.
    }
    if (oldStatus == g_hsInfo->hookStatus) {
        HDF_LOGD("%s: oldStatus == g_hsInfo->hookStatus.", __func__);
        mutex_unlock(&g_hsInfo->mutexLk[HOOK]);
        return;
    }
    HDF_LOGD("%s: Hook_work -- level = %d  hook status is %s.", __func__, level,
        g_hsInfo->hookStatus ? "key down" : "key up");
    if (g_hsInfo->hookStatus == HOOK_DOWN) {
        type = (pdata->hookDownType == HOOK_DOWN_HIGH) ? IRQF_TRIGGER_FALLING : IRQF_TRIGGER_RISING;
    } else {
        type = (pdata->hookDownType == HOOK_DOWN_HIGH) ? IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING;
    }
    irq_set_irq_type(g_hsInfo->irq[HOOK], type);
    InputReportKeySync(g_hsInfo, HOOK_KEY_CODE, g_hsInfo->hookStatus);
    mutex_unlock(&g_hsInfo->mutexLk[HOOK]);
}

static void HeadsetTimerCallback(struct timer_list *t)
{
    struct HeadsetPriv *hs = from_timer(hs, t, hsTimer);
    struct HeadsetPdata *pdata = NULL;
    int level = 0;
    bool bePlugIn;
    unsigned int type;

    HDF_LOGI("%s: enter.", __func__);
    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL.", __func__);
        return;
    }

    pdata = hs->pdata;
    if (hs->hsStatus == HEADSET_OUT) {
        HDF_LOGI("%s: Headset is out.", __func__);
        return;
    }
    level = ReadGpio(pdata->hookGpio);
    if (level < 0) {
        HDF_LOGE("%s: [ReadGpio] failed.", __func__);
        return;
    }
    if ((level > 0 && pdata->hookDownType == HOOK_DOWN_LOW) ||
        (level == 0 && pdata->hookDownType == HOOK_DOWN_HIGH)) {
        hs->isMic = true;
        enable_irq(hs->irq[HOOK]);
        hs->ishookIrq = ENABLE_FLAG;
        hs->hookStatus = HOOK_UP;
        type = (pdata->hookDownType == HOOK_DOWN_HIGH) ? IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING;
        irq_set_irq_type(hs->irq[HOOK], type);
    } else {
        hs->isMic = false;
    }
    HDF_LOGI("%s: hs->isMic = %d.", __func__, hs->isMic);
    hs->curHsStatus = hs->isMic ? BIT_HEADSET : BIT_HEADSET_NO_MIC;
    bePlugIn = hs->isMic;
    (void)ExtconSetStateSync(hs, EXTCON_JACK_MICROPHONE, bePlugIn);
    HDF_LOGD("%s: hs->curHsStatus = %d.", __func__, hs->curHsStatus);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void HeadsetEarlyResume(struct early_suspend *h)
{
    schedule_delayed_work(&g_hsInfo->hDelayedWork[HEADSET], msecs_to_jiffies(DELAY_WORK_MS10));
    HDF_LOGD("%s: done.", __func__);
}

static struct early_suspend g_hsEarlySuspend;
#endif

static int AnalogHskeyOpen(struct input_dev *dev)
{
    HDF_LOGI("%s: enter.", __func__);
    (void)dev;
    return 0;
}

static void AnalogHskeyClose(struct input_dev *dev)
{
    HDF_LOGI("%s: enter.", __func__);
    (void)dev;
}

static const unsigned int g_hsCable[] = {
    EXTCON_JACK_MICROPHONE,
    EXTCON_JACK_HEADPHONE,
    EXTCON_NONE,
};

static int32_t CreateAndRegisterEdev(struct device *dev, struct HeadsetPriv *hs)
{
    int32_t ret;

    HDF_LOGI("%s: enter.", __func__);
    if ((hs == NULL) || (dev == NULL)) {
        HDF_LOGE("%s: hs or dev is NULL.", __func__);
        return -EINVAL;
    }

    hs->edev = devm_extcon_dev_allocate(dev, g_hsCable);
    if (IS_ERR(hs->edev)) {
        HDF_LOGE("%s: failed to allocate extcon device.", __func__);
        ret = -ENOMEM;
        return ret;
    }
    ret = devm_extcon_dev_register(dev, hs->edev);
    if (ret < 0) {
        HDF_LOGE("%s: extcon_dev_register() failed: %d.", __func__, ret);
        return ret;
    }

    return ret;
}

static int32_t InitWorkData(struct HeadsetPriv *hs)
{
    if (hs == NULL) {
        HDF_LOGE("%s: hs is NULL.", __func__);
        return -EINVAL;
    }

    INIT_DELAYED_WORK(&hs->hDelayedWork[HEADSET], HeadsetObserveWork);
    INIT_DELAYED_WORK(&hs->hDelayedWork[HOOK], HookWorkCallback);

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
    if (!hs->inDev) {
        HDF_LOGE("%s: failed to allocate input device.", __func__);
        return -ENOMEM;
    }
    hs->inDev->name = pdev->name;
    hs->inDev->open = AnalogHskeyOpen;
    hs->inDev->close = AnalogHskeyClose;
    hs->inDev->dev.parent = &pdev->dev;

    hs->inDev->id.vendor = INPUT_DEVID_VENDOR;
    hs->inDev->id.product = INPUT_DEVID_PRODUCT;
    hs->inDev->id.version = INPUT_DEVID_VERSION;
    /* Register the input device */
    ret = input_register_device(hs->inDev);
    if (ret) {
        HDF_LOGE("%s: failed to register input device.", __func__);
        return ret;
    }
    input_set_capability(hs->inDev, EV_KEY, HOOK_KEY_CODE);

    return ret;
}

static int32_t SetHeadsetIrqEnable(struct device *dev, struct HeadsetPriv *hs)
{
    int32_t ret;
    struct HeadsetPdata *pdata = NULL;
    unsigned int irqType;

    if ((hs == NULL) || (hs->pdata == NULL) || (dev == NULL)) {
        HDF_LOGE("%s: hs, pdata or dev is NULL.", __func__);
        return -EINVAL;
    }

    pdata = hs->pdata;
    if (pdata->hsGpio) {
        hs->irq[HEADSET] = gpio_to_irq(pdata->hsGpio);
        irqType = (pdata->hsInsertType == HEADSET_IN_HIGH) ? IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING;
        hs->irqType[HEADSET] = irqType;
        ret = devm_request_irq(dev, hs->irq[HEADSET], HeadsetInterrupt, irqType, "headset_input", NULL);
        if (ret) {
            HDF_LOGE("%s: [devm_request_irq] failed.", __func__);
            return ret;
        }
        if (pdata->hsWakeup) {
            enable_irq_wake(hs->irq[HEADSET]);
        }
    } else {
        HDF_LOGE("%s: failed init headset, please full hsGpio function in board.", __func__);
        return -EEXIST;
    }
    if (pdata->hookGpio) {
        hs->irq[HOOK] = gpio_to_irq(pdata->hookGpio);
        irqType = (pdata->hookDownType == HOOK_DOWN_HIGH) ? IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING;
        hs->irqType[HOOK] = irqType;
        ret = devm_request_irq(dev, hs->irq[HOOK], HookInterrupt, irqType, "headset_hook", NULL);
        if (ret) {
            HDF_LOGE("%s: [devm_request_irq] failed.", __func__);
            return ret;
        }
        disable_irq(hs->irq[HOOK]);
    }

    return 0;
}

int AnalogHeadsetGpioInit(struct platform_device *pdev, struct HeadsetPdata *pdata)
{
    int ret;
    struct HeadsetPriv *hs;

    HDF_LOGI("%s: enter.", __func__);
    hs = devm_kzalloc(&pdev->dev, sizeof(*hs), GFP_KERNEL);
    if (hs == NULL) {
        HDF_LOGE("%s: failed to allocate driver data.", __func__);
        return -ENOMEM;
    }
    InitHeadsetPriv(hs, pdata);
    g_hsInfo = hs;
    ret = CreateAndRegisterEdev(&pdev->dev, hs);
    if (ret < 0) {
        HDF_LOGE("%s: [CreateAndRegisterEdev] failed.", __func__);
        return ret;
    }
    mutex_init(&hs->mutexLk[HEADSET]);
    mutex_init(&hs->mutexLk[HOOK]);
    ret = InitWorkData(hs);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: [InitWorkData] failed", __func__);
        return ret;
    }
    timer_setup(&hs->hsTimer, HeadsetTimerCallback, 0);
    /* Create and register the input driver */
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
#ifdef CONFIG_HAS_EARLYSUSPEND
    g_hsEarlySuspend.suspend = NULL;
    g_hsEarlySuspend.resume = HeadsetEarlyResume;
    g_hsEarlySuspend.level = ~0x0;
    register_early_suspend(&g_hsEarlySuspend);
#endif
    ret = SetHeadsetIrqEnable(&pdev->dev, hs);
    if (ret != 0) {
        HDF_LOGE("%s: [SetHeadsetIrqEnable] failed", __func__);
        return ret;
    }
    schedule_delayed_work(&hs->hDelayedWork[HEADSET], msecs_to_jiffies(DELAY_WORK_MS500));
    HDF_LOGI("%s: success.", __func__);
    return 0;
}
