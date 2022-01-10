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

#include "hdi_layer.h"
#include <unistd.h>
#include <errno.h>

namespace OHOS {
namespace HDI {
namespace DISPLAY {
uint32_t HdiLayer::mIdleId = 0;
std::unordered_set<uint32_t> HdiLayer::mIdSets;

HdiLayerBuffer::HdiLayerBuffer(const BufferHandle &hdl)
    : mPhyAddr(hdl.phyAddr), mHeight(hdl.height), mWidth(hdl.width), mStride(hdl.stride), mFormat(hdl.format)
{
    DISPLAY_DEBUGLOG();
    mFd = dup(hdl.fd);
    mHandle = hdl;
    if (mFd < 0) {
        DISPLAY_LOGE("the fd : %{public}d dup failed errno  %{public}d", hdl.fd, errno);
    }
}

HdiLayerBuffer::~HdiLayerBuffer()
{
    DISPLAY_DEBUGLOG();
    if (mFd >= 0) {
        close(mFd);
    }
}

HdiLayerBuffer &HdiLayerBuffer::operator = (const BufferHandle &right)
{
    DISPLAY_DEBUGLOG();
    if (mFd >= 0) {
        close(mFd);
    }
    mFd = dup(right.fd);
    mPhyAddr = right.phyAddr;
    mWidth = right.width;
    mHeight = right.height;
    mStride = right.stride;
    mFormat = right.format;
    return *this;
}

uint32_t HdiLayer::GetIdleId()
{
    const uint32_t oldIdleId = mIdleId;
    uint32_t id = INVALIDE_LAYER_ID;
    // ensure the mIdleId not INVALIDE_LAYER_ID
    mIdleId = mIdleId % INVALIDE_LAYER_ID;
    do {
        auto iter = mIdSets.find(mIdleId);
        if (iter == mIdSets.end()) {
            id = mIdleId;
            break;
        }
        mIdleId = (mIdleId + 1) % INVALIDE_LAYER_ID;
    } while (oldIdleId != mIdleId);
    mIdSets.emplace(id);
    mIdleId++;
    DISPLAY_DEBUGLOG("id %{public}d mIdleId %{public}d", id, mIdleId);
    return id;
}

int32_t HdiLayer::Init()
{
    DISPLAY_DEBUGLOG();
    uint32_t id = GetIdleId();
    DISPLAY_CHK_RETURN((id == INVALIDE_LAYER_ID), DISPLAY_FAILURE, DISPLAY_LOGE("have no id to used"));
    mId = id;
    return DISPLAY_SUCCESS;
}

int32_t HdiLayer::SetLayerSize(IRect *rect)
{
    DISPLAY_CHK_RETURN((rect == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("in rect is nullptr"));
    DISPLAY_DEBUGLOG(" displayRect x: %{public}d y : %{public}d w : %{public}d h : %{public}d", rect->x, rect->y, rect->w,
        rect->h);
    mDisplayRect = *rect;
    return DISPLAY_SUCCESS;
}

int32_t HdiLayer::SetLayerCrop(IRect *rect)
{
    DISPLAY_CHK_RETURN((rect == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("in rect is nullptr"));
    DISPLAY_DEBUGLOG("id : %{public}d crop x: %{public}d y : %{public}d w : %{public}d h : %{public}d", mId, rect->x,
        rect->y, rect->w, rect->h);
    mCrop = *rect;
    return DISPLAY_SUCCESS;
}

void HdiLayer::SetLayerZorder(uint32_t zorder)
{
    DISPLAY_DEBUGLOG("id : %{public}d zorder : %{public}d ", mId, zorder);
    mZorder = zorder;
}

int32_t HdiLayer::SetLayerPreMulti(bool preMul)
{
    DISPLAY_DEBUGLOG();
    mPreMul = preMul;
    return DISPLAY_SUCCESS;
}

int32_t HdiLayer::SetLayerAlpha(LayerAlpha *alpha)
{
    DISPLAY_CHK_RETURN((alpha == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("in alpha is nullptr"));
    DISPLAY_DEBUGLOG("enable alpha %{public}d galpha 0x%{public}x", alpha->enGlobalAlpha, alpha->gAlpha);
    mAlpha = *alpha;
    return DISPLAY_SUCCESS;
}

int32_t HdiLayer::SetTransformMode(TransformType type)
{
    DISPLAY_DEBUGLOG("TransformType %{public}d", type);
    mTransformType = type;
    return DISPLAY_SUCCESS;
}

int32_t HdiLayer::SetLayerDirtyRegion(IRect *region)
{
    DISPLAY_CHK_RETURN((region == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("the in rect is null"));
    DISPLAY_DEBUGLOG("id : %{public}d DirtyRegion x: %{public}d y : %{public}d w : %{public}d h : %{public}d", mId,
        region->x, region->y, region->w, region->h);
    return DISPLAY_SUCCESS;
}

int32_t HdiLayer::SetLayerVisibleRegion(uint32_t num, IRect *rect)
{
    DISPLAY_DEBUGLOG("id : %{public}d DirtyRegion x: %{public}d y : %{public}d w : %{public}d h : %{public}d", mId, rect->x,
        rect->y, rect->w, rect->h);
    return DISPLAY_SUCCESS;
}

int32_t HdiLayer::SetLayerBuffer(const BufferHandle *buffer, int32_t fence)
{
    DISPLAY_DEBUGLOG();
    DISPLAY_CHK_RETURN((buffer == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("buffer is nullptr"));
    std::unique_ptr<HdiLayerBuffer> layerbuffer = std::make_unique<HdiLayerBuffer>(*buffer);
    mHdiBuffer = std::move(layerbuffer);
    mAcquireFence = fence;
    return DISPLAY_SUCCESS;
}

int32_t HdiLayer::SetLayerCompositionType(CompositionType type)
{
    DISPLAY_DEBUGLOG("CompositionType type %{public}d", type);
    mCompositionType = type;
    return DISPLAY_SUCCESS;
}

int32_t HdiLayer::SetLayerBlendType(BlendType type)
{
    DISPLAY_DEBUGLOG("BlendType type %{public}d", type);
    mBlendType = type;
    return DISPLAY_SUCCESS;
}

void HdiLayer::SetPixel(const BufferHandle &handle, int x, int y, uint32_t color)
{
    const int32_t pixelBytes = 4;
    DISPLAY_CHK_RETURN_NOT_VALUE((handle.format <= 0),
        DISPLAY_LOGE("CheckPixel do not support format %{public}d", handle.format));
    DISPLAY_CHK_RETURN_NOT_VALUE((handle.virAddr == nullptr), DISPLAY_LOGE("CheckPixel viraddr is null must map it"));
    DISPLAY_CHK_RETURN_NOT_VALUE((x < 0 || x >= handle.width),
        DISPLAY_LOGE("CheckPixel invalid parameter x:%{public}d width:%{public}d", x, handle.width));
    DISPLAY_CHK_RETURN_NOT_VALUE((y < 0 || y >= handle.height),
        DISPLAY_LOGE("CheckPixel invalid parameter y:%{public}d height:%{public}d", y, handle.height));
    int32_t position = y * handle.width + x;
    if ((position * pixelBytes) > handle.size) {
        DISPLAY_LOGE("the pixel postion outside\n");
    }
    uint32_t *pixel = reinterpret_cast<uint32_t *>(handle.virAddr) + position;
    *pixel = color;
}

void HdiLayer::ClearColor(uint32_t color)
{
    DISPLAY_DEBUGLOG();
    BufferHandle &handle = mHdiBuffer->mHandle;
    for (int32_t x = 0; x < handle.width; x++) {
        for (int32_t y = 0; y < handle.height; y++) {
            SetPixel(handle, x, y, color);
        }
    }
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
