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

#ifndef __MPI_ENC_UTILS_H__
#define __MPI_ENC_UTILS_H__

#include <stdio.h>

#include "rk_venc_cmd.h"

typedef struct MpiEncTestArgs_t {
    MppCodingType       type;
    MppFrameFormat      format;

    RK_S32              width;
    RK_S32              height;
} MpiEncTestArgs;

typedef struct {
    // global flow control flag
    RK_U32 frm_eos;
    RK_U32 pkt_eos;
    RK_S32 frame_count;
    RK_U64 stream_size;

    // base flow context
    MppCtx ctx;
    MppApi *mpi;
    MppEncCfg cfg;

    // input / output
    MppBufferGroup buf_grp;
    MppBuffer pkt_buf;
    MppEncSeiMode sei_mode;
    MppEncHeaderMode header_mode;

    // paramter for resource malloc
    RK_U32 width;
    RK_U32 height;
    RK_U32 hor_stride;
    RK_U32 ver_stride;
    MppFrameFormat fmt;
    MppCodingType type;
    RK_S32 num_frames;

    // resources
    size_t frame_size;

    RK_U32 split_mode;
    RK_U32 split_arg;

    // rate control runtime parameter

    RK_S32 fps_in_flex;
    RK_S32 fps_in_den;
    RK_S32 fps_in_num;
    RK_S32 fps_out_flex;
    RK_S32 fps_out_den;
    RK_S32 fps_out_num;
    RK_S32 bps;
    RK_S32 bps_max;
    RK_S32 bps_min;
    RK_S32 rc_mode;
    RK_S32 gop_mode;
    RK_S32 gop_len;
    RK_S32 vi_len;
} MpiEncTestData;

int hal_mpp_get_sps(void *ctx, unsigned char *buf, size_t *buf_size);
int hal_mpp_encode(void *ctx, int dma_fd, unsigned char *buf, size_t *buf_size);

void *hal_mpp_ctx_create(MpiEncTestArgs *args);
void hal_mpp_ctx_delete(void *ctx);

#endif /* __MPI_ENC_UTILS_H__ */
