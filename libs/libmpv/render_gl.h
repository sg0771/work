/* Copyright (C) 2018 the mpv developers
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef MPV_CLIENT_API_RENDER_GL_H_
#define MPV_CLIENT_API_RENDER_GL_H_

#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct mpv_opengl_init_params {
    
    void *(*get_proc_address)(void *ctx, const char *name);
    
    void *get_proc_address_ctx;
} mpv_opengl_init_params;

typedef struct mpv_opengl_fbo {
    
    int fbo;
    
    int w, h;
    
    int internal_format;
} mpv_opengl_fbo;

typedef struct mpv_opengl_drm_params {
    int fd;
    int crtc_id;
    int connector_id;
    struct _drmModeAtomicReq **atomic_request_ptr;
    int render_fd;
} mpv_opengl_drm_params;


typedef struct mpv_opengl_drm_draw_surface_size {
    /**
     * size of the draw plane surface in pixels.
     */
    int width, height;
} mpv_opengl_drm_draw_surface_size;

typedef struct mpv_opengl_drm_params_v2 {
    /**
     * DRM fd (int). Set to -1 if invalid.
     */
    int fd;

    /**
     * Currently used crtc id
     */
    int crtc_id;

    /**
     * Currently used connector id
     */
    int connector_id;

    /**
     * Pointer to a drmModeAtomicReq pointer that is being used for the renderloop.
     * This pointer should hold a pointer to the atomic request pointer
     * The atomic request pointer is usually changed at every renderloop.
     */
    struct _drmModeAtomicReq **atomic_request_ptr;

    /**
     * DRM render node. Used for VAAPI interop.
     * Set to -1 if invalid.
     */
    int render_fd;
} mpv_opengl_drm_params_v2;

#define mpv_opengl_drm_osd_size mpv_opengl_drm_draw_surface_size

#ifdef __cplusplus
}
#endif

#endif
