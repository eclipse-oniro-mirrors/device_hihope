/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
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

#include <stdio.h>
#include <string.h>
#include "im2d.h"
#include "rga.h"
#include "display_common.h"
#include "display_gralloc.h"
#include "securec.h"
#include "display_gfx.h"

#define ALIGN_UP(x, a) ((((x) + ((a)-1)) / (a)) * (a))
GrallocFuncs *grallocFucs = NULL;
int32_t rkInitGfx()
{
    DISPLAY_LOGE("%s\n", querystring(RGA_ALL));
    return DISPLAY_SUCCESS;
}

int32_t rkDeinitGfx()
{
    return DISPLAY_SUCCESS;
}

RgaSURF_FORMAT colorSpaceModeChange(PixelFormat color, uint8_t *isYuv)
{
    RgaSURF_FORMAT rkFormat;
    switch (color) {
        case PIXEL_FMT_RGB_565:          /**< RGB565 format */
            rkFormat = RK_FORMAT_RGB_565;
            *isYuv = 0;
            break;
        case PIXEL_FMT_RGBA_4444:        /**< RGBA4444 format */
            rkFormat = RK_FORMAT_RGBA_4444;
            *isYuv = 0;
            break;
        case PIXEL_FMT_RGBA_5551:        /**< RGBA5551 format */
            rkFormat = RK_FORMAT_RGBA_5551;
            *isYuv = 0;
            break;
        case PIXEL_FMT_RGBX_8888:        /**< RGBX8888 format */
            rkFormat = RK_FORMAT_RGBX_8888;
            *isYuv = 0;
            break;
        case PIXEL_FMT_RGBA_8888:        /**< RGBA8888 format */
            rkFormat = RK_FORMAT_RGBA_8888;
            *isYuv = 0;
            break;
        case PIXEL_FMT_RGB_888:          /**< RGB888 format */
            rkFormat = RK_FORMAT_RGB_888;
            *isYuv = 0;
            break;
        case PIXEL_FMT_BGR_565:          /**< BGR565 format */
            rkFormat = RK_FORMAT_BGR_565;
            *isYuv = 0;
            break;
        case PIXEL_FMT_BGRA_4444:        /**< BGRA4444 format */
            rkFormat = RK_FORMAT_BGRA_4444;
            *isYuv = 0;
            break;
        case PIXEL_FMT_BGRA_5551:        /**< BGRA5551 format */
            rkFormat = RK_FORMAT_BGRA_5551;
            *isYuv = 0;
            break;
        case PIXEL_FMT_BGRX_8888:        /**< BGRX8888 format */
            rkFormat = RK_FORMAT_BGRX_8888;
            *isYuv = 0;
            break;
        case PIXEL_FMT_BGRA_8888:        /**< BGRA8888 format */
            rkFormat = RK_FORMAT_BGRA_8888;
            *isYuv = 0;
            break;
        case PIXEL_FMT_YCBCR_422_SP:     /**< YCBCR422 semi-planar format */
            rkFormat = RK_FORMAT_YCbCr_420_SP;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCRCB_422_SP:     /**< YCRCB422 semi-planar format */
            rkFormat = RK_FORMAT_YCrCb_422_SP;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCBCR_420_SP:     /**< YCBCR420 semi-planar format */
            rkFormat = RK_FORMAT_YCbCr_420_SP;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCRCB_420_SP:     /**< YCRCB420 semi-planar format */
            rkFormat = RK_FORMAT_YCrCb_420_SP;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCBCR_422_P:      /**< YCBCR422 planar format */
            rkFormat = RK_FORMAT_YCbCr_422_P;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCRCB_422_P:      /**< YCRCB422 planar format */
            rkFormat = RK_FORMAT_YCrCb_422_P;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCBCR_420_P:      /**< YCBCR420 planar format */
            rkFormat = RK_FORMAT_YCbCr_420_P;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YCRCB_420_P:      /**< YCRCB420 planar format */
            rkFormat = RK_FORMAT_YCrCb_420_P;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YUYV_422_PKG:     /**< YUYV422 packed format */
            rkFormat = RK_FORMAT_YUYV_422;
            *isYuv = 1;
            break;
        case PIXEL_FMT_UYVY_422_PKG:     /**< UYVY422 packed format */
            rkFormat = RK_FORMAT_UYVY_422;
            *isYuv = 1;
            break;
        case PIXEL_FMT_YVYU_422_PKG:     /**< YVYU422 packed format */
            rkFormat = RK_FORMAT_YUYV_422;
            *isYuv = 1;
            break;
        case PIXEL_FMT_VYUY_422_PKG:     /**< VYUY422 packed format */
            rkFormat = RK_FORMAT_VYUY_422;
            *isYuv = 1;
            break;
    default:
//          PIXEL_FMT_CLUT8:        /**< CLUT8 format */
//          PIXEL_FMT_CLUT1,            /**< CLUT1 format */
//          PIXEL_FMT_CLUT4,            /**< CLUT4 format */
//          PIXEL_FMT_RGBA_5658,        /**< RGBA5658 format */
//          PIXEL_FMT_RGBX_4444,        /**< RGBX4444 format */
//          PIXEL_FMT_RGB_444,          /**< RGB444 format */
//          PIXEL_FMT_RGBX_5551,        /**< RGBX5551 format */
//          PIXEL_FMT_RGB_555,          /**< RGB555 format */
//          PIXEL_FMT_BGRX_4444,        /**< BGRX4444 format */
//          PIXEL_FMT_BGRX_5551,        /**< BGRX5551 format */
//          PIXEL_FMT_YUV_422_I,        /**< YUV422 interleaved format */
            rkFormat = RK_FORMAT_UNKNOWN;
        break;
    }
    return rkFormat;
}

