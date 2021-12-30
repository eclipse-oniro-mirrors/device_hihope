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

#include "drm_crtc.h"
#include "display_common.h"
#include "drm_device.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
struct PlaneMaskName planeMaskNames[] = {
    { DrmPlaneType::DRM_PLANE_TYPE_CLUSTER0_MASK, "Cluster0" },
    { DrmPlaneType::DRM_PLANE_TYPE_CLUSTER1_MASK, "Cluster1" },
    { DrmPlaneType::DRM_PLANE_TYPE_ESMART0_MASK, "Esmart0" },
    { DrmPlaneType::DRM_PLANE_TYPE_ESMART1_MASK, "Esmart1" },
    { DrmPlaneType::DRM_PLANE_TYPE_SMART0_MASK, "Smart0" },
    { DrmPlaneType::DRM_PLANE_TYPE_SMART1_MASK, "Smart1" },
    { DrmPlaneType::DRM_PLANE_TYPE_Unknown, "unknown" },
};

DrmCrtc::DrmCrtc(drmModeCrtcPtr c, uint32_t pipe) : mId(c->crtc_id), mPipe(pipe), mPlaneMask(0)
{
}

int32_t DrmCrtc::Init(DrmDevice &drmDevice)
{
    DISPLAY_DEBUGLOG();
    int32_t ret;
    DrmProperty prop;
    ret = drmDevice.GetCrtcProperty(*this, PROP_MODEID, prop);
    mModePropId = prop.propId;
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not get mode prop id"));

    ret = drmDevice.GetCrtcProperty(*this, PROP_OUTFENCE, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get out fence prop id"));
    mOutFencePropId = prop.propId;

    ret = drmDevice.GetCrtcProperty(*this, PROP_ACTIVE, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get out fence prop id"));
    mActivePropId = prop.propId;

    // Plane mask
    mPlaneMask = 0;
    ret = drmDevice.GetCrtcProperty(*this, "PLANE_MASK", prop);
    if (ret != DISPLAY_SUCCESS) {
        DISPLAY_LOGE("Failed to get plane_mask property");
    } else {
        for (int i = 0; i < static_cast<int>(ARRAY_SIZE(planeMaskNames)); i++) {
            for (auto &drmEnum : prop.enums) {
                if (!strncmp(drmEnum.name.c_str(), (const char*)planeMaskNames[i].name,
                             strlen(drmEnum.name.c_str())) && (prop.value & (1LL << drmEnum.value)) > 0) {
                    mPlaneMask |=  static_cast<int>(planeMaskNames[i].mask);
                    DISPLAY_LOGI("crtc id %{public}d, plane name %{public}s value %{public}llx",
                                 GetId(), (const char*)planeMaskNames[i].name,
                                 (long long)planeMaskNames[i].mask);
                }
            }
        }
    }
    return DISPLAY_SUCCESS;
}

int32_t DrmCrtc::BindToDisplay(uint32_t id)
{
    DISPLAY_CHK_RETURN((mDisplayId != INVALIDE_DISPLAY_ID), DISPLAY_FAILURE,
        DISPLAY_LOGE("the crtc has bind to %{public}d", mDisplayId));
    mDisplayId = id;
    return DISPLAY_SUCCESS;
}

void DrmCrtc::UnBindDisplay(uint32_t id)
{
    DISPLAY_DEBUGLOG();
    if (mDisplayId == id) {
        mDisplayId = INVALIDE_DISPLAY_ID;
    } else {
        DISPLAY_LOGE("can not unbind");
    }
}

bool DrmCrtc::CanBind()
{
    return (mDisplayId == INVALIDE_DISPLAY_ID);
}

int32_t DrmCrtc::SetActivieMode(int32_t id)
{
    DISPLAY_DEBUGLOG("set activie modeid to %{public}d", id);
    DISPLAY_CHK_RETURN((id > 0), DISPLAY_PARAM_ERR, DISPLAY_LOGE("id %{public}d is invalid ", id));
    if (mActiveModeId != id) {
        mNeedModeSet = true;
    }
    mActiveModeId = id;
    return DISPLAY_SUCCESS;
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
