/*
 * Copyright © 2018-2021, VideoLAN and dav1d authors
 * Copyright © 2018, Two Orioles, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DAV1D_H
#define DAV1D_H

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdarg.h>


#include <stddef.h>
#include <stdint.h>


#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#ifndef DAV1D_API
#if defined _WIN32
#if defined DAV1D_BUILDING_DLL
#define DAV1D_API __declspec(dllexport)
#else
#define DAV1D_API
#endif
#else
#if __GNUC__ >= 4
#define DAV1D_API __attribute__ ((visibility ("default")))
#else
#define DAV1D_API
#endif
#endif
#endif

#if EPERM > 0
#define DAV1D_ERR(e) (-(e)) ///< Negate POSIX error code.
#else
#define DAV1D_ERR(e) (e)
#endif

    /**
     * A reference-counted object wrapper for a user-configurable pointer.
     */
    typedef struct Dav1dUserData {
        const uint8_t* data; ///< data pointer
        struct Dav1dRef* ref; ///< allocation origin
    } Dav1dUserData;

    /**
     * Input packet metadata which are copied from the input data used to
     * decode each image into the matching structure of the output image
     * returned back to the user. Since these are metadata fields, they
     * can be used for other purposes than the documented ones, they will
     * still be passed from input data to output picture without being
     * used internally.
     */
    typedef struct Dav1dDataProps {
        int64_t timestamp; ///< container timestamp of input data, INT64_MIN if unknown (default)
        int64_t duration; ///< container duration of input data, 0 if unknown (default)
        int64_t offset; ///< stream offset of input data, -1 if unknown (default)
        size_t size; ///< packet size, default Dav1dData.sz
        struct Dav1dUserData user_data; ///< user-configurable data, default NULL members
    } Dav1dDataProps;


#include <stdint.h>
#include <stddef.h>

    // Constants from Section 3. "Symbols and abbreviated terms"