int32_t rkFillRect(ISurface *iSurface, IRect *rect, uint32_t color, GfxOpt *opt)
{
    rga_buffer_t dst;
    im_rect imRect;
    IM_STATUS ret;
    uint8_t isYuv;

    errno_t eok = memset_s((void *)&imRect, sizeof(imRect), 0, sizeof(imRect));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        return false;
    }
    imRect.x = rect->x;
    imRect.y = rect->y;
    imRect.width = rect->w;
    imRect.height = rect->h;

    eok = memset_s((void *)&dst, sizeof(dst), 0, sizeof(dst));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        return false;
    }
    dst.phy_addr = 0; // (void*)iSurface->phyAddr;
    dst.vir_addr = 0; // iSurface->virAddr;
    dst.fd = (int32_t)iSurface->phyAddr;
    if ((int)dst.phy_addr == 0 && dst.fd == 0 && dst.vir_addr == NULL) {
        DISPLAY_LOGE("source iSurface address error");
        return DISPLAY_PARAM_ERR;
    }
    DISPLAY_LOGE("gfx vir %{public}p phy 0x%{public}x fd %{public}d", dst.vir_addr, (int32_t)dst.phy_addr, dst.fd);
    dst.width = iSurface->width;
    dst.height = iSurface->height;
    dst.wstride = ALIGN_UP(iSurface->width, 16);
    dst.hstride = ALIGN_UP(iSurface->height, 16);
    dst.format = colorSpaceModeChange(iSurface->enColorFmt, &isYuv);
    dst.color_space_mode = IM_COLOR_SPACE_DEFAULT;
    dst.color = color;
    if (opt->enGlobalAlpha)
        dst.global_alpha = opt->globalAlpha;
    ret = imfill(dst, imRect, color);
    if (ret != IM_STATUS_SUCCESS)
        return DISPLAY_FAILURE;
    else
        return DISPLAY_SUCCESS;
}

int32_t blendTypeChange(BlendType blendType)
{
    int32_t rkBlendType;
    switch (blendType) {
        case BLEND_SRC:              /**< SRC blending */
            rkBlendType = IM_ALPHA_BLEND_SRC;
            break;
        case BLEND_DST:              /**< SRC blending */
            rkBlendType = IM_ALPHA_BLEND_DST;
            break;
        case BLEND_SRCOVER:          /**< SRC_OVER blending */
            rkBlendType = IM_ALPHA_BLEND_SRC_OVER;
            break;
        case BLEND_DSTOVER:          /**< DST_OVER blending */
            rkBlendType = IM_ALPHA_BLEND_DST_OVER;
            break;
        default:
            /* Fix up later */
//        BLEND_NONE         /**< No blending */
//        BLEND_CLEAR:            /**< CLEAR blending */
//        BLEND_SRCIN:            /**< SRC_IN blending */
//        BLEND_DSTIN:            /**< DST_IN blending */
//        BLEND_SRCOUT:           /**< SRC_OUT blending */
//        BLEND_DSTOUT:           /**< DST_OUT blending */
//        BLEND_SRCATOP:          /**< SRC_ATOP blending */
//        BLEND_DSTATOP:          /**< DST_ATOP blending */
//        BLEND_ADD:              /**< ADD blending */
//        BLEND_XOR:              /**< XOR blending */
//        BLEND_DST:              /**< DST blending */
//        BLEND_AKS:              /**< AKS blending */
//        BLEND_AKD:              /**< AKD blending */
//        BLEND_BUTT:              /**< Null operation */
            rkBlendType = IM_STATUS_NOT_SUPPORTED;
            break;
    }
    return rkBlendType;
}

