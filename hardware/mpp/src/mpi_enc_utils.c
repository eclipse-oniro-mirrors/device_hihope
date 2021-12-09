/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MODULE_TAG "mpi_enc_utils"

#include <string.h>
#include "securec.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_buffer.h"
#include "rk_mpi.h"
#include "mpp_common.h"
#include "mpi_enc_utils.h"

static MPP_RET mpi_enc_gen_ref_cfg(MppEncRefCfg ref, RK_S32 gop_mode)
{
    MppEncRefLtFrmCfg lt_ref[4];
    MppEncRefStFrmCfg st_ref[16];
    RK_S32 lt_cnt = 0;
    RK_S32 st_cnt = 0;
    MPP_RET ret = 0;

    errno_t eok = memset_s(&lt_ref, sizeof(lt_ref), 0, sizeof(lt_ref));
    if (eok != EOK) {
        mpp_err("memset_s failed\n");
        return MPP_NOK;
    }
    eok = memset_s(&st_ref, sizeof(lt_ref), 0, sizeof(st_ref));
    if (eok != EOK) {
        mpp_err("memset_s failed\n");
        return MPP_NOK;
    }

    switch (gop_mode) {
        case 3 : {
            // tsvc4
            //      /-> P1      /-> P3        /-> P5      /-> P7
            //     /           /             /           /
            //    //--------> P2            //--------> P6
            //   //                        //
            //  ///---------------------> P4
            // ///
            // P0 ------------------------------------------------> P8
            lt_cnt = 1;

            /* set 8 frame lt-ref gap */
            lt_ref[0].lt_idx        = 0;
            lt_ref[0].temporal_id   = 0;
            lt_ref[0].ref_mode      = REF_TO_PREV_LT_REF;
            lt_ref[0].lt_gap        = 8; // 8:set 8 frame
            lt_ref[0].lt_delay      = 0;

            st_cnt = 9;
            /* set tsvc4 st-ref struct */
            /* st 0 layer 0 - ref */
            st_ref[0].is_non_ref    = 0;
            st_ref[0].temporal_id   = 0;
            st_ref[0].ref_mode      = REF_TO_TEMPORAL_LAYER;
            st_ref[0].ref_arg       = 0;
            st_ref[0].repeat        = 0;
            /* st 1 layer 3 - non-ref */
            st_ref[1].is_non_ref    = 1;
            st_ref[1].temporal_id   = 3;
            st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[1].ref_arg       = 0;
            st_ref[1].repeat        = 0;
            /* st 2 layer 2 - ref */
            st_ref[2].is_non_ref    = 0;
            st_ref[2].temporal_id   = 2;
            st_ref[2].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[2].ref_arg       = 0;
            st_ref[2].repeat        = 0;
            /* st 3 layer 3 - non-ref */
            st_ref[3].is_non_ref    = 1;
            st_ref[3].temporal_id   = 3;
            st_ref[3].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[3].ref_arg       = 0;
            st_ref[3].repeat        = 0;
            /* st 4 layer 1 - ref */
            st_ref[4].is_non_ref    = 0;
            st_ref[4].temporal_id   = 1;
            st_ref[4].ref_mode      = REF_TO_PREV_LT_REF;
            st_ref[4].ref_arg       = 0;
            st_ref[4].repeat        = 0;
            /* st 5 layer 3 - non-ref */
            st_ref[5].is_non_ref    = 1;
            st_ref[5].temporal_id   = 3;
            st_ref[5].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[5].ref_arg       = 0;
            st_ref[5].repeat        = 0;
            /* st 6 layer 2 - ref */
            st_ref[6].is_non_ref    = 0;
            st_ref[6].temporal_id   = 2;
            st_ref[6].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[6].ref_arg       = 0;
            st_ref[6].repeat        = 0;
            /* st 7 layer 3 - non-ref */
            st_ref[7].is_non_ref    = 1;
            st_ref[7].temporal_id   = 3;
            st_ref[7].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[7].ref_arg       = 0;
            st_ref[7].repeat        = 0;
            /* st 8 layer 0 - ref */
            st_ref[8].is_non_ref    = 0;
            st_ref[8].temporal_id   = 0;
            st_ref[8].ref_mode      = REF_TO_TEMPORAL_LAYER;
            st_ref[8].ref_arg       = 0;
            st_ref[8].repeat        = 0;
        }
            break;
        case 2 : {
            // tsvc3
            //     /-> P1      /-> P3
            //    /           /
            //   //--------> P2
            //  //
            // P0/---------------------> P4
            lt_cnt = 0;

            st_cnt = 5;
            /* set tsvc4 st-ref struct */
            /* st 0 layer 0 - ref */
            st_ref[0].is_non_ref    = 0;
            st_ref[0].temporal_id   = 0;
            st_ref[0].ref_mode      = REF_TO_TEMPORAL_LAYER;
            st_ref[0].ref_arg       = 0;
            st_ref[0].repeat        = 0;
            /* st 1 layer 2 - non-ref */
            st_ref[1].is_non_ref    = 1;
            st_ref[1].temporal_id   = 2;
            st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[1].ref_arg       = 0;
            st_ref[1].repeat        = 0;
            /* st 2 layer 1 - ref */
            st_ref[2].is_non_ref    = 0;
            st_ref[2].temporal_id   = 1;
            st_ref[2].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[2].ref_arg       = 0;
            st_ref[2].repeat        = 0;
            /* st 3 layer 2 - non-ref */
            st_ref[3].is_non_ref    = 1;
            st_ref[3].temporal_id   = 2;
            st_ref[3].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[3].ref_arg       = 0;
            st_ref[3].repeat        = 0;
            /* st 4 layer 0 - ref */
            st_ref[4].is_non_ref    = 0;
            st_ref[4].temporal_id   = 0;
            st_ref[4].ref_mode      = REF_TO_TEMPORAL_LAYER;
            st_ref[4].ref_arg       = 0;
            st_ref[4].repeat        = 0;
        }
            break;
        case 1 : {
            // tsvc2
            //   /-> P1
            //  /
            // P0--------> P2
            lt_cnt = 0;

            st_cnt = 3;
            /* set tsvc4 st-ref struct */
            /* st 0 layer 0 - ref */
            st_ref[0].is_non_ref    = 0;
            st_ref[0].temporal_id   = 0;
            st_ref[0].ref_mode      = REF_TO_TEMPORAL_LAYER;
            st_ref[0].ref_arg       = 0;
            st_ref[0].repeat        = 0;
            /* st 1 layer 2 - non-ref */
            st_ref[1].is_non_ref    = 1;
            st_ref[1].temporal_id   = 1;
            st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[1].ref_arg       = 0;
            st_ref[1].repeat        = 0;
            /* st 2 layer 1 - ref */
            st_ref[2].is_non_ref    = 0;
            st_ref[2].temporal_id   = 0;
            st_ref[2].ref_mode      = REF_TO_PREV_REF_FRM;
            st_ref[2].ref_arg       = 0;
            st_ref[2].repeat        = 0;
        }
            break;
        default : {
            mpp_err_f("unsupport gop mode %d\n", gop_mode);
        }
            break;
    }

    if (lt_cnt || st_cnt) {
        ret = mpp_enc_ref_cfg_set_cfg_cnt(ref, lt_cnt, st_cnt);

        if (lt_cnt)
            ret = mpp_enc_ref_cfg_add_lt_cfg(ref, lt_cnt, lt_ref);

        if (st_cnt)
            ret = mpp_enc_ref_cfg_add_st_cfg(ref, st_cnt, st_ref);

        /* check and get dpb size */
        ret = mpp_enc_ref_cfg_check(ref);
    }

    return ret;
}

