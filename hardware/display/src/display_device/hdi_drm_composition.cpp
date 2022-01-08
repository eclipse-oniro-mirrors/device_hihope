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

#include "hdi_drm_composition.h"
#include <cerrno>
#include "hdi_drm_layer.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
HdiDrmComposition::HdiDrmComposition(std::shared_ptr<DrmConnector> connector, std::shared_ptr<DrmCrtc> crtc,
    std::shared_ptr<DrmDevice> drmDevice)
    : mDrmDevice(drmDevice), mConnector(connector), mCrtc(crtc)
{
    DISPLAY_DEBUGLOG();
}

int32_t HdiDrmComposition::Init()
{
    DISPLAY_DEBUGLOG();
    mPrimPlanes.clear();
    mOverlayPlanes.clear();
    mPlanes.clear();
    DISPLAY_CHK_RETURN((mCrtc == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("crtc is null"));
    DISPLAY_CHK_RETURN((mConnector == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("connector is null"));
    DISPLAY_CHK_RETURN((mDrmDevice == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("drmDevice is null"));
    mPrimPlanes = mDrmDevice->GetDrmPlane(mCrtc->GetPipe(), DRM_PLANE_TYPE_PRIMARY);
    mOverlayPlanes = mDrmDevice->GetDrmPlane(mCrtc->GetPipe(), DRM_PLANE_TYPE_OVERLAY);
    mPlanes.insert(mPlanes.end(), mPrimPlanes.begin(), mPrimPlanes.end());
    mPlanes.insert(mPlanes.end(), mOverlayPlanes.begin(), mOverlayPlanes.end());
    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::SetLayers(std::vector<HdiLayer *> &layers, HdiLayer &clientLayer)
{
    // now we do not surpport present direct
    DISPLAY_DEBUGLOG();
    mCompLayers.clear();
    mCompLayers.push_back(&clientLayer);
    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::SetCrtcProperty(DrmPlane &drmPlane,
                                           drmModeAtomicReqPtr pset,
                                           int32_t bufferW,
                                           int32_t bufferH)
{
    int ret;

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropCrtc_xId(), 0);
    DISPLAY_LOGI("set the fb planeid %{public}d, GetPropCrtc_xId %{public}d, crop.x %{public}d", drmPlane.GetId(),
        drmPlane.GetPropCrtc_xId(), 0);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropCrtc_yId(), 0);
    DISPLAY_LOGI("set the fb planeid %{public}d, GetPropCrtc_yId %{public}d, crop.y %{public}d", drmPlane.GetId(),
        drmPlane.GetPropCrtc_yId(), 0);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropCrtc_wId(), bufferW);
    DISPLAY_LOGI("set the fb planeid %{public}d, GetPropCrtc_wId %{public}d, crop.w %{public}d", drmPlane.GetId(),
        drmPlane.GetPropCrtc_wId(), bufferW);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropCrtc_hId(), bufferH);
    DISPLAY_LOGI("set the fb planeid %{public}d, GetPropCrtc_hId %{public}d, crop.h %{public}d", drmPlane.GetId(),
        drmPlane.GetPropCrtc_xId(), bufferH);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::SetSrcProperty(DrmPlane &drmPlane,
                                          drmModeAtomicReqPtr pset,
                                          int32_t bufferW,
                                          int32_t bufferH)
{
    int ret;

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropSrc_xId(), 0<<16); // 16:shift left 16 bits
    DISPLAY_LOGI("set the fb planeid %{public}d, GetPropSrc_xId %{public}d, displayRect.x %{public}d", drmPlane.GetId(),
        drmPlane.GetPropSrc_xId(), 0);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropSrc_yId(), 0<<16); // 16:shift left 16 bits
    DISPLAY_LOGI("set the fb planeid %{public}d, GetPropSrc_yId %{public}d, displayRect.y %{public}d", drmPlane.GetId(),
        drmPlane.GetPropSrc_yId(), 0);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropSrc_wId(),
                                   bufferW<<16); // 16:shift left 16 bits
    DISPLAY_LOGI("set the fb planeid %{public}d, GetPropCrtc_wId %{public}d, displayRect.w %{public}d",
        drmPlane.GetId(), drmPlane.GetPropSrc_wId(), bufferW);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropSrc_hId(),
        bufferH<<16); // 16:shift left 16 bits
    DISPLAY_LOGI("set the fb planeid %{public}d, GetPropSrc_hId %{public}d, displayRect.h %{public}d", drmPlane.GetId(),
        drmPlane.GetPropSrc_hId(), bufferH);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the fb planeid fialed errno : %{public}d", errno));

    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::ApplyPlane(HdiDrmLayer &layer,
                                      HdiLayer &hlayer,
                                      DrmPlane &drmPlane,
                                      drmModeAtomicReqPtr pset)
{
    int ret;
    int fenceFd = layer.GetAcquireFenceFd();
    int propId = drmPlane.GetPropFenceInId();
    HdiLayerBuffer *layerBuffer = hlayer.GetCurrentBuffer();
    int32_t bufferW = layerBuffer->GetWight();
    int32_t bufferH = layerBuffer->GetHeight();

    DISPLAY_DEBUGLOG();
    if (propId != 0) {
        DISPLAY_LOGI("set the fence in prop");
        if (fenceFd >= 0) {
            ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), propId, fenceFd);
            DISPLAY_DEBUGLOG("set the IfenceProp plane id %{public}d, propId %{public}d, fenceFd %{public}d",
                drmPlane.GetId(), propId, fenceFd);
            DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set IN_FENCE_FD failed"));
        }
    }

    ret = SetCrtcProperty(drmPlane, pset, bufferW, bufferH);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set Crtc fialed errno : %{public}d", errno));

    ret = SetSrcProperty(drmPlane, pset, bufferW, bufferH);
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set Src fialed errno : %{public}d", errno));

    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropZposId(), layer.GetZorder());
    DISPLAY_LOGI("set the fb planeid %{public}d, GetPropZposId %{public}d, zpos %{public}d", drmPlane.GetId(),
        drmPlane.GetPropZposId(), layer.GetZorder());
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the zpos fialed errno : %{public}d", errno));

    // set fb id
    DrmGemBuffer *gemBuffer = layer.GetGemBuffer();
    DISPLAY_CHK_RETURN((gemBuffer == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("current gemBuffer is nullptr"));
    DISPLAY_CHK_RETURN((!gemBuffer->IsValid()), DISPLAY_FAILURE, DISPLAY_LOGE("the DrmGemBuffer is invalid"));
    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropFbId(), gemBuffer->GetFbId());
    DISPLAY_DEBUGLOG("set the fb planeid %{public}d, propId %{public}d, fbId %{public}d", drmPlane.GetId(),
        drmPlane.GetPropFbId(), gemBuffer->GetFbId());
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set fb id fialed errno : %{public}d", errno));

    // set crtc id
    ret = drmModeAtomicAddProperty(pset, drmPlane.GetId(), drmPlane.GetPropCrtcId(), mCrtc->GetId());
    DISPLAY_DEBUGLOG("set the crtc planeId %{public}d, propId %{public}d, crtcId %{public}d", drmPlane.GetId(),
        drmPlane.GetPropCrtcId(), mCrtc->GetId());
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set crtc id fialed errno : %{public}d", errno));
    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::UpdateMode(std::unique_ptr<DrmModeBlock> &modeBlock, drmModeAtomicReq &pset)
{
    // set the mode
    DISPLAY_LOGI();
    if (mCrtc->NeedModeSet()) {
        modeBlock = mConnector->GetModeBlockFromId(mCrtc->GetActiveModeId());
        if ((modeBlock != nullptr) && (modeBlock->GetBlockId() != DRM_INVALID_ID)) {
            // set to active
            DISPLAY_DEBUGLOG("set crtc to active");
            int ret = drmModeAtomicAddProperty(&pset, mCrtc->GetId(), mCrtc->GetActivePropId(), 1);
            DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE,
                DISPLAY_LOGE("can not add the active prop errno %{public}d", errno));

            // set the mode id
            DISPLAY_DEBUGLOG("set the mode");
            ret = drmModeAtomicAddProperty(&pset, mCrtc->GetId(), mCrtc->GetModePropId(), modeBlock->GetBlockId());
            DISPLAY_LOGI("set the mode planeId %{public}d, propId %{public}d, GetBlockId: %{public}d", mCrtc->GetId(),
                mCrtc->GetModePropId(), modeBlock->GetBlockId());
            DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE,
                DISPLAY_LOGE("can not add the mode prop errno %{public}d", errno));
            ret = drmModeAtomicAddProperty(&pset, mConnector->GetId(), mConnector->GetPropCrtcId(), mCrtc->GetId());
            DISPLAY_LOGI("set the connector id: %{public}d, propId %{public}d, crtcId %{public}d", mConnector->GetId(),
                mConnector->GetPropCrtcId(), mCrtc->GetId());
            DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE,
                DISPLAY_LOGE("can not add the crtc id prop %{public}d", errno));
        }
    }
    return DISPLAY_SUCCESS;
}

