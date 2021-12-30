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

#ifndef DRM_PLANE_H
#define DRM_PLANE_H
#include <cinttypes>
#include <string>
#include <vector>
#include <xf86drm.h>
#include <xf86drmMode.h>

namespace OHOS {
namespace HDI {
namespace DISPLAY {
enum class DrmPropertyType {
    DRM_PROPERTY_TYPE_INT,
    DRM_PROPERTY_TYPE_ENUM,
    DRM_PROPERTY_TYPE_OBJECT,
    DRM_PROPERTY_TYPE_BLOB,
    DRM_PROPERTY_TYPE_BITMASK,
    DRM_PROPERTY_TYPE_INVALID,
};

enum class DrmPlaneType {
    DRM_PLANE_TYPE_CLUSTER0_WIN0 = 1 << 0,
    DRM_PLANE_TYPE_CLUSTER0_WIN1 = 1 << 1,

    DRM_PLANE_TYPE_CLUSTER1_WIN0 = 1 << 2,
    DRM_PLANE_TYPE_CLUSTER1_WIN1 = 1 << 3,

    DRM_PLANE_TYPE_ESMART0_WIN0 = 1 << 4,
    DRM_PLANE_TYPE_ESMART0_WIN1 = 1 << 5,
    DRM_PLANE_TYPE_ESMART0_WIN2 = 1 << 6,
    DRM_PLANE_TYPE_ESMART0_WIN3 = 1 << 7,

    DRM_PLANE_TYPE_ESMART1_WIN0 = 1 << 8,
    DRM_PLANE_TYPE_ESMART1_WIN1 = 1 << 9,
    DRM_PLANE_TYPE_ESMART1_WIN2 = 1 << 10,
    DRM_PLANE_TYPE_ESMART1_WIN3 = 1 << 11,

    DRM_PLANE_TYPE_SMART0_WIN0 = 1 << 12,
    DRM_PLANE_TYPE_SMART0_WIN1 = 1 << 13,
    DRM_PLANE_TYPE_SMART0_WIN2 = 1 << 14,
    DRM_PLANE_TYPE_SMART0_WIN3 = 1 << 15,

    DRM_PLANE_TYPE_SMART1_WIN0 = 1 << 16,
    DRM_PLANE_TYPE_SMART1_WIN1 = 1 << 17,
    DRM_PLANE_TYPE_SMART1_WIN2 = 1 << 18,
    DRM_PLANE_TYPE_SMART1_WIN3 = 1 << 19,

    DRM_PLANE_TYPE_CLUSTER0_MASK = 0x3,
    DRM_PLANE_TYPE_CLUSTER1_MASK = 0xc,
    DRM_PLANE_TYPE_CLUSTER_MASK = 0xf,
    DRM_PLANE_TYPE_ESMART0_MASK = 0xf0,
    DRM_PLANE_TYPE_ESMART1_MASK = 0xf00,
    DRM_PLANE_TYPE_SMART0_MASK  = 0xf000,
    DRM_PLANE_TYPE_SMART1_MASK  = 0xf0000,
    DRM_PLANE_TYPE_Unknown      = 0xffffffff,
};

struct PlaneMaskName {
    DrmPlaneType mask;
    const char *name;
};

struct PlaneTypeName {
    DrmPlaneType type;
    const char *name;
};

const std::string PROP_FBID = "FB_ID";
const std::string PROP_IN_FENCE_FD = "IN_FENCE_FD";
const std::string PROP_CRTC_ID = "CRTC_ID";
const std::string PROP_TYPE = "type";

const std::string PROP_CRTC_X_ID = "CRTC_X";
const std::string PROP_CRTC_Y_ID = "CRTC_Y";
const std::string PROP_CRTC_W_ID = "CRTC_W";
const std::string PROP_CRTC_H_ID = "CRTC_H";

const std::string PROP_SRC_X_ID = "SRC_X";
const std::string PROP_SRC_Y_ID = "SRC_Y";
const std::string PROP_SRC_W_ID = "SRC_W";
const std::string PROP_SRC_H_ID = "SRC_H";

const std::string PROP_ZPOS_ID = "zpos";

class DrmDevice;

class DrmPlane {
public:
    explicit DrmPlane(drmModePlane &p);
    virtual ~DrmPlane();
    int32_t Init(DrmDevice &drmDevice);
    int GetCrtcProp(DrmDevice &drmDevice);
    int GetSrcProp(DrmDevice &drmDevice);
    uint32_t GetId() const
    {
        return mId;
    }
    uint32_t GetPropFbId() const
    {
        return mPropFbId;
    }
    uint32_t GetPropCrtc_xId() const
    {
        return mPropCrtc_xId;
    }
    uint32_t GetPropCrtc_yId() const
    {
        return mPropCrtc_yId;
    }
    uint32_t GetPropCrtc_wId() const
    {
        return mPropCrtc_wId;
    }
    uint32_t GetPropCrtc_hId() const
    {
        return mPropCrtc_hId;
    }
    uint32_t GetPropSrc_xId() const
    {
        return mPropSrc_xId;
    }
    uint32_t GetPropSrc_yId() const
    {
        return mPropSrc_yId;
    }
    uint32_t GetPropSrc_wId() const
    {
        return mPropSrc_wId;
    }
    uint32_t GetPropSrc_hId() const
    {
        return mPropSrc_hId;
    }
    uint32_t GetPropZposId() const
    {
        return mPropZposId;
    }
    uint32_t GetPropFenceInId() const
    {
        return mPropFenceInId;
    }
    uint32_t GetPropCrtcId() const
    {
        return mPropCrtcId;
    }
    uint32_t GetPossibleCrtcs() const
    {
        return mPossibleCrtcs;
    }
    uint32_t GetType() const
    {
        return mType;
    }
    void BindToPipe(uint32_t pipe)
    {
        mPipe = pipe;
    }
    void UnBindPipe()
    {
        mPipe = 0;
    }
    bool IsIdle() const
    {
        return (mPipe == 0);
    }
    uint32_t GetCrtcId()
    {
        return mCrtcId;
    }
    uint32_t GetPipe()
    {
        return mPipe;
    }
    DrmPlaneType GetWinType()
    {
        return mWinType;
    }
    std::string GetName()
    {
        return mName;
    }
private:
    uint32_t mId = 0;
    uint32_t mPossibleCrtcs = 0;
    uint32_t mCrtcId = 0;
    uint32_t mPropFbId = 0;
    uint32_t mPropFenceInId = 0;
    uint32_t mPropCrtcId = 0;
    DrmPlaneType mWinType = DrmPlaneType::DRM_PLANE_TYPE_Unknown;
    std::string mName;

    uint32_t mPropCrtc_xId = 0;
    uint32_t mPropCrtc_yId = 0;
    uint32_t mPropCrtc_wId = 0;
    uint32_t mPropCrtc_hId = 0;

    uint32_t mPropSrc_xId = 0;
    uint32_t mPropSrc_yId = 0;
    uint32_t mPropSrc_wId = 0;
    uint32_t mPropSrc_hId = 0;

    uint32_t mPropZposId = 0;

    uint32_t mPipe = 0;
    uint32_t mType = 0;
    std::vector<uint32_t> mFormats;
};
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY

#endif // DRM_PLANE_H