static MPP_RET mpi_enc_gen_smart_gop_ref_cfg(MppEncRefCfg ref, RK_S32 gop_len, RK_S32 vi_len)
{
    MppEncRefLtFrmCfg lt_ref[4];
    MppEncRefStFrmCfg st_ref[16];
    RK_S32 lt_cnt = 1;
    RK_S32 st_cnt = 8;
    RK_S32 pos = 0;
    MPP_RET ret;

    errno_t eok = memset_s(&lt_ref, sizeof(lt_ref), 0, sizeof(lt_ref));
    if (eok != EOK) {
        mpp_err("memset_s failed\n");
        return MPP_NOK;
    }
    eok = memset_s(&st_ref, sizeof(st_ref), 0, sizeof(st_ref));
    if (eok != EOK) {
        mpp_err("memset_s failed\n");
        return MPP_NOK;
    }

    ret = mpp_enc_ref_cfg_set_cfg_cnt(ref, lt_cnt, st_cnt);

    /* set 8 frame lt-ref gap */
    lt_ref[0].lt_idx        = 0;
    lt_ref[0].temporal_id   = 0;
    lt_ref[0].ref_mode      = REF_TO_PREV_LT_REF;
    lt_ref[0].lt_gap        = gop_len;
    lt_ref[0].lt_delay      = 0;

    mpp_enc_ref_cfg_add_lt_cfg(ref, 1, lt_ref);

    /* st 0 layer 0 - ref */
    st_ref[pos].is_non_ref  = 0;
    st_ref[pos].temporal_id = 0;
    st_ref[pos].ref_mode    = REF_TO_PREV_INTRA;
    st_ref[pos].ref_arg     = 0;
    st_ref[pos].repeat      = 0;
    pos++;

    /* st 1 layer 1 - non-ref */
    if (vi_len > 1) {
        st_ref[pos].is_non_ref  = 0;
        st_ref[pos].temporal_id = 1;
        st_ref[pos].ref_mode    = REF_TO_PREV_REF_FRM;
        st_ref[pos].ref_arg     = 0;
        st_ref[pos].repeat      = vi_len - 2;
        pos++;
    }

    st_ref[pos].is_non_ref  = 0;
    st_ref[pos].temporal_id = 0;
    st_ref[pos].ref_mode    = REF_TO_PREV_INTRA;
    st_ref[pos].ref_arg     = 0;
    st_ref[pos].repeat      = 0;
    pos++;

    mpp_enc_ref_cfg_add_st_cfg(ref, pos, st_ref);

    /* check and get dpb size */
    ret = mpp_enc_ref_cfg_check(ref);

    return ret;
}