#define DAV1D_MAX_CDEF_STRENGTHS 8
#define DAV1D_MAX_OPERATING_POINTS 32
#define DAV1D_MAX_TILE_COLS 64
#define DAV1D_MAX_TILE_ROWS 64
#define DAV1D_MAX_SEGMENTS 8
#define DAV1D_NUM_REF_FRAMES 8
#define DAV1D_PRIMARY_REF_NONE 7
#define DAV1D_REFS_PER_FRAME 7
#define DAV1D_TOTAL_REFS_PER_FRAME (DAV1D_REFS_PER_FRAME + 1)

    enum Dav1dObuType {
        DAV1D_OBU_SEQ_HDR = 1,
        DAV1D_OBU_TD = 2,
        DAV1D_OBU_FRAME_HDR = 3,
        DAV1D_OBU_TILE_GRP = 4,
        DAV1D_OBU_METADATA = 5,
        DAV1D_OBU_FRAME = 6,
        DAV1D_OBU_REDUNDANT_FRAME_HDR = 7,
        DAV1D_OBU_PADDING = 15,
    };

    enum Dav1dTxfmMode {
        DAV1D_TX_4X4_ONLY,
        DAV1D_TX_LARGEST,
        DAV1D_TX_SWITCHABLE,
        DAV1D_N_TX_MODES,
    };

    enum Dav1dFilterMode {
        DAV1D_FILTER_8TAP_REGULAR,
        DAV1D_FILTER_8TAP_SMOOTH,
        DAV1D_FILTER_8TAP_SHARP,
        DAV1D_N_SWITCHABLE_FILTERS,
        DAV1D_FILTER_BILINEAR = DAV1D_N_SWITCHABLE_FILTERS,
        DAV1D_N_FILTERS,
        DAV1D_FILTER_SWITCHABLE = DAV1D_N_FILTERS,
    };

    enum Dav1dAdaptiveBoolean {
        DAV1D_OFF = 0,
        DAV1D_ON = 1,
        DAV1D_ADAPTIVE = 2,
    };

    enum Dav1dRestorationType {
        DAV1D_RESTORATION_NONE,
        DAV1D_RESTORATION_SWITCHABLE,
        DAV1D_RESTORATION_WIENER,
        DAV1D_RESTORATION_SGRPROJ,
    };

    enum Dav1dWarpedMotionType {
        DAV1D_WM_TYPE_IDENTITY,
        DAV1D_WM_TYPE_TRANSLATION,
        DAV1D_WM_TYPE_ROT_ZOOM,
        DAV1D_WM_TYPE_AFFINE,
    };

    typedef struct Dav1dWarpedMotionParams {
        enum Dav1dWarpedMotionType type;
        int32_t matrix[6];
        union {
            struct {
                int16_t alpha, beta, gamma, delta;
            } p;
            int16_t abcd[4];
        } u;
    } Dav1dWarpedMotionParams;

    enum Dav1dPixelLayout {
        DAV1D_PIXEL_LAYOUT_I400, ///< monochrome
        DAV1D_PIXEL_LAYOUT_I420, ///< 4:2:0 planar
        DAV1D_PIXEL_LAYOUT_I422, ///< 4:2:2 planar
        DAV1D_PIXEL_LAYOUT_I444, ///< 4:4:4 planar
    };

    enum Dav1dFrameType {
        DAV1D_FRAME_TYPE_KEY = 0,    ///< Key Intra frame
        DAV1D_FRAME_TYPE_INTER = 1,  ///< Inter frame
        DAV1D_FRAME_TYPE_INTRA = 2,  ///< Non key Intra frame
        DAV1D_FRAME_TYPE_SWITCH = 3, ///< Switch Inter frame
    };

    enum Dav1dColorPrimaries {
        DAV1D_COLOR_PRI_BT709 = 1,
        DAV1D_COLOR_PRI_UNKNOWN = 2,
        DAV1D_COLOR_PRI_BT470M = 4,
        DAV1D_COLOR_PRI_BT470BG = 5,
        DAV1D_COLOR_PRI_BT601 = 6,
        DAV1D_COLOR_PRI_SMPTE240 = 7,
        DAV1D_COLOR_PRI_FILM = 8,
        DAV1D_COLOR_PRI_BT2020 = 9,
        DAV1D_COLOR_PRI_XYZ = 10,
        DAV1D_COLOR_PRI_SMPTE431 = 11,
        DAV1D_COLOR_PRI_SMPTE432 = 12,
        DAV1D_COLOR_PRI_EBU3213 = 22,
        DAV1D_COLOR_PRI_RESERVED = 255,
    };

    enum Dav1dTransferCharacteristics {
        DAV1D_TRC_BT709 = 1,
        DAV1D_TRC_UNKNOWN = 2,
        DAV1D_TRC_BT470M = 4,
        DAV1D_TRC_BT470BG = 5,
        DAV1D_TRC_BT601 = 6,
        DAV1D_TRC_SMPTE240 = 7,
        DAV1D_TRC_LINEAR = 8,
        DAV1D_TRC_LOG100 = 9,         ///< logarithmic (100:1 range)
        DAV1D_TRC_LOG100_SQRT10 = 10, ///< lograithmic (100*sqrt(10):1 range)
        DAV1D_TRC_IEC61966 = 11,
        DAV1D_TRC_BT1361 = 12,
        DAV1D_TRC_SRGB = 13,
        DAV1D_TRC_BT2020_10BIT = 14,
        DAV1D_TRC_BT2020_12BIT = 15,
        DAV1D_TRC_SMPTE2084 = 16,     ///< PQ
        DAV1D_TRC_SMPTE428 = 17,
        DAV1D_TRC_HLG = 18,           ///< hybrid log/gamma (BT.2100 / ARIB STD-B67)
        DAV1D_TRC_RESERVED = 255,
    };

    enum Dav1dMatrixCoefficients {
        DAV1D_MC_IDENTITY = 0,
        DAV1D_MC_BT709 = 1,
        DAV1D_MC_UNKNOWN = 2,
        DAV1D_MC_FCC = 4,
        DAV1D_MC_BT470BG = 5,
        DAV1D_MC_BT601 = 6,
        DAV1D_MC_SMPTE240 = 7,
        DAV1D_MC_SMPTE_YCGCO = 8,
        DAV1D_MC_BT2020_NCL = 9,
        DAV1D_MC_BT2020_CL = 10,
        DAV1D_MC_SMPTE2085 = 11,
        DAV1D_MC_CHROMAT_NCL = 12, ///< Chromaticity-derived
        DAV1D_MC_CHROMAT_CL = 13,
        DAV1D_MC_ICTCP = 14,
        DAV1D_MC_RESERVED = 255,
    };

    enum Dav1dChromaSamplePosition {
        DAV1D_CHR_UNKNOWN = 0,
        DAV1D_CHR_VERTICAL = 1,  ///< Horizontally co-located with luma(0, 0)
        ///< sample, between two vertical samples
        DAV1D_CHR_COLOCATED = 2, ///< Co-located with luma(0, 0) sample
    };

    typedef struct Dav1dContentLightLevel {
        int max_content_light_level;
        int max_frame_average_light_level;
    } Dav1dContentLightLevel;

    typedef struct Dav1dMasteringDisplay {
        ///< 0.16 fixed point
        uint16_t primaries[3][2];
        ///< 0.16 fixed point
        uint16_t white_point[2];
        ///< 24.8 fixed point
        uint32_t max_luminance;
        ///< 18.14 fixed point
        uint32_t min_luminance;
    } Dav1dMasteringDisplay;

    typedef struct Dav1dITUTT35 {
        uint8_t  country_code;
        uint8_t  country_code_extension_byte;
        size_t   payload_size;
        uint8_t* payload;
    } Dav1dITUTT35;

    typedef struct Dav1dSequenceHeader {
        /**
         * Stream profile, 0 for 8-10 bits/component 4:2:0 or monochrome;
         * 1 for 8-10 bits/component 4:4:4; 2 for 4:2:2 at any bits/component,
         * or 12 bits/component at any chroma subsampling.
         */
        int profile;
        /**
         * Maximum dimensions for this stream. In non-scalable streams, these
         * are often the actual dimensions of the stream, although that is not
         * a normative requirement.
         */
        int max_width, max_height;
        enum Dav1dPixelLayout layout; ///< format of the picture
        enum Dav1dColorPrimaries pri; ///< color primaries (av1)
        enum Dav1dTransferCharacteristics trc; ///< transfer characteristics (av1)
        enum Dav1dMatrixCoefficients mtrx; ///< matrix coefficients (av1)
        enum Dav1dChromaSamplePosition chr; ///< chroma sample position (av1)
        /**
         * 0, 1 and 2 mean 8, 10 or 12 bits/component, respectively. This is not
         * exactly the same as 'hbd' from the spec; the spec's hbd distinguishes
         * between 8 (0) and 10-12 (1) bits/component, and another element
         * (twelve_bit) to distinguish between 10 and 12 bits/component. To get
         * the spec's hbd, use !!our_hbd, and to get twelve_bit, use hbd == 2.
         */
        int hbd;
        /**
         * Pixel data uses JPEG pixel range ([0,255] for 8bits) instead of
         * MPEG pixel range ([16,235] for 8bits luma, [16,240] for 8bits chroma).
         */
        int color_range;

        int num_operating_points;
        struct Dav1dSequenceHeaderOperatingPoint {
            int major_level, minor_level;
            int initial_display_delay;
            int idc;
            int tier;
            int decoder_model_param_present;
            int display_model_param_present;
        } operating_points[DAV1D_MAX_OPERATING_POINTS];

        int still_picture;
        int reduced_still_picture_header;
        int timing_info_present;
        int num_units_in_tick;
        int time_scale;
        int equal_picture_interval;
        unsigned num_ticks_per_picture;
        int decoder_model_info_present;
        int encoder_decoder_buffer_delay_length;
        int num_units_in_decoding_tick;
        int buffer_removal_delay_length;
        int frame_presentation_delay_length;
        int display_model_info_present;
        int width_n_bits, height_n_bits;
        int frame_id_numbers_present;
        int delta_frame_id_n_bits;
        int frame_id_n_bits;
        int sb128;
        int filter_intra;
        int intra_edge_filter;
        int inter_intra;
        int masked_compound;
        int warped_motion;
        int dual_filter;
        int order_hint;
        int jnt_comp;
        int ref_frame_mvs;
        enum Dav1dAdaptiveBoolean screen_content_tools;
        enum Dav1dAdaptiveBoolean force_integer_mv;
        int order_hint_n_bits;
        int super_res;
        int cdef;
        int restoration;
        int ss_hor, ss_ver, monochrome;
        int color_description_present;
        int separate_uv_delta_q;
        int film_grain_present;

        // Dav1dSequenceHeaders of the same sequence are required to be
        // bit-identical until this offset. See 7.5 "Ordering of OBUs":
        //   Within a particular coded video sequence, the contents of
        //   sequence_header_obu must be bit-identical each time the
        //   sequence header appears except for the contents of
        //   operating_parameters_info.
        struct Dav1dSequenceHeaderOperatingParameterInfo {
            int decoder_buffer_delay;
            int encoder_buffer_delay;
            int low_delay_mode;
        } operating_parameter_info[DAV1D_MAX_OPERATING_POINTS];
    } Dav1dSequenceHeader;

    typedef struct Dav1dSegmentationData {
        int delta_q;
        int delta_lf_y_v, delta_lf_y_h, delta_lf_u, delta_lf_v;
        int ref;
        int skip;
        int globalmv;
    } Dav1dSegmentationData;

    typedef struct Dav1dSegmentationDataSet {
        Dav1dSegmentationData d[DAV1D_MAX_SEGMENTS];
        int preskip;
        int last_active_segid;
    } Dav1dSegmentationDataSet;

    typedef struct Dav1dLoopfilterModeRefDeltas {
        int mode_delta[2 /* is_zeromv */];
        int ref_delta[DAV1D_TOTAL_REFS_PER_FRAME];
    } Dav1dLoopfilterModeRefDeltas;

    typedef struct Dav1dFilmGrainData {
        unsigned seed;
        int num_y_points;
        uint8_t y_points[14][2 /* value, scaling */];
        int chroma_scaling_from_luma;
        int num_uv_points[2];
        uint8_t uv_points[2][10][2 /* value, scaling */];
        int scaling_shift;
        int ar_coeff_lag;
        int8_t ar_coeffs_y[24];
        int8_t ar_coeffs_uv[2][25 + 3 /* padding for alignment purposes */];
        uint64_t ar_coeff_shift;
        int grain_scale_shift;
        int uv_mult[2];
        int uv_luma_mult[2];
        int uv_offset[2];
        int overlap_flag;
        int clip_to_restricted_range;
    } Dav1dFilmGrainData;

    typedef struct Dav1dFrameHeader {
        struct {
            Dav1dFilmGrainData data;
            int present, update;
        } film_grain; ///< film grain parameters
        enum Dav1dFrameType frame_type; ///< type of the picture
        int width[2 /* { coded_width, superresolution_upscaled_width } */], height;
        int frame_offset; ///< frame number
        int temporal_id; ///< temporal id of the frame for SVC
        int spatial_id; ///< spatial id of the frame for SVC

        int show_existing_frame;
        int existing_frame_idx;
        int frame_id;
        int frame_presentation_delay;
        int show_frame;
        int showable_frame;
        int error_resilient_mode;
        int disable_cdf_update;
        int allow_screen_content_tools;
        int force_integer_mv;
        int frame_size_override;
        int primary_ref_frame;
        int buffer_removal_time_present;
        struct Dav1dFrameHeaderOperatingPoint {
            int buffer_removal_time;
        } operating_points[DAV1D_MAX_OPERATING_POINTS];
        int refresh_frame_flags;
        int render_width, render_height;
        struct {
            int width_scale_denominator;
            int enabled;
        } super_res;
        int have_render_size;
        int allow_intrabc;
        int frame_ref_short_signaling;
        int refidx[DAV1D_REFS_PER_FRAME];
        int hp;
        enum Dav1dFilterMode subpel_filter_mode;
        int switchable_motion_mode;
        int use_ref_frame_mvs;
        int refresh_context;
        struct {
            int uniform;
            unsigned n_bytes;
            int min_log2_cols, max_log2_cols, log2_cols, cols;
            int min_log2_rows, max_log2_rows, log2_rows, rows;
            uint16_t col_start_sb[DAV1D_MAX_TILE_COLS + 1];
            uint16_t row_start_sb[DAV1D_MAX_TILE_ROWS + 1];
            int update;
        } tiling;
        struct {
            int yac;
            int ydc_delta;
            int udc_delta, uac_delta, vdc_delta, vac_delta;
            int qm, qm_y, qm_u, qm_v;
        } quant;
        struct {
            int enabled, update_map, temporal, update_data;
            Dav1dSegmentationDataSet seg_data;
            int lossless[DAV1D_MAX_SEGMENTS], qidx[DAV1D_MAX_SEGMENTS];
        } segmentation;
        struct {
            struct {
                int present;
                int res_log2;
            } q;
            struct {
                int present;
                int res_log2;
                int multi;
            } lf;
        } delta;
        int all_lossless;
        struct {
            int level_y[2 /* dir */];
            int level_u, level_v;
            int mode_ref_delta_enabled;
            int mode_ref_delta_update;
            Dav1dLoopfilterModeRefDeltas mode_ref_deltas;
            int sharpness;
        } loopfilter;
        struct {
            int damping;
            int n_bits;
            int y_strength[DAV1D_MAX_CDEF_STRENGTHS];
            int uv_strength[DAV1D_MAX_CDEF_STRENGTHS];
        } cdef;
        struct {
            enum Dav1dRestorationType type[3 /* plane */];
            int unit_size[2 /* y, uv */];
        } restoration;
        enum Dav1dTxfmMode txfm_mode;
        int switchable_comp_refs;
        int skip_mode_allowed, skip_mode_enabled, skip_mode_refs[2];
        int warp_motion;
        int reduced_txtp_set;
        Dav1dWarpedMotionParams gmv[DAV1D_REFS_PER_FRAME];
    } Dav1dFrameHeader;



    /* Number of bytes to align AND pad picture memory buffers by, so that SIMD
     * implementations can over-read by a few bytes, and use aligned read/write
     * instructions. */
