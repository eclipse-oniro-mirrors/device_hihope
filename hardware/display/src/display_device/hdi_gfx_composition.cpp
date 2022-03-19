/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <cinttypes>
#include <dlfcn.h>
#include <cerrno>
#include "display_gfx.h"
#include "hdi_gfx_composition.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
int32_t HdiGfxComposition::Init(void)
{
    DISPLAY_DEBUGLOG();
    int32_t ret = GfxModuleInit();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS) || (mGfxFuncs == nullptr), DISPLAY_FAILURE,
        DISPLAY_LOGE("GfxModuleInit failed"));
    ret = mGfxFuncs->InitGfx();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("gfx init failed"));
    return DISPLAY_SUCCESS;
}

int32_t HdiGfxComposition::GfxModuleInit(void)
{
    DISPLAY_DEBUGLOG();
    mGfxModule = dlopen("libdisplay_gfx.z.so", RTLD_NOW | RTLD_NOLOAD);
    if (mGfxModule != nullptr) {
        DISPLAY_LOGI("Module '%{public}s' already loaded", "libdisplay_gfx.z.so");
    } else {
        DISPLAY_LOGI("Loading module '%{public}s'", "libdisplay_gfx.z.so");
        mGfxModule = dlopen("libdisplay_gfx.z.so", RTLD_NOW);
        if (mGfxModule == nullptr) {
            DISPLAY_LOGE("Failed to load module: %{public}s", dlerror());
            return DISPLAY_FAILURE;
        }
    }

    using InitFunc = int32_t (*)(GfxFuncs **funcs);
    InitFunc func = reinterpret_cast<InitFunc>(dlsym(mGfxModule, "GfxInitialize"));
    if (func == nullptr) {
        DISPLAY_LOGE("Failed to lookup %{public}s function: %s", "GfxInitialize", dlerror());
        dlclose(mGfxModule);
        return DISPLAY_FAILURE;
    }
    return func(&mGfxFuncs);
}

int32_t HdiGfxComposition::GfxModuleDeinit(void)
{
    DISPLAY_DEBUGLOG();
    int32_t ret = DISPLAY_SUCCESS;
    if (mGfxModule == nullptr) {
        using DeinitFunc = int32_t (*)(GfxFuncs *funcs);
        DeinitFunc func = reinterpret_cast<DeinitFunc>(dlsym(mGfxModule, "GfxUninitialize"));
        if (func == nullptr) {
            DISPLAY_LOGE("Failed to lookup %{public}s function: %s", "GfxUninitialize", dlerror());
        } else {
            ret = func(mGfxFuncs);
        }
        dlclose(mGfxModule);
    }
    return ret;
}

bool HdiGfxComposition::CanHandle(HdiLayer &hdiLayer)
{
    DISPLAY_DEBUGLOG();
    (void)hdiLayer;
    return true;
}

int32_t HdiGfxComposition::SetLayers(std::vector<HdiLayer *> &layers, HdiLayer &clientLayer)
{
    DISPLAY_DEBUGLOG("layers size %{public}zd", layers.size());
    mClientLayer = &clientLayer;
    mCompLayers.clear();
    for (auto &layer : layers) {
        if (CanHandle(*layer)) {
            if ((layer->GetCompositionType() != COMPOSITION_VIDEO) &&
                (layer->GetCompositionType() != COMPOSITION_CURSOR)) {
                layer->SetDeviceSelect(COMPOSITION_DEVICE);
            } else {
                layer->SetDeviceSelect(layer->GetCompositionType());
            }
            mCompLayers.push_back(layer);
        }
    }
    DISPLAY_DEBUGLOG("composer layers size %{public}zd", mCompLayers.size());
    return DISPLAY_SUCCESS;
}

void HdiGfxComposition::InitGfxSurface(ISurface &iSurface, HdiLayerBuffer &buffer)
{
    iSurface.width = buffer.GetWight();
    iSurface.height = buffer.GetHeight();
    iSurface.phyAddr = buffer.GetFb(); // buffer.GetPhysicalAddr();
    iSurface.enColorFmt = (PixelFormat)buffer.GetFormat();
    iSurface.stride = buffer.GetStride();
    iSurface.bAlphaExt1555 = true;
    iSurface.bAlphaMax255 = true;
    iSurface.alpha0 = 0XFF;
    iSurface.alpha1 = 0XFF;
    DISPLAY_DEBUGLOG("iSurface fd %{public}d w:%{public}d h:%{public}d addr:0x%{public}" \
        PRIx64 " fmt:%{public}d stride:%{public}d",
        buffer.GetFb(), iSurface.width, iSurface.height,
        buffer.GetPhysicalAddr(), iSurface.enColorFmt, iSurface.stride);
}