int32_t TransformTypeChange(TransformType type)
{
    int32_t rkRotateType;
    switch (type) {
        case ROTATE_90:            /**< Rotation by 90 degrees */
            rkRotateType = IM_HAL_TRANSFORM_ROT_90;
            break;
        case ROTATE_180:             /**< Rotation by 180 degrees */
            rkRotateType = IM_HAL_TRANSFORM_ROT_180;
            break;
        case ROTATE_270:             /**< Rotation by 270 degrees */
            rkRotateType = IM_HAL_TRANSFORM_ROT_270;
            break;
        default:
            rkRotateType = 0;        /**< No rotation */
            break;
    }
    return rkRotateType;
}

int32_t mirrorTypeChange(MirrorType type)
{
    int32_t rkMirrorType;
    switch (type) {
        case MIRROR_LR:            /**< Left and right mirrors */
            rkMirrorType = IM_HAL_TRANSFORM_FLIP_H;
            break;
        case MIRROR_TB:            /**< Top and bottom mirrors */
            rkMirrorType = IM_HAL_TRANSFORM_FLIP_V;
            break;
        default:
            rkMirrorType = 0;
            break;
    }
    return rkMirrorType;
}

int32_t doFlit(ISurface *srcSurface, IRect *srcRect, ISurface *dstSurface, IRect *dstRect, GfxOpt *opt)
{
    int32_t usage = 0;
    uint8_t isYuv = 0;
    rga_buffer_t dstRgaBuffer, srcRgaBuffer, bRgbBuffer;
    IM_STATUS ret = 0;
    im_rect srect;
    im_rect drect;
    im_rect prect;
    int32_t rkBlendType = 0;
    int32_t rkRotateType = 0;
    int32_t rkMirrorType = 0;

    errno_t eok = memset_s(&dstRgaBuffer, sizeof(dstRgaBuffer), 0, sizeof(dstRgaBuffer));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        return false;
    }
    eok = memset_s(&srcRgaBuffer, sizeof(srcRgaBuffer), 0, sizeof(srcRgaBuffer));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        return false;
    }
    eok = memset_s(&bRgbBuffer, sizeof(bRgbBuffer), 0, sizeof(bRgbBuffer));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        return false;
    }
    eok = memset_s(&srect, sizeof(srect), 0, sizeof(srect));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        return false;
    }
    eok = memset_s(&drect, sizeof(drect), 0, sizeof(drect));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        return false;
    }
    eok = memset_s(&prect, sizeof(prect), 0, sizeof(prect));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        return false;
    }
    if (opt->enGlobalAlpha) {
        dstRgaBuffer.global_alpha = opt->globalAlpha;
        srcRgaBuffer.global_alpha = opt->globalAlpha;
    }
    dstRgaBuffer.width = dstSurface->width;
    dstRgaBuffer.height = dstSurface->height;
    dstRgaBuffer.wstride = ALIGN_UP(dstSurface->width, 16);
    dstRgaBuffer.hstride = ALIGN_UP(dstSurface->height, 16);
    dstRgaBuffer.format = colorSpaceModeChange(dstSurface->enColorFmt, &isYuv);
    dstRgaBuffer.phy_addr = 0; // (void *)dstSurface->phyAddr;
    dstRgaBuffer.vir_addr = 0; // dstSurface->virAddr;
    dstRgaBuffer.color_space_mode = IM_COLOR_SPACE_DEFAULT;
    dstRgaBuffer.fd = (int32_t)dstSurface->phyAddr;
    if (isYuv == 1) {
        DISPLAY_LOGE("rk gfx do not support dst buffer is yuv format");
        return DISPLAY_PARAM_ERR;
    }

    srcRgaBuffer.width = srcSurface->width;
    srcRgaBuffer.height = srcSurface->height;
    srcRgaBuffer.wstride = ALIGN_UP(srcSurface->width, 16);
    srcRgaBuffer.hstride = ALIGN_UP(srcSurface->height, 16);
    srcRgaBuffer.phy_addr = 0; // (void *)srcSurface->phyAddr;
    srcRgaBuffer.vir_addr = 0; // srcSurface->virAddr;
    srcRgaBuffer.format = colorSpaceModeChange(srcSurface->enColorFmt, &isYuv);
    srcRgaBuffer.color_space_mode = IM_COLOR_SPACE_DEFAULT;
    srcRgaBuffer.fd = (int32_t)srcSurface->phyAddr;

    if ((int)srcRgaBuffer.phy_addr == 0 && srcRgaBuffer.fd == 0 && srcRgaBuffer.vir_addr == NULL) {
        DISPLAY_LOGE("source iSurface address error");
        return DISPLAY_PARAM_ERR;
    }

    DISPLAY_LOGE("gfx src fd %{public}d, w %{public}d, h %{publuc}d, sw %{public}d sh %{public}d vir %{public}p",
        (int32_t)srcSurface->phyAddr, srcSurface->width, srcSurface->height, ALIGN_UP(srcSurface->width, 16),
        ALIGN_UP(srcSurface->height, 16), srcRgaBuffer.vir_addr);
    DISPLAY_LOGE("gfx dst fd %{public}d, w %{public}d, h %{public}d, sw %{public}d sh %{public}d vir %{public}p",
        (int32_t)dstSurface->phyAddr, dstSurface->width, dstSurface->height, ALIGN_UP(dstSurface->width, 16),
        ALIGN_UP(dstSurface->height, 16), dstRgaBuffer.vir_addr);

    srect.x = srcRect->x;
    srect.y = srcRect->y;
    srect.height = srcRect->h;
    srect.width = srcRect->w;
    drect.x = dstRect->x;
    drect.y = dstRect->y;
    drect.height = dstRect->h;
    drect.width = dstRect->w;

    if (opt->blendType) {
        rkBlendType = blendTypeChange(opt->blendType);
        if (rkBlendType > 0) {
            usage |= rkBlendType;
            if (rkBlendType == IM_ALPHA_BLEND_DST_OVER || rkBlendType == IM_ALPHA_BLEND_SRC_OVER)
                usage |= IM_ALPHA_BLEND_PRE_MUL;
        } else if (rkBlendType == IM_STATUS_NOT_SUPPORTED) {
            return DISPLAY_NOT_SUPPORT;
        }
    }
    if (opt->rotateType) {
        rkRotateType = TransformTypeChange(opt->rotateType);
        if (rkRotateType != 0)
            usage |= rkRotateType;
    }
    if (opt->mirrorType == MIRROR_LR || opt->mirrorType == MIRROR_TB) {
        rkMirrorType = mirrorTypeChange(opt->mirrorType);
        if (rkMirrorType != 0)
            usage |= rkMirrorType;
    }
    if (opt->enableScale) {
        DISPLAY_LOGE("gfx scale from (%{puhblic}d, %{public}d) to (%{public}d, %{public}d)", srcRgaBuffer.width, srcRgaBuffer.height, dstRgaBuffer.width,
                     dstRgaBuffer.height);
    }
    usage |= IM_SYNC;
    if (isYuv == 1) {
        if (rkBlendType == IM_ALPHA_BLEND_SRC_OVER || rkBlendType == IM_ALPHA_BLEND_SRC) {
            usage = 0;
            if (opt->enableScale == 0) {
                eok = memset_s(&srect, sizeof(srect), 0, sizeof(srect));
                if (eok != EOK) {
                    DISPLAY_LOGE("memset_s failed");
                    return false;
                }
                srect.width = srcRgaBuffer.width;
                srect.height = srcRgaBuffer.height;

                eok = memset_s(&drect, sizeof(drect), 0, sizeof(drect));
                if (eok != EOK) {
                    DISPLAY_LOGE("memset_s failed");
                    return false;
                }
                drect.x = dstRgaBuffer.width - srcRgaBuffer.width;
                drect.y = dstRgaBuffer.height - srcRgaBuffer.height;
                drect.width = srcRgaBuffer.width;
                drect.height = srcRgaBuffer.height;
            }
            usage = rkRotateType | rkMirrorType | IM_SYNC;
            ret = improcess(srcRgaBuffer, dstRgaBuffer, bRgbBuffer, srect, drect, prect, usage);
            if (ret != IM_STATUS_SUCCESS) {
                DISPLAY_LOGE("gfx improcess %{public}s", imStrError(ret));
            }
        } else if (rkBlendType == IM_ALPHA_BLEND_DST_OVER) {
            if (grallocFucs == NULL) {
                ret = GrallocInitialize(&grallocFucs);
                DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("Gralloc init failed"));
            }
            AllocInfo info = {
                .width = dstRgaBuffer.width,
                .height = dstRgaBuffer.height,
                .usage = HBM_USE_MEM_DMA | HBM_USE_CPU_READ | HBM_USE_CPU_WRITE,
                .format = PIXEL_FMT_RGBA_8888, // srcSurface->enColorFmt,
            };
            BufferHandle *buffer = NULL;

            ret = grallocFucs->AllocMem(&info, &buffer);
            DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not alloc memory"));

            bRgbBuffer.width = dstRgaBuffer.width;
            bRgbBuffer.height = dstRgaBuffer.height;
            bRgbBuffer.wstride = dstRgaBuffer.wstride;
            bRgbBuffer.hstride = dstRgaBuffer.hstride;
            bRgbBuffer.format = RK_FORMAT_RGBA_8888; // srcRgaBuffer.format;
            bRgbBuffer.phy_addr = 0; // (void *) buffer->phyAddr;
            bRgbBuffer.vir_addr = 0; // buffer->virAddr;
            bRgbBuffer.color_space_mode = dstRgaBuffer.color_space_mode;
            bRgbBuffer.fd = (int32_t)buffer->phyAddr;
            int ret = memcpy_s(&prect, sizeof(drect), &drect, sizeof(drect));
            if (!ret) {
                printf("memcpy_s failed!\n");
            }
            ret = improcess(srcRgaBuffer, bRgbBuffer, dstRgaBuffer, srect, prect, drect, usage);
            if (ret != IM_STATUS_SUCCESS) {
                DISPLAY_LOGE("gfx improcess %{public}s", imStrError(ret));
            } else {
                ret = imcopy(bRgbBuffer, dstRgaBuffer);
                if (ret != IM_STATUS_SUCCESS) {
                    DISPLAY_LOGE("gfx improcess %{public}s", imStrError(ret));
                }
            }
            grallocFucs->FreeMem(buffer);
        }
    } else {
        ret = improcess(srcRgaBuffer, dstRgaBuffer, bRgbBuffer, srect, drect, prect, usage);
        if (ret != IM_STATUS_SUCCESS) {
            DISPLAY_LOGE("gfx improcess %{public}s", imStrError(ret));
        }
    }
    if (ret != IM_STATUS_SUCCESS)
        return DISPLAY_FAILURE;
    else
        return DISPLAY_SUCCESS;
}