#define DAV1D_PICTURE_ALIGNMENT 64

    typedef struct Dav1dPictureParameters {
        int w; ///< width (in pixels)
        int h; ///< height (in pixels)
        enum Dav1dPixelLayout layout; ///< format of the picture
        int bpc; ///< bits per pixel component (8 or 10)
    } Dav1dPictureParameters;

    typedef struct Dav1dPicture {
        Dav1dSequenceHeader* seq_hdr;
        Dav1dFrameHeader* frame_hdr;

        /**
         * Pointers to planar image data (Y is [0], U is [1], V is [2]). The data
         * should be bytes (for 8 bpc) or words (for 10 bpc). In case of words
         * containing 10 bpc image data, the pixels should be located in the LSB
         * bits, so that values range between [0, 1023]; the upper bits should be
         * zero'ed out.
         */
        void* data[3];

        /**
         * Number of bytes between 2 lines in data[] for luma [0] or chroma [1].
         */
        ptrdiff_t stride[2];

        Dav1dPictureParameters p;
        Dav1dDataProps m;

        /**
         * High Dynamic Range Content Light Level metadata applying to this picture,
         * as defined in section 5.8.3 and 6.7.3
         */
        Dav1dContentLightLevel* content_light;
        /**
         * High Dynamic Range Mastering Display Color Volume metadata applying to
         * this picture, as defined in section 5.8.4 and 6.7.4
         */
        Dav1dMasteringDisplay* mastering_display;
        /**
         * ITU-T T.35 metadata as defined in section 5.8.2 and 6.7.2
         */
        Dav1dITUTT35* itut_t35;

        uintptr_t reserved[4]; ///< reserved for future use

        struct Dav1dRef* frame_hdr_ref; ///< Dav1dFrameHeader allocation origin
        struct Dav1dRef* seq_hdr_ref; ///< Dav1dSequenceHeader allocation origin
        struct Dav1dRef* content_light_ref; ///< Dav1dContentLightLevel allocation origin
        struct Dav1dRef* mastering_display_ref; ///< Dav1dMasteringDisplay allocation origin
        struct Dav1dRef* itut_t35_ref; ///< Dav1dITUTT35 allocation origin
        uintptr_t reserved_ref[4]; ///< reserved for future use
        struct Dav1dRef* ref; ///< Frame data allocation origin

        void* allocator_data; ///< pointer managed by the allocator
    } Dav1dPicture;

    typedef struct Dav1dPicAllocator {
        void* cookie; ///< custom data to pass to the allocator callbacks.
        /**
         * Allocate the picture buffer based on the Dav1dPictureParameters.
         *
         * The data[0], data[1] and data[2] must be DAV1D_PICTURE_ALIGNMENT byte
         * aligned and with a pixel width/height multiple of 128 pixels. Any
         * allocated memory area should also be padded by DAV1D_PICTURE_ALIGNMENT
         * bytes.
         * data[1] and data[2] must share the same stride[1].
         *
         * This function will be called on the main thread (the thread which calls
         * dav1d_get_picture()).
         *
         * @param  pic The picture to allocate the buffer for. The callback needs to
         *             fill the picture data[0], data[1], data[2], stride[0] and
         *             stride[1].
         *             The allocator can fill the pic allocator_data pointer with
         *             a custom pointer that will be passed to
         *             release_picture_callback().
         * @param cookie Custom pointer passed to all calls.
         *
         * @note No fields other than data, stride and allocator_data must be filled
         *       by this callback.
         * @return 0 on success. A negative DAV1D_ERR value on error.
         */
        int (*alloc_picture_callback)(Dav1dPicture* pic, void* cookie);
        /**
         * Release the picture buffer.
         *
         * If frame threading is used, this function may be called by the main
         * thread (the thread which calls dav1d_get_picture()) or any of the frame
         * threads and thus must be thread-safe. If frame threading is not used,
         * this function will only be called on the main thread.
         *
         * @param pic    The picture that was filled by alloc_picture_callback().
         * @param cookie Custom pointer passed to all calls.
         */
        void (*release_picture_callback)(Dav1dPicture* pic, void* cookie);
    } Dav1dPicAllocator;

    /**
     * Release reference to a picture.
     */
    DAV1D_API void dav1d_picture_unref(Dav1dPicture* p);