// now not handle the alpha of layer
int32_t HdiGfxComposition::BlitLayer(HdiLayer &src, HdiLayer &dst)
{
    ISurface srcSurface = { 0 };
    ISurface dstSurface = { 0 };
    GfxOpt opt = { 0 };
    DISPLAY_DEBUGLOG();
    HdiLayerBuffer *srcBuffer = src.GetCurrentBuffer();
    DISPLAY_CHK_RETURN((srcBuffer == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("the srcbuffer is null"));
    DISPLAY_DEBUGLOG("init the src iSurface");
    InitGfxSurface(srcSurface, *srcBuffer);

    HdiLayerBuffer *dstBuffer = dst.GetCurrentBuffer();
    DISPLAY_CHK_RETURN((dstBuffer == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("can not get client layer buffer"));
    DISPLAY_DEBUGLOG("init the dst iSurface");
    InitGfxSurface(dstSurface, *dstBuffer);

    opt.blendType = src.GetLayerBlenType();
    DISPLAY_DEBUGLOG("blendType %{public}d", opt.blendType);
    opt.enPixelAlpha = true;
    opt.enableScale = true;

    if (src.GetAlpha().enGlobalAlpha) { // is alpha is 0xff we not set it
        opt.enGlobalAlpha = true;
        srcSurface.alpha0 = src.GetAlpha().gAlpha;
        DISPLAY_DEBUGLOG("src alpha %{public}x", src.GetAlpha().gAlpha);
    }
    opt.rotateType = src.GetTransFormType();
    DISPLAY_DEBUGLOG(" the roate type is %{public}d", opt.rotateType);
    IRect crop = src.GetLayerCrop();
    IRect displayRect = src.GetLayerDisplayRect();
    DISPLAY_DEBUGLOG("crop x: %{public}d y : %{public}d w : %{public}d h: %{public}d", crop.x, crop.y, crop.w, crop.h);
    DISPLAY_DEBUGLOG("displayRect x: %{public}d y : %{public}d w : %{public}d h : %{public}d",
        displayRect.x, displayRect.y, displayRect.w, displayRect.h);
    DISPLAY_CHK_RETURN(mGfxFuncs == nullptr, DISPLAY_FAILURE, DISPLAY_LOGE("Blit: mGfxFuncs is null"));
    return mGfxFuncs->Blit(&srcSurface, &crop, &dstSurface, &displayRect, &opt);
}

int32_t HdiGfxComposition::ClearRect(HdiLayer &src, HdiLayer &dst)
{
    ISurface dstSurface = { 0 };
    GfxOpt opt = { 0 };
    DISPLAY_DEBUGLOG();
    HdiLayerBuffer *dstBuffer = dst.GetCurrentBuffer();
    DISPLAY_CHK_RETURN((dstBuffer == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("can not get client layer buffer"));
    InitGfxSurface(dstSurface, *dstBuffer);
    IRect rect = src.GetLayerDisplayRect();
    DISPLAY_CHK_RETURN(mGfxFuncs == nullptr, DISPLAY_FAILURE, DISPLAY_LOGE("Rect: mGfxFuncs is null"));
    return mGfxFuncs->FillRect(&dstSurface, &rect, 0, &opt);
}

int32_t HdiGfxComposition::Apply(bool modeSet)
{
    int32_t ret;
    DISPLAY_DEBUGLOG("composer layers size %{public}zd", mCompLayers.size());
    for (uint32_t i = 0; i < mCompLayers.size(); i++) {
        HdiLayer *layer = mCompLayers[i];
        CompositionType compType = layer->GetCompositionType();
        switch (compType) {
            case COMPOSITION_VIDEO:
                ret = ClearRect(*layer, *mClientLayer);
                DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE,
                    DISPLAY_LOGE("clear layer %{public}d failed", i));
                break;
            case COMPOSITION_DEVICE:
                ret = BlitLayer(*layer, *mClientLayer);
                DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE,
                    DISPLAY_LOGE("blit layer %{public}d failed ", i));
                break;
            default:
                DISPLAY_LOGE("the gfx composition can not surpport the type %{public}d", compType);
                break;
        }
    }
    return DISPLAY_SUCCESS;
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
