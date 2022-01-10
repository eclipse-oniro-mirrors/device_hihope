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

#include "drm_plane.h"
#include "drm_device.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
struct PlaneTypeName planeTypeNames[] = {
    { DrmPlaneType::DRM_PLANE_TYPE_CLUSTER0_WIN0, "Cluster0-win0" },
    { DrmPlaneType::DRM_PLANE_TYPE_CLUSTER0_WIN1, "Cluster0-win1" },
    { DrmPlaneType::DRM_PLANE_TYPE_CLUSTER1_WIN0, "Cluster1-win0" },
    { DrmPlaneType::DRM_PLANE_TYPE_CLUSTER1_WIN1, "Cluster1-win1" },

    { DrmPlaneType::DRM_PLANE_TYPE_ESMART0_WIN0, "Esmart0-win0" },
    { DrmPlaneType::DRM_PLANE_TYPE_ESMART0_WIN1, "Esmart0-win1" },
    { DrmPlaneType::DRM_PLANE_TYPE_ESMART0_WIN2, "Esmart0-win2" },
    { DrmPlaneType::DRM_PLANE_TYPE_ESMART0_WIN3, "Esmart0-win3" },

    { DrmPlaneType::DRM_PLANE_TYPE_ESMART1_WIN0, "Esmart1-win0" },
    { DrmPlaneType::DRM_PLANE_TYPE_ESMART1_WIN1, "Esmart1-win1" },
    { DrmPlaneType::DRM_PLANE_TYPE_ESMART1_WIN2, "Esmart1-win2" },
    { DrmPlaneType::DRM_PLANE_TYPE_ESMART1_WIN3, "Esmart1-win3" },

    { DrmPlaneType::DRM_PLANE_TYPE_SMART0_WIN0, "Smart0-win0" },
    { DrmPlaneType::DRM_PLANE_TYPE_SMART0_WIN1, "Smart0-win1" },
    { DrmPlaneType::DRM_PLANE_TYPE_SMART0_WIN2, "Smart0-win2" },
    { DrmPlaneType::DRM_PLANE_TYPE_SMART0_WIN3, "Smart0-win3" },

    { DrmPlaneType::DRM_PLANE_TYPE_SMART1_WIN0, "Smart1-win0" },
    { DrmPlaneType::DRM_PLANE_TYPE_SMART1_WIN1, "Smart1-win1" },
    { DrmPlaneType::DRM_PLANE_TYPE_SMART1_WIN2, "Smart1-win2" },
    { DrmPlaneType::DRM_PLANE_TYPE_SMART1_WIN3, "Smart1-win3" },

    { DrmPlaneType::DRM_PLANE_TYPE_Unknown, "unknown" },
};

DrmPlane::DrmPlane(drmModePlane &p)
    : mId(p.plane_id), mPossibleCrtcs(p.possible_crtcs), mCrtcId(p.crtc_id),
    mFormats(p.formats, p.formats + p.count_formats)
{}

DrmPlane::~DrmPlane()
{
    DISPLAY_DEBUGLOG();
}