/* ���˱������õ�һЩ����������ߣ�����ͼ���ʽ����������ʽ�ȣ�����������֪��ֱ����0
 * test_mpp_enc_cfg_setup�������Զ�����Ĭ�ϵĲ�����bps�� gop�Ȳ���
 */
static MPP_RET test_ctx_init(MpiEncTestData **data, MpiEncTestArgs *cmd)
{
    MpiEncTestData *p = NULL;
    MPP_RET ret = 0;

    if (!data || !cmd) {
        mpp_err_f("invalid input data %p cmd %p\n", data, cmd);
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_calloc(MpiEncTestData, 1);
    if (!p) {
        mpp_err_f("create MpiEncTestData failed\n");
        ret = MPP_ERR_MALLOC;
        *data = p;
    }

    // get paramter from cmd
    p->width        = cmd->width;
    p->height       = cmd->height;
    p->hor_stride   = MPP_ALIGN(cmd->width, 16);
    p->ver_stride   = MPP_ALIGN(cmd->height, 16);
    p->fmt          = cmd->format;
    p->type         = cmd->type;

    // update resource parameter
    switch (p->fmt & MPP_FRAME_FMT_MASK) {
        case MPP_FMT_YUV420SP:
        case MPP_FMT_YUV420P: {
            p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 3 / 2;
        }
            break;

        case MPP_FMT_YUV422_YUYV :
        case MPP_FMT_YUV422_YVYU :
        case MPP_FMT_YUV422_UYVY :
        case MPP_FMT_YUV422_VYUY :
        case MPP_FMT_YUV422P :
        case MPP_FMT_YUV422SP :
        case MPP_FMT_RGB444 :
        case MPP_FMT_BGR444 :
        case MPP_FMT_RGB555 :
        case MPP_FMT_BGR555 :
        case MPP_FMT_RGB565 :
        case MPP_FMT_BGR565 : {
            p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 2;
        }
            break;
        default: {
            p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 4;
        }
            break;
    }
    return ret;
}

static MPP_RET test_ctx_deinit(MpiEncTestData **data)
{
    MpiEncTestData *p = NULL;

    if (!data) {
        mpp_err_f("invalid input data %p\n", data);
        return MPP_ERR_NULL_PTR;
    }
    p = *data;
    if (p) {
        MPP_FREE(p);
        *data = NULL;
    }
    return MPP_OK;
}

static MPP_RET test_mpp_enc_cfg_setup(MpiEncTestData *p)
{
    MPP_RET ret;
    MppApi *mpi;
    MppCtx ctx;
    MppEncCfg cfg;

    if (NULL == p)
        return MPP_ERR_NULL_PTR;

    mpi = p->mpi;
    ctx = p->ctx;
    cfg = p->cfg;

    /* setup default parameter */
    if (p->fps_in_den == 0)
        p->fps_in_den = 1;
    if (p->fps_in_num == 0)
        p->fps_in_num = 30;
    if (p->fps_out_den == 0)
        p->fps_out_den = 1;
    if (p->fps_out_num == 0)
        p->fps_out_num = 30;

    /* ����Ĭ��bps */
    if (!p->bps)
        p->bps = p->width * p->height / 8 * (p->fps_out_num / p->fps_out_den);

    mpp_enc_cfg_set_s32(cfg, "prep:width", p->width);
    mpp_enc_cfg_set_s32(cfg, "prep:height", p->height);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", p->hor_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", p->ver_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:format", p->fmt);

    mpp_enc_cfg_set_s32(cfg, "rc:mode", p->rc_mode);

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", p->fps_in_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", p->fps_in_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm", p->fps_in_den);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", p->fps_out_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", p->fps_out_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", p->fps_out_den);
    mpp_enc_cfg_set_s32(cfg, "rc:gop", p->gop_len ? p->gop_len : p->fps_out_num * 2);

    /* drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20);        /* 20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1);         /* Do not continuous drop frame */

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", p->bps);
    switch (p->rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP : {
            /* do not setup bitrate on FIXQP mode */
        }
            break;
        case MPP_ENC_RC_MODE_CBR : {
            /* CBR mode has narrow bound */
            mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
            mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 15 / 16);
        }
            break;
        case MPP_ENC_RC_MODE_VBR :
        case MPP_ENC_RC_MODE_AVBR : {
            /* VBR mode has wide bound */
            mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
            mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 1 / 16);
        }
            break;
        default : {
            /* default use CBR mode */
            mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
            mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 15 / 16);
        }
            break;
    }

    /* setup qp for different codec and rc_mode */
    switch (p->type) {
        case MPP_VIDEO_CodingAVC :
        case MPP_VIDEO_CodingHEVC : {
            switch (p->rc_mode) {
                case MPP_ENC_RC_MODE_FIXQP : {
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 20);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 20);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 20);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 20);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 20);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
                }
                    break;
                case MPP_ENC_RC_MODE_CBR :
                case MPP_ENC_RC_MODE_VBR :
                case MPP_ENC_RC_MODE_AVBR : {
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 26);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 51);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 10);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 51);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 10);
                    mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
                }
                    break;
                default : {
                    mpp_err_f("unsupport encoder rc mode %d\n", p->rc_mode);
                }
                    break;
            }
        }
            break;
        case MPP_VIDEO_CodingVP8 : {
            /* vp8 only setup base qp range */
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 40);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max",  127);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min",  0);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 127);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 0);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 6);
        }
            break;
        case MPP_VIDEO_CodingMJPEG : {
            /* jpeg use special codec config to control qtable */
            mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", 80);
            mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", 99);
            mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", 1);
        }
            break;
        default : {
        }
            break;
    }

    /* setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type", p->type);
    switch (p->type) {
        case MPP_VIDEO_CodingAVC : {
            /*
            * H.264 profile_idc parameter
            * 66  - Baseline profile
            * 77  - Main profile
            * 100 - High profile
            */
            mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
            /*
            * H.264 level_idc parameter
            * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
            * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
            * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
            * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
            * 50 / 51 / 52         - 4K@30fps
            */
            mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
            mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
            mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
            mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);
        }
            break;
        case MPP_VIDEO_CodingHEVC :
        case MPP_VIDEO_CodingMJPEG :
        case MPP_VIDEO_CodingVP8 : {
        }
            break;
        default : {
            mpp_err_f("unsupport encoder coding type %d\n", p->type);
        }
            break;
    }

    p->split_mode = 0;
    p->split_arg = 0;

    mpp_env_get_u32("split_mode", &p->split_mode, MPP_ENC_SPLIT_NONE);
    mpp_env_get_u32("split_arg", &p->split_arg, 0);

    if (p->split_mode) {
        mpp_log("%p split_mode %d split_arg %d\n", ctx, p->split_mode, p->split_arg);
        mpp_enc_cfg_set_s32(cfg, "split:mode", p->split_mode);
        mpp_enc_cfg_set_s32(cfg, "split:arg", p->split_arg);
    }

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        mpp_err("mpi control enc set cfg failed ret %d\n", ret);
        return ret;
    }

    /* optional */
    p->sei_mode = MPP_ENC_SEI_MODE_ONE_FRAME;
    ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
    if (ret) {
        mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
        return ret;
    }

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        p->header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &p->header_mode);
        if (ret) {
            mpp_err("mpi control enc set header mode failed ret %d\n", ret);
            return ret;
        }
    }

    RK_U32 gop_mode = p->gop_mode;

    mpp_env_get_u32("gop_mode", &gop_mode, gop_mode);
    if (gop_mode) {
        MppEncRefCfg ref;

        mpp_enc_ref_cfg_init(&ref);

        if (p->gop_mode < 4) {
            mpi_enc_gen_ref_cfg(ref, gop_mode);
        } else {
            mpi_enc_gen_smart_gop_ref_cfg(ref, p->gop_len, p->vi_len);
        }
        ret = mpi->control(ctx, MPP_ENC_SET_REF_CFG, ref);
        if (ret) {
            mpp_err("mpi control enc set ref cfg failed ret %d\n", ret);
            return ret;
        }
        mpp_enc_ref_cfg_deinit(&ref);
    }
    return ret;
}