int32_t HdiDrmComposition::Apply(bool modeSet)
{
    uint64_t crtcOutFence = -1;
    int ret;
    std::unique_ptr<DrmModeBlock> modeBlock;
    int drmFd = mDrmDevice->GetDrmFd();
    
    DISPLAY_LOGI("mPlane size: %{public}zd mCompLayers size: %{public}zd", mPlanes.size(), mCompLayers.size());
    DISPLAY_CHK_RETURN((mPlanes.size() < mCompLayers.size()), DISPLAY_FAILURE, DISPLAY_LOGE("plane not enough"));
    drmModeAtomicReqPtr pset = drmModeAtomicAlloc();
    DISPLAY_CHK_RETURN((pset == nullptr), DISPLAY_NULL_PTR,
        DISPLAY_LOGE("drm atomic alloc failed errno %{public}d", errno));
    AtomicReqPtr atomicReqPtr = AtomicReqPtr(pset);

    // set the outFence property
    ret = drmModeAtomicAddProperty(atomicReqPtr.Get(), mCrtc->GetId(), mCrtc->GetOutFencePropId(),
        (uint64_t)&crtcOutFence);

    DISPLAY_DEBUGLOG("Apply Set OutFence crtc id: %{public}d, fencePropId %{public}d", mCrtc->GetId(),
        mCrtc->GetOutFencePropId());
    DISPLAY_CHK_RETURN((ret < 0), DISPLAY_FAILURE, DISPLAY_LOGE("set the outfence property of crtc failed "));

    // set the plane info.
    DISPLAY_LOGI("mCompLayers size %{public}zd", mCompLayers.size());
    DISPLAY_LOGI("crtc id %{public}d connect id %{public}d encoder id %{public}d", mCrtc->GetId(),
        mConnector->GetId(), mConnector->GetEncoderId());

    /*  Bind the plane not used by other crtcs to the crtc. */
    for (uint32_t i = 0; i < mCompLayers.size(); i++) {
        HdiDrmLayer *layer = static_cast<HdiDrmLayer *>(mCompLayers[i]);
        HdiLayer *hlayer = mCompLayers[i];
        for (uint32_t j = 0; j < mPlanes.size(); j++) {
            auto &drmPlane = mPlanes[j];
            if (drmPlane->GetPipe() != 0 && drmPlane->GetPipe() != (1 << mCrtc->GetPipe())) {
                DISPLAY_LOGI("plane %{public}d used pipe %{public}d crtc pipe %{public}d", drmPlane->GetId(),
                    drmPlane->GetPipe(), mCrtc->GetPipe());
                continue;
            }
            if (drmPlane->GetCrtcId() == mCrtc->GetId() || drmPlane->GetCrtcId() == 0) {
                ret = ApplyPlane(*layer, *hlayer, *drmPlane, atomicReqPtr.Get());
                if (ret != DISPLAY_SUCCESS) {
                    DISPLAY_LOGE("apply plane failed");
                    break;
                }
                /* mark the plane is used by crtc */
                drmPlane->BindToPipe(1 << mCrtc->GetPipe());
                break;
            }
        }
    }
    ret = UpdateMode(modeBlock, *(atomicReqPtr.Get()));
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("update mode failed"));
    uint32_t flags = DRM_MODE_ATOMIC_ALLOW_MODESET;

    ret = drmModeAtomicCommit(drmFd, atomicReqPtr.Get(), flags, nullptr);
    DISPLAY_CHK_RETURN((ret != 0), DISPLAY_FAILURE,
        DISPLAY_LOGE("drmModeAtomicCommit failed %{public}d errno %{public}d", ret, errno));
    // set the release fence
    for (auto layer : mCompLayers) {
        layer->SetReleaseFence(static_cast<int>(crtcOutFence));
    }

    return DISPLAY_SUCCESS;
}
} // OHOS
} // HDI
} // DISPLAY