int DrmPlane::GetCrtcProp(DrmDevice &drmDevice)
{
    int32_t ret;
    int32_t crtc_x, crtc_y, crtc_w, crtc_h;
    DrmProperty prop;

    ret = drmDevice.GetPlaneProperty(*this, PROP_CRTC_X_ID, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane crtc_x prop id"));
    mPropCrtc_xId = prop.propId;
    crtc_x = prop.value;

    ret = drmDevice.GetPlaneProperty(*this, PROP_CRTC_Y_ID, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane crtc_y prop id"));
    mPropCrtc_yId = prop.propId;
    crtc_y = prop.value;

    ret = drmDevice.GetPlaneProperty(*this, PROP_CRTC_W_ID, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane crtc_w prop id"));
    mPropCrtc_wId = prop.propId;
    crtc_w = prop.value;

    ret = drmDevice.GetPlaneProperty(*this, PROP_CRTC_H_ID, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane crtc_h prop id"));
    mPropCrtc_hId = prop.propId;
    crtc_h = prop.value;

    DISPLAY_LOGE("plane %{public}d crtc_x %{public}d crtc_y %{public}d crtc_w %{public}d crtc_h %{public}d",
        GetId(), crtc_x, crtc_y, crtc_w, crtc_h);

    return 0;
}

int  DrmPlane::GetSrcProp(DrmDevice &drmDevice)
{
    int32_t ret;
    int32_t src_x, src_y, src_w, src_h;
    DrmProperty prop;

    ret = drmDevice.GetPlaneProperty(*this, PROP_SRC_X_ID, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane src_x prop id"));
    mPropSrc_xId = prop.propId;
    src_x = prop.value;

    ret = drmDevice.GetPlaneProperty(*this, PROP_SRC_Y_ID, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane src_y prop id"));
    mPropSrc_yId = prop.propId;
    src_y = prop.value;

    ret = drmDevice.GetPlaneProperty(*this, PROP_SRC_W_ID, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane src_w prop id"));
    mPropSrc_wId = prop.propId;
    src_w = prop.value;

    ret = drmDevice.GetPlaneProperty(*this, PROP_SRC_H_ID, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane src_h prop id"));
    mPropSrc_hId = prop.propId;
    src_h = prop.value;

    DISPLAY_LOGE("plane %{public}d src_x %{public}d src_y %{public}d src_w %{public}d src_h %{public}d",
        GetId(), src_x, src_y, src_w, src_h);

    return 0;
}

int32_t DrmPlane::Init(DrmDevice &drmDevice)
{
    DISPLAY_DEBUGLOG();
    int32_t ret;
    uint32_t find_name = 0;
    DrmProperty prop;
    GetCrtcProp(drmDevice);
    GetSrcProp(drmDevice);
    ret = drmDevice.GetPlaneProperty(*this, PROP_FBID, prop);
    mPropFbId = prop.propId;
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not get plane fb id"));
    ret = drmDevice.GetPlaneProperty(*this, PROP_IN_FENCE_FD, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get plane in fence prop id"));
    mPropFenceInId = prop.propId;
    ret = drmDevice.GetPlaneProperty(*this, PROP_CRTC_ID, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane crtc prop id"));
    mPropCrtcId = prop.propId;

    ret = drmDevice.GetPlaneProperty(*this, PROP_ZPOS_ID, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane crtc prop id"));
    mPropZposId = prop.propId;

    ret = drmDevice.GetPlaneProperty(*this, "NAME", prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane crtc prop id"));

    for (int i = 0; i < static_cast<int>ARRAY_SIZE(planeTypeNames); i++) {
        find_name = 0;

        for (auto &drmEnum : prop.enums) {
            if (!strncmp(drmEnum.name.c_str(), (const char*)planeTypeNames[i].name,
                                strlen(planeTypeNames[i].name))) {
                find_name = (1LL << drmEnum.value);
            }
        }

        if (find_name) {
            DISPLAY_LOGI("find plane id %{public}d, type %{public}x %{public}s",
                                    GetId(), planeTypeNames[i].type, planeTypeNames[i].name);
            mWinType = planeTypeNames[i].type;
            mName = planeTypeNames[i].name;
            break;
        }
    }

    ret = drmDevice.GetPlaneProperty(*this, PROP_TYPE, prop);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("cat not get pane crtc prop id"));
    switch (prop.value) {
        case DRM_PLANE_TYPE_OVERLAY:
        case DRM_PLANE_TYPE_PRIMARY:
        case DRM_PLANE_TYPE_CURSOR:
            mType = static_cast<uint32_t>(prop.value);
            break;
        default:
            DISPLAY_LOGE("unknown type value %{public}" PRIu64 "", prop.value);
            return DISPLAY_FAILURE;
    }
    return DISPLAY_SUCCESS;
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
