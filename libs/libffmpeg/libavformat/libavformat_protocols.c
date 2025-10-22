/*
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "ffmpeg-config.h"

#include "libavutil/avstring.h"
#include "libavutil/mem.h"

#include "url.h"

extern const URLProtocol ff_async_protocol;
extern const URLProtocol ff_bluray_protocol;
extern const URLProtocol ff_cache_protocol;
extern const URLProtocol ff_concat_protocol;
extern const URLProtocol ff_crypto_protocol;
extern const URLProtocol ff_data_protocol;
extern const URLProtocol ff_ffrtmpcrypt_protocol;
extern const URLProtocol ff_ffrtmphttp_protocol;
extern const URLProtocol ff_file_protocol;
extern const URLProtocol ff_ftp_protocol;
extern const URLProtocol ff_gopher_protocol;
extern const URLProtocol ff_hls_protocol;
extern const URLProtocol ff_http_protocol;
extern const URLProtocol ff_httpproxy_protocol;
extern const URLProtocol ff_https_protocol;
extern const URLProtocol ff_icecast_protocol;
extern const URLProtocol ff_mmsh_protocol;
extern const URLProtocol ff_mmst_protocol;
extern const URLProtocol ff_md5_protocol;
extern const URLProtocol ff_pipe_protocol;
extern const URLProtocol ff_prompeg_protocol;
extern const URLProtocol ff_rtmp_protocol;
extern const URLProtocol ff_rtmpe_protocol;
extern const URLProtocol ff_rtmps_protocol;
extern const URLProtocol ff_rtmpt_protocol;
extern const URLProtocol ff_rtmpte_protocol;
extern const URLProtocol ff_rtmpts_protocol;
extern const URLProtocol ff_rtp_protocol;
extern const URLProtocol ff_sctp_protocol;
extern const URLProtocol ff_srtp_protocol;
extern const URLProtocol ff_subfile_protocol;
extern const URLProtocol ff_tee_protocol;
extern const URLProtocol ff_tcp_protocol;
extern const URLProtocol ff_tls_protocol;
extern const URLProtocol ff_udp_protocol;
extern const URLProtocol ff_udplite_protocol;
extern const URLProtocol ff_unix_protocol;
extern const URLProtocol ff_libamqp_protocol;
extern const URLProtocol ff_librtmp_protocol;
extern const URLProtocol ff_librtmpe_protocol;
extern const URLProtocol ff_librtmps_protocol;
extern const URLProtocol ff_librtmpt_protocol;
extern const URLProtocol ff_librtmpte_protocol;
extern const URLProtocol ff_libsrt_protocol;
extern const URLProtocol ff_libssh_protocol;
extern const URLProtocol ff_libsmbclient_protocol;
extern const URLProtocol ff_libzmq_protocol;

static const URLProtocol* const url_protocols[] = {
&ff_async_protocol,
& ff_bluray_protocol,
& ff_cache_protocol,
& ff_concat_protocol,
& ff_crypto_protocol,
& ff_data_protocol,
& ff_ffrtmpcrypt_protocol,
& ff_ffrtmphttp_protocol,
& ff_file_protocol,
& ff_ftp_protocol,
& ff_gopher_protocol,
& ff_hls_protocol,
& ff_http_protocol,
& ff_httpproxy_protocol,
& ff_https_protocol,
& ff_icecast_protocol,
& ff_mmsh_protocol,
& ff_mmst_protocol,
& ff_md5_protocol,
& ff_pipe_protocol,
& ff_prompeg_protocol,
& ff_rtmp_protocol,
& ff_rtmpe_protocol,
& ff_rtmps_protocol,
& ff_rtmpt_protocol,
& ff_rtmpte_protocol,
& ff_rtmpts_protocol,
& ff_rtp_protocol,
#if CONFIG_SCTP_PROTOCOL
& ff_sctp_protocol,
#endif
& ff_srtp_protocol,
& ff_subfile_protocol,
& ff_tee_protocol,
& ff_tcp_protocol,
& ff_tls_protocol,
& ff_udp_protocol,
& ff_udplite_protocol,
#if CONFIG_UNIX_PROTOCOL
& ff_unix_protocol,
#endif
#if CONFIG_LIBAMQP_PROTOCOL
& ff_libamqp_protocol,
#endif
#if CONFIG_LIBRTMP
& ff_librtmp_protocol,
& ff_librtmpe_protocol,
& ff_librtmps_protocol,
& ff_librtmpt_protocol,
& ff_librtmpte_protocol,
#endif
#if CONFIG_LIBSRT
& ff_libsrt_protocol,
#endif
#if CONFIG_LIBSSH
& ff_libssh_protocol,
#endif
#if CONFIG_LIBSMBCLIENT
& ff_libsmbclient_protocol,
#endif
#if CONFIG_LIBZMQ
& ff_libzmq_protocol,
#endif
    NULL };


#if FF_API_CHILD_CLASS_NEXT
const AVClass *ff_urlcontext_child_class_next(const AVClass *prev)
{
    int i;

    /* find the protocol that corresponds to prev */
    for (i = 0; prev && url_protocols[i]; i++) {
        if (url_protocols[i]->priv_data_class == prev) {
            i++;
            break;
        }
    }

    /* find next protocol with priv options */
    for (; url_protocols[i]; i++)
        if (url_protocols[i]->priv_data_class)
            return url_protocols[i]->priv_data_class;
    return NULL;
}
#endif

const AVClass *ff_urlcontext_child_class_iterate(void **iter)
{
    const AVClass *ret = NULL;
    uintptr_t i;

    for (i = (uintptr_t)*iter; url_protocols[i]; i++) {
        ret = url_protocols[i]->priv_data_class;
        if (ret)
            break;
    }

    *iter = (void*)(uintptr_t)(url_protocols[i] ? i + 1 : i);
    return ret;
}

const char *avio_enum_protocols(void **opaque, int output)
{
    const URLProtocol **p = *opaque;

    p = p ? p + 1 : url_protocols;
    *opaque = p;
    if (!*p) {
        *opaque = NULL;
        return NULL;
    }
    if ((output && (*p)->url_write) || (!output && (*p)->url_read))
        return (*p)->name;
    return avio_enum_protocols(opaque, output);
}

const AVClass *avio_protocol_get_class(const char *name)
{
    int i = 0;
    for (i = 0; url_protocols[i]; i++) {
        if (!strcmp(url_protocols[i]->name, name))
            return url_protocols[i]->priv_data_class;
    }
    return NULL;
}

const URLProtocol **ffurl_get_protocols(const char *whitelist,
                                        const char *blacklist)
{
    const URLProtocol **ret;
    int i, ret_idx = 0;

    ret = av_mallocz_array(FF_ARRAY_ELEMS(url_protocols), sizeof(*ret));
    if (!ret)
        return NULL;

    for (i = 0; url_protocols[i]; i++) {
        const URLProtocol *up = url_protocols[i];

        if (whitelist && *whitelist && !av_match_name(up->name, whitelist))
            continue;
        if (blacklist && *blacklist && av_match_name(up->name, blacklist))
            continue;

        ret[ret_idx++] = up;
    }

    return ret;
}