int32_t rkBlit(ISurface *srcSurface, IRect *srcRect, ISurface *dstSurface, IRect *dstRect, GfxOpt *opt)
{
    CHECK_NULLPOINTER_RETURN_VALUE(srcSurface, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(srcRect, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(dstSurface, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(dstRect, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(opt, DISPLAY_NULL_PTR);

    if (doFlit(srcSurface, srcRect, dstSurface, dstRect, opt) < 0)
        return DISPLAY_FAILURE;
    else
        return DISPLAY_SUCCESS;
}

int32_t rkSync(int32_t timeOut)
{
    return DISPLAY_SUCCESS;
}

int32_t GfxInitialize(GfxFuncs **funcs)
{
    DISPLAY_CHK_RETURN((funcs == NULL), DISPLAY_PARAM_ERR, DISPLAY_LOGE("info is null"));
    GfxFuncs *gfxFuncs = (GfxFuncs *)malloc(sizeof(GfxFuncs));
    errno_t eok = memset_s((void *)gfxFuncs, sizeof(GfxFuncs), 0, sizeof(GfxFuncs));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        free(gfxFuncs);
        return false;
    }
    gfxFuncs->InitGfx = rkInitGfx;
    gfxFuncs->DeinitGfx = rkDeinitGfx;
    gfxFuncs->FillRect = rkFillRect;
    gfxFuncs->Blit = rkBlit;
    gfxFuncs->Sync = rkSync;
    *funcs = gfxFuncs;

    return DISPLAY_SUCCESS;
}

int32_t GfxUninitialize(GfxFuncs *funcs)
{
    CHECK_NULLPOINTER_RETURN_VALUE(funcs, DISPLAY_NULL_PTR);
    free(funcs);
    DISPLAY_LOGI("%s: gfx uninitialize success", __func__);
    return DISPLAY_SUCCESS;
}