#include <stddef.h>
#include <stdint.h>

    typedef struct Dav1dData {
        const uint8_t* data; ///< data pointer
        size_t sz; ///< data size
        struct Dav1dRef* ref; ///< allocation origin
        Dav1dDataProps m; ///< user provided metadata passed to the output picture
    } Dav1dData;

    /**
     * Allocate data.
     *
     * @param data Input context.
     * @param   sz Size of the data that should be allocated.
     *
     * @return Pointer to the allocated buffer on success. NULL on error.
     */
    DAV1D_API uint8_t* dav1d_data_create(Dav1dData* data, size_t sz);

    /**
     * Wrap an existing data array.
     *
     * @param          data Input context.
     * @param           buf The data to be wrapped.
     * @param            sz Size of the data.
     * @param free_callback Function to be called when we release our last
     *                      reference to this data. In this callback, $buf will be
     *                      the $buf argument to this function, and $cookie will
     *                      be the $cookie input argument to this function.
     * @param        cookie Opaque parameter passed to free_callback().
     *
     * @return 0 on success. A negative DAV1D_ERR value on error.
     */
    DAV1D_API int dav1d_data_wrap(Dav1dData* data, const uint8_t* buf, size_t sz,
        void (*free_callback)(const uint8_t* buf, void* cookie),
        void* cookie);

    /**
     * Wrap a user-provided data pointer into a reference counted object.
     *
     * data->m.user_data field will initialized to wrap the provided $user_data
     * pointer.
     *
     * $free_callback will be called on the same thread that released the last
     * reference. If frame threading is used, make sure $free_callback is
     * thread-safe.
     *
     * @param          data Input context.
     * @param     user_data The user data to be wrapped.
     * @param free_callback Function to be called when we release our last
     *                      reference to this data. In this callback, $user_data
     *                      will be the $user_data argument to this function, and
     *                      $cookie will be the $cookie input argument to this
     *                      function.
     * @param        cookie Opaque parameter passed to $free_callback.
     *
     * @return 0 on success. A negative DAV1D_ERR value on error.
     */
    DAV1D_API int dav1d_data_wrap_user_data(Dav1dData* data,
        const uint8_t* user_data,
        void (*free_callback)(const uint8_t* user_data,
            void* cookie),
        void* cookie);

    /**
     * Free the data reference.
     *
     * The reference count for data->m.user_data will be decremented (if it has been
     * initialized with dav1d_data_wrap_user_data). The $data object will be memset
     * to 0.
     *
     * @param data Input context.
     */
    DAV1D_API void dav1d_data_unref(Dav1dData* data);