static void test_mpp_frame_set_param(MppFrame frame, MpiEncTestData *p)
{
    mpp_frame_set_width(frame, p->width);
    mpp_frame_set_height(frame, p->height);
    mpp_frame_set_hor_stride(frame, p->hor_stride);
    mpp_frame_set_ver_stride(frame, p->ver_stride);
    mpp_frame_set_fmt(frame, p->fmt);
    mpp_frame_set_eos(frame, p->frm_eos);
}

static void test_mpp_ctx_cleanup(MpiEncTestData *p)
{
    if (!p) {
        return;
    }

    p->mpi->reset(p->ctx);

    if (p->ctx) {
        mpp_destroy(p->ctx);
        p->ctx = NULL;
    }

    if (p->cfg) {
        mpp_enc_cfg_deinit(p->cfg);
        p->cfg = NULL;
    }

    if (p->pkt_buf) {
        mpp_buffer_put(p->pkt_buf);
        p->pkt_buf = NULL;
    }

    if (p->buf_grp) {
        mpp_buffer_group_put(p->buf_grp);
        p->buf_grp = NULL;
    }

    test_ctx_deinit(&p);
}

int hal_mpp_get_sps(void *ctx, unsigned char *buf, size_t *buf_size)
{
    int ret;
    MppApi *mpi;
    MppCtx mpp_ctx;
    MppPacket packet = NULL;
    MpiEncTestData *p = (MpiEncTestData *)ctx;

    mpi = p->mpi;
    mpp_ctx = p->ctx;

    /*
     * Can use packet with normal malloc buffer as input not pkt_buf.
     * Please refer to vpu_api_legacy.cpp for normal buffer case.
     * Using pkt_buf buffer here is just for simplifing demo.
     */

    mpp_packet_init_with_buffer(&packet, p->pkt_buf);
    /* NOTE: It is important to clear output packet length!! */
    mpp_packet_set_length(packet, 0);

    // ����һ��H264��H265ר�õ�һЩ����
    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        ret = mpi->control(mpp_ctx, MPP_ENC_GET_HDR_SYNC, packet);
        if (ret) {
            mpp_err("mpi control enc get extra info failed\n");
            ret = MPP_NOK;
            mpp_packet_deinit(&packet);
            return MPP_OK;
        }
    }

    void *ptr = mpp_packet_get_pos(packet);
    size_t len = mpp_packet_get_length(packet);

    if (*buf_size < len) {
        mpp_err("mpi buffer size too small\n");
        ret = MPP_NOK;
        mpp_packet_deinit(&packet);
        return MPP_OK;
    }

    ret = memcpy_s(buf, len, ptr, len);
    if (!ret) {
        printf("memcpy_s failed!\n");
    }

    *buf_size = len;

    ret = MPP_OK;
    return ret;
}

