/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "rk_codec_node.h"
#include <securec.h>

namespace OHOS::Camera {
uint32_t RKCodecNode::previewWidth_ = 0;
uint32_t RKCodecNode::previewHeight_ = 0;
const unsigned long long TIME_CONVERSION_NS_S = 1000000000ULL; /* ns to s */

RKCodecNode::RKCodecNode(const std::string& name, const std::string& type) : NodeBase(name, type)
{
    CAMERA_LOGV("%{public}s enter, type(%{public}s)\n", name_.c_str(), type_.c_str());
}

RKCodecNode::~RKCodecNode()
{
    CAMERA_LOGI("~RKCodecNode Node exit.");
}

RetCode RKCodecNode::Start(const int32_t streamId)
{
    CAMERA_LOGI("RKCodecNode::Start streamId = %{public}d\n", streamId);
    return RC_OK;
}

RetCode RKCodecNode::Stop(const int32_t streamId)
{
    CAMERA_LOGI("RKCodecNode::Stop streamId = %{public}d\n", streamId);

    if (halCtx_ != nullptr) {
        CAMERA_LOGI("RKCodecNode::Stop hal_mpp_ctx_delete\n");
        hal_mpp_ctx_delete(halCtx_);
        halCtx_ = nullptr;
        mppStatus_ = 0;
    }

    return RC_OK;
}

RetCode RKCodecNode::Flush(const int32_t streamId)
{
    CAMERA_LOGI("RKCodecNode::Flush streamId = %{public}d\n", streamId);
    return RC_OK;
}

void RKCodecNode::encodeJpegToMemory(unsigned char* image, int width, int height,
    const char* comment, size_t* jpegSize, unsigned char** jpegBuf)
{
    struct jpeg_compress_struct cInfo;
    struct jpeg_error_mgr jErr;
    JSAMPROW row_pointer[1];
    int row_stride = 0;
    constexpr uint32_t colorMap = 3;
    constexpr uint32_t compressionRatio = 100;
    constexpr uint32_t pixelsThick = 3;

    cInfo.err = jpeg_std_error(&jErr);

    jpeg_create_compress(&cInfo);
    cInfo.image_width = width;
    cInfo.image_height = height;
    cInfo.input_components = colorMap;
    cInfo.in_color_space = JCS_RGB;

    jpeg_set_defaults(&cInfo);
    jpeg_set_quality(&cInfo, compressionRatio, TRUE);
    jpeg_mem_dest(&cInfo, jpegBuf, jpegSize);
    jpeg_start_compress(&cInfo, TRUE);

    if (comment) {
        jpeg_write_marker(&cInfo, JPEG_COM, (const JOCTET*)comment, strlen(comment));
    }

    row_stride = width;
    while (cInfo.next_scanline < cInfo.image_height) {
        row_pointer[0] = &image[cInfo.next_scanline * row_stride * pixelsThick];
        jpeg_write_scanlines(&cInfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cInfo);
    jpeg_destroy_compress(&cInfo);
}

int RKCodecNode::findStartCode(unsigned char *data, size_t dataSz)
{
    constexpr uint32_t dataSize = 4;
    constexpr uint32_t dataBit2 = 2;
    constexpr uint32_t dataBit3 = 3;

    if (data == nullptr) {
        CAMERA_LOGI("RKCodecNode::findStartCode paramater == nullptr");
        return -1;
    }

    if ((dataSz > dataSize) && (data[0] == 0) && (data[1] == 0) && \
        (data[dataBit2] == 0) && (data[dataBit3] == 1)) {
        return 4; // 4:start node
    }

    return -1;
}

static constexpr uint32_t nalBit = 0x1F;
#define NAL_TYPE(value)             ((value) & nalBit)
void RKCodecNode::SerchIFps(unsigned char* buf, size_t bufSize, std::shared_ptr<IBuffer>& buffer)
{
    size_t nalType = 0;
    size_t idx = 0;
    size_t size = bufSize;
    constexpr uint32_t nalTypeValue = 0x05;

    if (buffer == nullptr || buf == nullptr) {
        CAMERA_LOGI("RKCodecNode::SerchIFps paramater == nullptr");
        return;
    }

    for (int i = 0; i < bufSize; i++) {
        int ret = findStartCode(buf + idx, size);
        if (ret == -1) {
            idx += 1;
            size -= 1;
        } else {
            nalType = NAL_TYPE(buf[idx + ret]);
            CAMERA_LOGI("ForkNode::ForkBuffers nalu == 0x%{public}x buf == 0x%{public}x \n", nalType, buf[idx + ret]);
            if (nalType == nalTypeValue) {
                buffer->SetEsKeyFrame(1);
                CAMERA_LOGI("ForkNode::ForkBuffers SetEsKeyFrame == 1 nalu == 0x%{public}x\n", nalType);
                break;
            } else {
                idx += ret;
                size -= ret;
            }
        }

        if (idx >= bufSize) {
            break;
        }
    }

    if (idx >= bufSize) {
        buffer->SetEsKeyFrame(0);
        CAMERA_LOGI("ForkNode::ForkBuffers SetEsKeyFrame == 0 nalu == 0x%{public}x idx = %{public}d\n",
            nalType, idx);
    }
}

void RKCodecNode::Yuv420ToRGBA8888(std::shared_ptr<IBuffer>& buffer)
{
    if (buffer == nullptr) {
        CAMERA_LOGI("RKCodecNode::Yuv420ToRGBA8888 buffer == nullptr");
        return;
    }

    int dma_fd = buffer->GetFileDescriptor();
    void* temp = malloc(buffer->GetSize());
    if (temp == nullptr) {
        CAMERA_LOGI("RKCodecNode::Yuv420ToRGBA8888 malloc buffer == nullptr");
        return;
    }

    previewWidth_ = buffer->GetWidth();
    previewHeight_ = buffer->GetHeight();
    int ret = memcpy_s(temp, buffer->GetSize(), (const void *)buffer->GetVirAddress(), buffer->GetSize());
    if (ret != 0) {
        printf("memcpy_s failed!\n");
    }
    RockchipRga rkRga;

    rga_info_t src = {};
    rga_info_t dst = {};

    src.fd = -1;
    src.mmuFlag = 1;
    src.rotation = 0;
    src.virAddr = (void *)temp;

    dst.fd = dma_fd;
    dst.mmuFlag = 1;
    dst.virAddr = 0;

    rga_set_rect(&src.rect, 0, 0, buffer->GetWidth(), buffer->GetHeight(),
        buffer->GetWidth(), buffer->GetHeight(), RK_FORMAT_YCbCr_420_P);
    rga_set_rect(&dst.rect, 0, 0, buffer->GetWidth(), buffer->GetHeight(),
        buffer->GetWidth(), buffer->GetHeight(), RK_FORMAT_RGBA_8888);

    rkRga.RkRgaBlit(&src, &dst, NULL);
    rkRga.RkRgaFlush();
    free(temp);
}

void RKCodecNode::Yuv420ToJpeg(std::shared_ptr<IBuffer>& buffer)
{
    constexpr uint32_t RGB24Width = 3;

    if (buffer == nullptr) {
        CAMERA_LOGI("RKCodecNode::Yuv420ToJpeg buffer == nullptr");
        return;
    }

    int dma_fd = buffer->GetFileDescriptor();
    unsigned char* jBuf = nullptr;
    size_t jpegSize = 0;
    uint32_t tempSize = (previewWidth_ * previewHeight_ * RGB24Width);

    void* temp = malloc(tempSize);
    if (temp == nullptr) {
        CAMERA_LOGI("RKCodecNode::Yuv420ToJpeg malloc buffer == nullptr");
        return;
    }

    RockchipRga rkRga;
    rga_info_t src = {};
    rga_info_t dst = {};

    src.mmuFlag = 1;
    src.rotation = 0;
    src.virAddr = 0;
    src.fd = dma_fd;

    dst.fd = -1;
    dst.mmuFlag = 1;
    dst.virAddr = temp;

    rga_set_rect(&src.rect, 0, 0, previewWidth_, previewHeight_,
        previewWidth_, previewHeight_, RK_FORMAT_YCbCr_420_P);
    rga_set_rect(&dst.rect, 0, 0, previewWidth_, previewHeight_,
        previewWidth_, previewHeight_, RK_FORMAT_RGB_888);

    rkRga.RkRgaBlit(&src, &dst, NULL);
    rkRga.RkRgaFlush();
    encodeJpegToMemory((unsigned char *)temp, previewWidth_, previewHeight_, nullptr, &jpegSize, &jBuf);
    int ret = memcpy_s((unsigned char*)buffer->GetVirAddress(), jpegSize, jBuf, jpegSize);
    if (ret != 0) {
        printf("memcpy_s failed!\n");
    }
    buffer->SetEsFrameSize(jpegSize);
    free(jBuf);
    free(temp);

    CAMERA_LOGE("RKCodecNode::Yuv420ToJpeg jpegSize = %{public}d\n", jpegSize);
}

void RKCodecNode::Yuv420ToH264(std::shared_ptr<IBuffer>& buffer)
{
    if (buffer == nullptr) {
        CAMERA_LOGI("RKCodecNode::Yuv420ToH264 buffer == nullptr");
        return;
    }

    int ret = 0;
    size_t buf_size = 0;
    struct timespec ts = {};
    int64_t timestamp = 0;
    int dma_fd = buffer->GetFileDescriptor();

    if (mppStatus_ == 0) {
        MpiEncTestArgs args = {};
        args.width       = previewWidth_;
        args.height      = previewHeight_;
        args.format      = MPP_FMT_YUV420P;
        args.type        = MPP_VIDEO_CodingAVC;
        halCtx_ = hal_mpp_ctx_create(&args);
        if (halCtx_ == nullptr) {
            CAMERA_LOGI("RKCodecNode::Yuv420ToH264 halCtx_ = %{public}p\n", halCtx_);
            return;
        }
        mppStatus_ = 1;
        buf_size = ((MpiEncTestData *)halCtx_)->frame_size;

        ret = hal_mpp_encode(halCtx_, dma_fd, (unsigned char *)buffer->GetVirAddress(), &buf_size);
        SerchIFps((unsigned char *)buffer->GetVirAddress(), buf_size, buffer);

        buffer->SetEsFrameSize(buf_size);
        clock_gettime(CLOCK_MONOTONIC, &ts);
        timestamp = ts.tv_nsec + ts.tv_sec * TIME_CONVERSION_NS_S;
        buffer->SetEsTimestamp(timestamp);
        CAMERA_LOGI("RKCodecNode::Yuv420ToH264 video capture on\n");
    } else {
        if (halCtx_ == nullptr) {
            CAMERA_LOGI("RKCodecNode::Yuv420ToH264 halCtx_ = %{public}p\n", halCtx_);
            return;
        }
        buf_size = ((MpiEncTestData *)halCtx_)->frame_size;
        ret = hal_mpp_encode(halCtx_, dma_fd, (unsigned char *)buffer->GetVirAddress(), &buf_size);
        SerchIFps((unsigned char *)buffer->GetVirAddress(), buf_size, buffer);
        buffer->SetEsFrameSize(buf_size);
        clock_gettime(CLOCK_MONOTONIC, &ts);
        timestamp = ts.tv_nsec + ts.tv_sec * TIME_CONVERSION_NS_S;
        buffer->SetEsTimestamp(timestamp);
    }

    CAMERA_LOGI("ForkNode::ForkBuffers H264 size = %{public}d ret = %{public}d timestamp = %{public}lld\n",
        buf_size, ret, timestamp);
}

void RKCodecNode::DeliverBuffer(std::shared_ptr<IBuffer>& buffer)
{
    if (buffer == nullptr) {
        CAMERA_LOGE("RKCodecNode::DeliverBuffer frameSpec is null");
        return;
    }

    int32_t id = buffer->GetStreamId();
    CAMERA_LOGE("RKCodecNode::DeliverBuffer StreamId %{public}d", id);
    if (buffer->GetEncodeType() == ENCODE_TYPE_JPEG) {
        Yuv420ToJpeg(buffer);
    } else if (buffer->GetEncodeType() == ENCODE_TYPE_H264) {
        Yuv420ToH264(buffer);
    } else {
        Yuv420ToRGBA8888(buffer);
    }

    outPutPorts_ = GetOutPorts();
    for (auto& it : outPutPorts_) {
        if (it->format_.streamId_ == id) {
            it->DeliverBuffer(buffer);
            CAMERA_LOGI("RKCodecNode deliver buffer streamid = %{public}d", it->format_.streamId_);
            return;
        }
    }
}

RetCode RKCodecNode::Capture(const int32_t streamId, const int32_t captureId)
{
    CAMERA_LOGV("RKCodecNode::Capture");
    return RC_OK;
}

RetCode RKCodecNode::CancelCapture(const int32_t streamId)
{
    CAMERA_LOGI("RKCodecNode::CancelCapture streamid = %{public}d", streamId);

    return RC_OK;
}

REGISTERNODE(RKCodecNode, {"RKCodec"})
} // namespace OHOS::Camera