#define DAV1D_API_VERSION_MAJOR 5
#define DAV1D_API_VERSION_MINOR 1
#define DAV1D_API_VERSION_PATCH 0

    typedef struct Dav1dContext Dav1dContext;
    typedef struct Dav1dRef Dav1dRef;

#define DAV1D_MAX_FRAME_THREADS 256
#define DAV1D_MAX_TILE_THREADS 64
#define DAV1D_MAX_POSTFILTER_THREADS 256

    typedef struct Dav1dLogger {
        void* cookie; ///< Custom data to pass to the callback.
        /**
         * Logger callback. May be NULL to disable logging.
         *
         * @param cookie Custom pointer passed to all calls.
         * @param format The vprintf compatible format string.
         * @param     ap List of arguments referenced by the format string.
         */
        void (*callback)(void* cookie, const char* format, va_list ap);
    } Dav1dLogger;

    typedef struct Dav1dSettings {
        int n_frame_threads;
        int n_tile_threads;
        int apply_grain;
        int operating_point; ///< select an operating point for scalable AV1 bitstreams (0 - 31)
        int all_layers; ///< output all spatial layers of a scalable AV1 biststream
        unsigned frame_size_limit; ///< maximum frame size, in pixels (0 = unlimited)
        Dav1dPicAllocator allocator; ///< Picture allocator callback.
        Dav1dLogger logger; ///< Logger callback.
        int n_postfilter_threads;
        uint8_t reserved[28]; ///< reserved for future use
    } Dav1dSettings;

    /**
     * Get library version.
     */
    DAV1D_API const char* dav1d_version(void);

    /**
     * Initialize settings to default values.
     *
     * @param s Input settings context.
     */
    DAV1D_API void dav1d_default_settings(Dav1dSettings* s);

    /**
     * Allocate and open a decoder instance.
     *
     * @param c_out The decoder instance to open. *c_out will be set to the
     *              allocated context.
     * @param     s Input settings context.
     *
     * @note The context must be freed using dav1d_close() when decoding is
     *       finished.
     *
     * @return 0 on success, or < 0 (a negative DAV1D_ERR code) on error.
     */
    DAV1D_API int dav1d_open(Dav1dContext** c_out, const Dav1dSettings* s);

    /**
     * Parse a Sequence Header OBU from bitstream data.
     *
     * @param out Output Sequence Header.
     * @param buf The data to be parser.
     * @param sz  Size of the data.
     *
     * @return 0 on success, or < 0 (a negative DAV1D_ERR code) on error.
     *
     * @note It is safe to feed this function data containing other OBUs than a
     *       Sequence Header, as they will simply be ignored. If there is more than
     *       one Sequence Header OBU present, only the last will be returned.
     */
    DAV1D_API int dav1d_parse_sequence_header(Dav1dSequenceHeader* out,
        const uint8_t* buf, const size_t sz);

    /**
     * Feed bitstream data to the decoder.
     *
     * @param   c Input decoder instance.
     * @param  in Input bitstream data. On success, ownership of the reference is
     *            passed to the library.
     *
     * @return
     *         0: Success, and the data was consumed.
     *  DAV1D_ERR(EAGAIN): The data can't be consumed. dav1d_get_picture() should
     *                     be called to get one or more frames before the function
     *                     can consume new data.
     *  other negative DAV1D_ERR codes: Error during decoding or because of invalid
     *                                  passed-in arguments.
     */
    DAV1D_API int dav1d_send_data(Dav1dContext* c, Dav1dData* in);

    /**
     * Return a decoded picture.
     *
     * @param   c Input decoder instance.
     * @param out Output frame. The caller assumes ownership of the returned
     *            reference.
     *
     * @return
     *         0: Success, and a frame is returned.
     *  DAV1D_ERR(EAGAIN): Not enough data to output a frame. dav1d_send_data()
     *                     should be called with new input.
     *  other negative DAV1D_ERR codes: Error during decoding or because of invalid
     *                                  passed-in arguments.
     *
     * @note To drain buffered frames from the decoder (i.e. on end of stream),
     *       call this function until it returns DAV1D_ERR(EAGAIN).
     *
     * @code{.c}
     *  Dav1dData data = { 0 };
     *  Dav1dPicture p = { 0 };
     *  int res;
     *
     *  read_data(&data);
     *  do {
     *      res = dav1d_send_data(c, &data);
     *      // Keep going even if the function can't consume the current data
     *         packet. It eventually will after one or more frames have been
     *         returned in this loop.
     *      if (res < 0 && res != DAV1D_ERR(EAGAIN))
     *          free_and_abort();
     *      res = dav1d_get_picture(c, &p);
     *      if (res < 0) {
     *          if (res != DAV1D_ERR(EAGAIN))
     *              free_and_abort();
     *      } else
     *          output_and_unref_picture(&p);
     *  // Stay in the loop as long as there's data to consume.
     *  } while (data.sz || read_data(&data) == SUCCESS);
     *
     *  // Handle EOS by draining all buffered frames.
     *  do {
     *      res = dav1d_get_picture(c, &p);
     *      if (res < 0) {
     *          if (res != DAV1D_ERR(EAGAIN))
     *              free_and_abort();
     *      } else
     *          output_and_unref_picture(&p);
     *  } while (res == 0);
     * @endcode
     */
    DAV1D_API int dav1d_get_picture(Dav1dContext* c, Dav1dPicture* out);

    /**
     * Close a decoder instance and free all associated memory.
     *
     * @param c_out The decoder instance to close. *c_out will be set to NULL.
     */
    DAV1D_API void dav1d_close(Dav1dContext** c_out);

    /**
     * Flush all delayed frames in decoder and clear internal decoder state,
     * to be used when seeking.
     *
     * @param c Input decoder instance.
     *
     * @note Decoding will start only after a valid sequence header OBU is
     *       delivered to dav1d_send_data().
     *
     */
    DAV1D_API void dav1d_flush(Dav1dContext* c);

    enum Dav1dEventFlags {
        /**
         * The last returned picture contains a reference to a new Sequence Header,
         * either because it's the start of a new coded sequence, or the decoder was
         * flushed before it was generated.
         */
        DAV1D_EVENT_FLAG_NEW_SEQUENCE = 1 << 0,
        /**
         * The last returned picture contains a reference to a Sequence Header with
         * new operating parameters information for the current coded sequence.
         */
        DAV1D_EVENT_FLAG_NEW_OP_PARAMS_INFO = 1 << 1,
    };

    /**
     * Fetch a combination of DAV1D_EVENT_FLAG_* event flags generated by the decoding
     * process.
     *
     * @param c Input decoder instance.
     * @param flags Where to write the flags.
     *
     * @return 0 on success, or < 0 (a negative DAV1D_ERR code) on error.
     *
     * @note Calling this function will clear all the event flags currently stored in
     *       the decoder.
     *
     */
    DAV1D_API int dav1d_get_event_flags(Dav1dContext* c, enum Dav1dEventFlags* flags);

# ifdef __cplusplus
}
# endif

#endif /* DAV1D_H */