int hal_mpp_encode(void *ctx, int dma_fd, unsigned char *buf, size_t *buf_size)
{
    MPP_RET ret = 0;
    MppFrame frame = NULL;
    MppMeta meta = NULL;
    MppPacket packet = NULL;
    MpiEncTestData *p = (MpiEncTestData *)ctx;
    MppApi *mpi = p->mpi;
    RK_U32 eoi = 1;
    RK_U32 packet_num = 0;
    MppBuffer cam_buf = NULL;
    MppBufferInfo info;

    errno_t eok = memset_s(&info, sizeof(MppBufferInfo), 0, sizeof(MppBufferInfo));
    if (eok != EOK) {
        mpp_err("memset_s failed\n");
        return MPP_NOK;
    }
    info.type = MPP_BUFFER_TYPE_EXT_DMA;
    info.fd = dma_fd;
    info.size = p->frame_size & 0x07ffffff;
    info.index = (p->frame_size & 0xf8000000) >> 27;

    ret = mpp_buffer_import(&cam_buf, &info);
    if (ret != MPP_SUCCESS) {
        mpp_err_f("mpp_buffer_import failed\n");
        return MPP_NOK;
    }

    ret = mpp_frame_init(&frame);
    if (ret) {
        mpp_err_f("mpp_frame_init failed\n");
        return MPP_NOK;
    }

    /* set frame size info */
    test_mpp_frame_set_param(frame, p);

    /* set frame data include fd */
    mpp_frame_set_buffer(frame, cam_buf);

    /* packet init */
    mpp_packet_init_with_buffer(&packet, p->pkt_buf);
    /* NOTE: It is important to clear output packet length!! */
    mpp_packet_set_length(packet, 0);

    meta = mpp_frame_get_meta(frame);
    mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);

    /*
     * NOTE: in non-block mode the frame can be resent.
     * The default input timeout mode is block.
     *
     * User should release the input frame to meet the requirements of
     * resource creator must be the resource destroyer.
     */
    ret = mpi->encode_put_frame(p->ctx, frame);
    if (ret) {
        mpp_err("mpp encode put frame failed\n");
        mpp_frame_deinit(&frame);
        if (cam_buf) {
            mpp_buffer_put(cam_buf);
        }
        mpp_packet_deinit(&packet);
        return ret;
    }

    mpp_frame_deinit(&frame);

    packet_num = 0;

    do {
        ret = mpi->encode_get_packet(p->ctx, &packet);
        if (ret) {
            mpp_err("mpp encode get packet failed\n");
            if (cam_buf) {
                mpp_buffer_put(cam_buf);
            }
            mpp_packet_deinit(&packet);
            return ret;
        }

        mpp_assert(packet);

        if (packet) {
            /* for low delay partition encoding */
            if (mpp_packet_is_partition(packet)) {
                eoi = mpp_packet_is_eoi(packet);
            }

            p->frame_count += eoi;
            packet_num++;
        }
    } while (!eoi);

    void *ptr = mpp_packet_get_pos(packet);
    size_t len = mpp_packet_get_length(packet);

    if (packet_num != 1) {
        mpp_err("packet_num %u != 1\n");
        ret = MPP_NOK;
        if (cam_buf) {
            mpp_buffer_put(cam_buf);
        }
        mpp_packet_deinit(&packet);
        return ret;
    }

    if (*buf_size < len) {
        mpp_err("mpi buffer size too small\n");
        ret = MPP_NOK;
        if (cam_buf) {
            mpp_buffer_put(cam_buf);
        }
        mpp_packet_deinit(&packet);
        return ret;
    }

    ret = memcpy_s(buf, len, ptr, len);
    if (!ret) {
        printf("memcpy_s failed!\n");
    }

    *buf_size = len;

    ret = MPP_OK;
    return ret;
}

void *hal_mpp_ctx_create(MpiEncTestArgs *args)
{
    MPP_RET ret = 0;
    MpiEncTestData *p = NULL;
    MppPollType timeout = MPP_POLL_BLOCK;

    /* ��ʼ��һ�³��õĲ������ã�cmd�����ñȽ϶� */
    ret = test_ctx_init(&p, args);
    if (ret) {
        mpp_err_f("test data init failed ret %d\n", ret);
        mpp_err("%p mpi_enc_test failed ret %d\n", p->ctx, ret);
        return NULL;
    }

    ret = mpp_buffer_group_get_internal(&p->buf_grp, MPP_BUFFER_TYPE_DRM);
    if (ret) {
        mpp_err_f("failed to get mpp buffer group ret %d\n", ret);
        mpp_err("%p mpi_enc_test failed ret %d\n", p->ctx, ret);
        return NULL;
    }

    /* pkt_buf�����Ա���?���Ǹ�packetʹ�õ�buffer */
    ret = mpp_buffer_get(p->buf_grp, &p->pkt_buf, p->frame_size);
    if (ret) {
        mpp_err_f("failed to get buffer for output packet ret %d\n", ret);
        mpp_err("%p mpi_enc_test failed ret %d\n", p->ctx, ret);
        return NULL;
    }

    // encoder demo
    ret = mpp_create(&p->ctx, &p->mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
        mpp_err("%p mpi_enc_test failed ret %d\n", p->ctx, ret);
        return NULL;
    }

    mpp_log("%p mpi_enc_test encoder test start w %d h %d type %d\n",
            p->ctx, p->width, p->height, p->type);

    ret = p->mpi->control(p->ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (MPP_OK != ret) {
        mpp_err("mpi control set output timeout %d ret %d\n", timeout, ret);
        mpp_err("%p mpi_enc_test failed ret %d\n", p->ctx, ret);
        return NULL;
    }

    ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
        mpp_err("%p mpi_enc_test failed ret %d\n", p->ctx, ret);
        return NULL;
    }

    ret = mpp_enc_cfg_init(&p->cfg);
    if (ret) {
        mpp_err_f("mpp_enc_cfg_init failed ret %d\n", ret);
        mpp_err("%p mpi_enc_test failed ret %d\n", p->ctx, ret);
        return NULL;
    }

    ret = test_mpp_enc_cfg_setup(p);
    if (ret) {
        mpp_err_f("test mpp setup failed ret %d\n", ret);
        mpp_err("%p mpi_enc_test failed ret %d\n", p->ctx, ret);
        return NULL;
    }

    return p;
}

void hal_mpp_ctx_delete(void *ctx)
{
    test_mpp_ctx_cleanup(ctx);
}