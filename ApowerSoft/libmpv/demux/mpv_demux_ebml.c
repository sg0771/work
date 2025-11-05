/*
 * native ebml reader for the Matroska demuxer
 * new parser copyright (c) 2010 Uoti Urpala
 * copyright (c) 2004 Aurelien Jacobs <aurel@gnuage.org>
 * based on the one written by Ronald Bultje for gstreamer
 *
 * This file is part of mpv.
 *
 * mpv is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * mpv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with mpv.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mpv-config.h"

#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stddef.h>
#include <assert.h>

#include <libavutil/intfloat.h>
#include <libavutil/common.h>
#include "mpv_talloc.h"
#include "ebml.h"
#include "stream/stream.h"
#include "common/msg.h"

// Whether the id is a known Matroska level 1 element (allowed as element on
// global file level, after the level 0 MATROSKA_ID_SEGMENT).
// This (intentionally) doesn't include "global" elements.
bool ebml_is_mkv_level1_id(uint32_t id)
{
    switch (id) {
    case MATROSKA_ID_SEEKHEAD:
    case MATROSKA_ID_INFO:
    case MATROSKA_ID_CLUSTER:
    case MATROSKA_ID_TRACKS:
    case MATROSKA_ID_CUES:
    case MATROSKA_ID_ATTACHMENTS:
    case MATROSKA_ID_CHAPTERS:
    case MATROSKA_ID_TAGS:
        return true;
    default:
        return false;
    }
}

/*
 * Read: the element content data ID.
 * Return: the ID.
 */
uint32_t ebml_read_id(stream_t *s)
{
    int i, len_mask = 0x80;
    uint32_t id;

    for (i = 0, id = stream_read_char(s); i < 4 && !(id & len_mask); i++)
        len_mask >>= 1;
    if (i >= 4)
        return EBML_ID_INVALID;
    while (i--)
        id = (id << 8) | stream_read_char(s);
    return id;
}

/*
 * Read: element content length.
 */
uint64_t ebml_read_length(stream_t *s)
{
    int i, j, num_ffs = 0, len_mask = 0x80;
    uint64_t len;

    for (i = 0, len = stream_read_char(s); i < 8 && !(len & len_mask); i++)
        len_mask >>= 1;
    if (i >= 8)
        return EBML_UINT_INVALID;
    j = i + 1;
    if ((int) (len &= (len_mask - 1)) == len_mask - 1)
        num_ffs++;
    while (i--) {
        len = (len << 8) | stream_read_char(s);
        if ((len & 0xFF) == 0xFF)
            num_ffs++;
    }
    if (j == num_ffs)
        return EBML_UINT_INVALID;
    if (len >= 1ULL<<63)   // Can happen if stream_read_char returns EOF
        return EBML_UINT_INVALID;
    return len;
}


/*
 * Read a variable length signed int.
 */
int64_t ebml_read_signed_length(stream_t *s)
{
    uint64_t unum;
    int l;

    /* read as unsigned number first */
    uint64_t offset = stream_tell(s);
    unum = ebml_read_length(s);
    if (unum == EBML_UINT_INVALID)
        return EBML_INT_INVALID;
    l = stream_tell(s) - offset;

    return unum - ((1LL << ((7 * l) - 1)) - 1);
}

/*
 * Read the next element as an unsigned int.
 */
uint64_t ebml_read_uint(stream_t *s)
{
    uint64_t len, value = 0;

    len = ebml_read_length(s);
    if (len == EBML_UINT_INVALID || len > 8)
        return EBML_UINT_INVALID;

    while (len--)
        value = (value << 8) | stream_read_char(s);

    return value;
}

/*
 * Read the next element as a signed int.
 */
int64_t ebml_read_int(stream_t *s)
{
    uint64_t value = 0;
    uint64_t len;
    int l;

    len = ebml_read_length(s);
    if (len == EBML_UINT_INVALID || len > 8)
        return EBML_INT_INVALID;
    if (!len)
        return 0;

    len--;
    l = stream_read_char(s);
    if (l & 0x80)
        value = -1;
    value = (value << 8) | l;
    while (len--)
        value = (value << 8) | stream_read_char(s);

    return (int64_t)value; // assume complement of 2
}

/*
 * Skip the current element.
 * end: the end of the parent element or -1 (for robust error handling)
 */
int ebml_read_skip(struct mp_log *log, int64_t end, stream_t *s)
{
    uint64_t len;

    int64_t pos = stream_tell(s);

    len = ebml_read_length(s);
    if (len == EBML_UINT_INVALID)
        goto invalid;

    int64_t pos2 = stream_tell(s);
    if (len >= INT64_MAX - pos2 || (end > 0 && pos2 + len > end))
        goto invalid;

    if (!stream_seek_skip(s, pos2 + len))
        goto invalid;

    return 0;

invalid:
    mp_err(log, "Invalid EBML length at position %"PRId64"\n", pos);
    stream_seek_skip(s, pos);
    return 1;
}

/*
 * Skip to (probable) next cluster (MATROSKA_ID_CLUSTER) element start position.
 */
int ebml_resync_cluster(struct mp_log *log, stream_t *s)
{
    int64_t pos = stream_tell(s);
    uint32_t last_4_bytes = 0;
    stream_read_peek(s, &(char){0}, 1);
    if (!s->eof) {
        mp_err(log, "Corrupt file detected. "
               "Trying to resync starting from position %"PRId64"...\n", pos);
    }
    while (!s->eof) {
        // Assumes MATROSKA_ID_CLUSTER is 4 bytes, with no 0 bytes.
        if (last_4_bytes == MATROSKA_ID_CLUSTER) {
            mp_err(log, "Cluster found at %"PRId64".\n", pos - 4);
            stream_seek(s, pos - 4);
            return 0;
        }
        last_4_bytes = (last_4_bytes << 8) | stream_read_char(s);
        pos++;
    }
    return -1;
}



#define EVALARGS(F, ...) F(__VA_ARGS__)
#define E(str, N, type) const struct ebml_elem_desc ebml_ ## N ## _desc = { str, type };
#define E_SN(str, count, N) const struct ebml_elem_desc ebml_ ## N ## _desc = { str, EBML_TYPE_SUBELEMENTS, sizeof(struct ebml_ ## N), count, (const struct ebml_field_desc[]){
#define E_S(str, count) EVALARGS(E_SN, str, count, N)
#define FN(id, name, multiple, N) { id, multiple, offsetof(struct ebml_ ## N, name), offsetof(struct ebml_ ## N, n_ ## name), &ebml_##name##_desc},
#define F(id, name, multiple) EVALARGS(FN, id, name, multiple, N)
// Generated by TOOLS/matroska.py, do not edit manually


E("TagDefault", tag_default, EBML_TYPE_UINT)

E("TagString", tag_string, EBML_TYPE_STR)

E("TagLanguage", tag_language, EBML_TYPE_STR)

E("TagName", tag_name, EBML_TYPE_STR)

#define N simple_tag
E_S("SimpleTag", 4)
F(MATROSKA_ID_TAGNAME, tag_name, 0)
F(MATROSKA_ID_TAGLANGUAGE, tag_language, 0)
F(MATROSKA_ID_TAGSTRING, tag_string, 0)
F(MATROSKA_ID_TAGDEFAULT, tag_default, 0)
}
};
#undef N

E("TargetAttachmentUID", target_attachment_uid, EBML_TYPE_UINT)

E("TargetChapterUID", target_chapter_uid, EBML_TYPE_UINT)

E("TargetEditionUID", target_edition_uid, EBML_TYPE_UINT)

E("TargetTrackUID", target_track_uid, EBML_TYPE_UINT)

E("TargetType", target_type, EBML_TYPE_STR)

E("TargetTypeValue", target_type_value, EBML_TYPE_UINT)

#define N targets
E_S("Targets", 6)
F(MATROSKA_ID_TARGETTYPEVALUE, target_type_value, 0)
F(MATROSKA_ID_TARGETTYPE, target_type, 0)
F(MATROSKA_ID_TARGETTRACKUID, target_track_uid, 0)
F(MATROSKA_ID_TARGETEDITIONUID, target_edition_uid, 0)
F(MATROSKA_ID_TARGETCHAPTERUID, target_chapter_uid, 0)
F(MATROSKA_ID_TARGETATTACHMENTUID, target_attachment_uid, 0)
}};
#undef N

#define N tag
E_S("Tag", 2)
F(MATROSKA_ID_TARGETS, targets, 0)
F(MATROSKA_ID_SIMPLETAG, simple_tag, 1)
}};
#undef N

#define N tags
E_S("Tags", 1)
F(MATROSKA_ID_TAG, tag, 1)
}};
#undef N

E("ChapCountry", chap_country, EBML_TYPE_STR)

E("ChapLanguage", chap_language, EBML_TYPE_STR)

E("ChapString", chap_string, EBML_TYPE_STR)

#define N chapter_display
E_S("ChapterDisplay", 3)
F(MATROSKA_ID_CHAPSTRING, chap_string, 0)
F(MATROSKA_ID_CHAPLANGUAGE, chap_language, 1)
F(MATROSKA_ID_CHAPCOUNTRY, chap_country, 1)
}};
#undef N

E("ChapterSegmentEditionUID", chapter_segment_edition_uid, EBML_TYPE_UINT)

E("ChapterSegmentUID", chapter_segment_uid, EBML_TYPE_BINARY)

E("ChapterFlagEnabled", chapter_flag_enabled, EBML_TYPE_UINT)

E("ChapterFlagHidden", chapter_flag_hidden, EBML_TYPE_UINT)

E("ChapterTimeEnd", chapter_time_end, EBML_TYPE_UINT)

E("ChapterTimeStart", chapter_time_start, EBML_TYPE_UINT)

E("ChapterUID", chapter_uid, EBML_TYPE_UINT)

#define N chapter_atom
E_S("ChapterAtom", 8)
F(MATROSKA_ID_CHAPTERUID, chapter_uid, 0)
F(MATROSKA_ID_CHAPTERTIMESTART, chapter_time_start, 0)
F(MATROSKA_ID_CHAPTERTIMEEND, chapter_time_end, 0)
F(MATROSKA_ID_CHAPTERFLAGHIDDEN, chapter_flag_hidden, 0)
F(MATROSKA_ID_CHAPTERFLAGENABLED, chapter_flag_enabled, 0)
F(MATROSKA_ID_CHAPTERSEGMENTUID, chapter_segment_uid, 0)
F(MATROSKA_ID_CHAPTERSEGMENTEDITIONUID, chapter_segment_edition_uid, 0)
F(MATROSKA_ID_CHAPTERDISPLAY, chapter_display, 1)
}};
#undef N

E("EditionFlagOrdered", edition_flag_ordered, EBML_TYPE_UINT)

E("EditionFlagDefault", edition_flag_default, EBML_TYPE_UINT)

E("EditionFlagHidden", edition_flag_hidden, EBML_TYPE_UINT)

E("EditionUID", edition_uid, EBML_TYPE_UINT)

#define N edition_entry
E_S("EditionEntry", 5)
F(MATROSKA_ID_EDITIONUID, edition_uid, 0)
F(MATROSKA_ID_EDITIONFLAGHIDDEN, edition_flag_hidden, 0)
F(MATROSKA_ID_EDITIONFLAGDEFAULT, edition_flag_default, 0)
F(MATROSKA_ID_EDITIONFLAGORDERED, edition_flag_ordered, 0)
F(MATROSKA_ID_CHAPTERATOM, chapter_atom, 1)
}};
#undef N

#define N chapters
E_S("Chapters", 1)
F(MATROSKA_ID_EDITIONENTRY, edition_entry, 1)
}};
#undef N

E("FileUID", file_uid, EBML_TYPE_UINT)

E("FileData", file_data, EBML_TYPE_BINARY)

E("FileMimeType", file_mime_type, EBML_TYPE_STR)

E("FileName", file_name, EBML_TYPE_STR)

E("FileDescription", file_description, EBML_TYPE_STR)

#define N attached_file
E_S("AttachedFile", 5)
F(MATROSKA_ID_FILEDESCRIPTION, file_description, 0)
F(MATROSKA_ID_FILENAME, file_name, 0)
F(MATROSKA_ID_FILEMIMETYPE, file_mime_type, 0)
F(MATROSKA_ID_FILEDATA, file_data, 0)
F(MATROSKA_ID_FILEUID, file_uid, 0)
}};
#undef N

#define N attachments
E_S("Attachments", 1)
F(MATROSKA_ID_ATTACHEDFILE, attached_file, 1)
}};
#undef N

E("CueDuration", cue_duration, EBML_TYPE_UINT)

E("CueRelativePosition", cue_relative_position, EBML_TYPE_UINT)

E("CueClusterPosition", cue_cluster_position, EBML_TYPE_UINT)

E("CueTrack", cue_track, EBML_TYPE_UINT)

#define N cue_track_positions
E_S("CueTrackPositions", 4)
F(MATROSKA_ID_CUETRACK, cue_track, 0)
F(MATROSKA_ID_CUECLUSTERPOSITION, cue_cluster_position, 0)
F(MATROSKA_ID_CUERELATIVEPOSITION, cue_relative_position, 0)
F(MATROSKA_ID_CUEDURATION, cue_duration, 0)
}};
#undef N

E("CueTime", cue_time, EBML_TYPE_UINT)

#define N cue_point
E_S("CuePoint", 2)
F(MATROSKA_ID_CUETIME, cue_time, 0)
F(MATROSKA_ID_CUETRACKPOSITIONS, cue_track_positions, 1)
}};
#undef N

#define N cues
E_S("Cues", 1)
F(MATROSKA_ID_CUEPOINT, cue_point, 1)
}};
#undef N

E("ContentCompSettings", content_comp_settings, EBML_TYPE_BINARY)

E("ContentCompAlgo", content_comp_algo, EBML_TYPE_UINT)

#define N content_compression
E_S("ContentCompression", 2)
F(MATROSKA_ID_CONTENTCOMPALGO, content_comp_algo, 0)
F(MATROSKA_ID_CONTENTCOMPSETTINGS, content_comp_settings, 0)
}};
#undef N

E("ContentEncodingType", content_encoding_type, EBML_TYPE_UINT)

E("ContentEncodingScope", content_encoding_scope, EBML_TYPE_UINT)

E("ContentEncodingOrder", content_encoding_order, EBML_TYPE_UINT)

#define N content_encoding
E_S("ContentEncoding", 4)
F(MATROSKA_ID_CONTENTENCODINGORDER, content_encoding_order, 0)
F(MATROSKA_ID_CONTENTENCODINGSCOPE, content_encoding_scope, 0)
F(MATROSKA_ID_CONTENTENCODINGTYPE, content_encoding_type, 0)
F(MATROSKA_ID_CONTENTCOMPRESSION, content_compression, 0)
}};
#undef N

#define N content_encodings
E_S("ContentEncodings", 1)
F(MATROSKA_ID_CONTENTENCODING, content_encoding, 1)
}};
#undef N

E("BitDepth", bit_depth, EBML_TYPE_UINT)

E("Channels", channels, EBML_TYPE_UINT)

E("OutputSamplingFrequency", output_sampling_frequency, EBML_TYPE_FLOAT)

E("SamplingFrequency", sampling_frequency, EBML_TYPE_FLOAT)

#define N audio
E_S("Audio", 4)
F(MATROSKA_ID_SAMPLINGFREQUENCY, sampling_frequency, 0)
F(MATROSKA_ID_OUTPUTSAMPLINGFREQUENCY, output_sampling_frequency, 0)
F(MATROSKA_ID_CHANNELS, channels, 0)
F(MATROSKA_ID_BITDEPTH, bit_depth, 0)
}};
#undef N

E("ProjectionPoseRoll", projection_pose_roll, EBML_TYPE_FLOAT)

E("ProjectionPosePitch", projection_pose_pitch, EBML_TYPE_FLOAT)

E("ProjectionPoseYaw", projection_pose_yaw, EBML_TYPE_FLOAT)

E("ProjectionPrivate", projection_private, EBML_TYPE_BINARY)

E("ProjectionType", projection_type, EBML_TYPE_UINT)

#define N projection
E_S("Projection", 5)
F(MATROSKA_ID_PROJECTIONTYPE, projection_type, 0)
F(MATROSKA_ID_PROJECTIONPRIVATE, projection_private, 0)
F(MATROSKA_ID_PROJECTIONPOSEYAW, projection_pose_yaw, 0)
F(MATROSKA_ID_PROJECTIONPOSEPITCH, projection_pose_pitch, 0)
F(MATROSKA_ID_PROJECTIONPOSEROLL, projection_pose_roll, 0)
}};
#undef N

E("LuminanceMin", luminance_min, EBML_TYPE_FLOAT)

E("LuminanceMax", luminance_max, EBML_TYPE_FLOAT)

E("WhitePointChromaticityY", white_point_chromaticity_y, EBML_TYPE_FLOAT)

E("WhitePointChromaticityX", white_point_chromaticity_x, EBML_TYPE_FLOAT)

E("PrimaryBChromaticityY", primary_b_chromaticity_y, EBML_TYPE_FLOAT)

E("PrimaryBChromaticityX", primary_b_chromaticity_x, EBML_TYPE_FLOAT)

E("PrimaryGChromaticityY", primary_g_chromaticity_y, EBML_TYPE_FLOAT)

E("PrimaryGChromaticityX", primary_g_chromaticity_x, EBML_TYPE_FLOAT)

E("PrimaryRChromaticityY", primary_r_chromaticity_y, EBML_TYPE_FLOAT)

E("PrimaryRChromaticityX", primary_r_chromaticity_x, EBML_TYPE_FLOAT)

#define N mastering_metadata
E_S("MasteringMetadata", 10)
F(MATROSKA_ID_PRIMARYRCHROMATICITYX, primary_r_chromaticity_x, 0)
F(MATROSKA_ID_PRIMARYRCHROMATICITYY, primary_r_chromaticity_y, 0)
F(MATROSKA_ID_PRIMARYGCHROMATICITYX, primary_g_chromaticity_x, 0)
F(MATROSKA_ID_PRIMARYGCHROMATICITYY, primary_g_chromaticity_y, 0)
F(MATROSKA_ID_PRIMARYBCHROMATICITYX, primary_b_chromaticity_x, 0)
F(MATROSKA_ID_PRIMARYBCHROMATICITYY, primary_b_chromaticity_y, 0)
F(MATROSKA_ID_WHITEPOINTCHROMATICITYX, white_point_chromaticity_x, 0)
F(MATROSKA_ID_WHITEPOINTCHROMATICITYY, white_point_chromaticity_y, 0)
F(MATROSKA_ID_LUMINANCEMAX, luminance_max, 0)
F(MATROSKA_ID_LUMINANCEMIN, luminance_min, 0)
}};
#undef N

E("MaxFALL", max_fall, EBML_TYPE_UINT)

E("MaxCLL", max_cll, EBML_TYPE_UINT)

E("Primaries", primaries, EBML_TYPE_UINT)

E("TransferCharacteristics", transfer_characteristics, EBML_TYPE_UINT)

E("Range", range, EBML_TYPE_UINT)

E("ChromaSitingVert", chroma_siting_vert, EBML_TYPE_UINT)

E("ChromaSitingHorz", chroma_siting_horz, EBML_TYPE_UINT)

E("CbSubsamplingVert", cb_subsampling_vert, EBML_TYPE_UINT)

E("CbSubsamplingHorz", cb_subsampling_horz, EBML_TYPE_UINT)

E("ChromaSubsamplingVert", chroma_subsampling_vert, EBML_TYPE_UINT)

E("ChromaSubsamplingHorz", chroma_subsampling_horz, EBML_TYPE_UINT)

E("BitsPerChannel", bits_per_channel, EBML_TYPE_UINT)

E("MatrixCoefficients", matrix_coefficients, EBML_TYPE_UINT)

#define N colour
E_S("Colour", 14)
F(MATROSKA_ID_MATRIXCOEFFICIENTS, matrix_coefficients, 0)
F(MATROSKA_ID_BITSPERCHANNEL, bits_per_channel, 0)
F(MATROSKA_ID_CHROMASUBSAMPLINGHORZ, chroma_subsampling_horz, 0)
F(MATROSKA_ID_CHROMASUBSAMPLINGVERT, chroma_subsampling_vert, 0)
F(MATROSKA_ID_CBSUBSAMPLINGHORZ, cb_subsampling_horz, 0)
F(MATROSKA_ID_CBSUBSAMPLINGVERT, cb_subsampling_vert, 0)
F(MATROSKA_ID_CHROMASITINGHORZ, chroma_siting_horz, 0)
F(MATROSKA_ID_CHROMASITINGVERT, chroma_siting_vert, 0)
F(MATROSKA_ID_RANGE, range, 0)
F(MATROSKA_ID_TRANSFERCHARACTERISTICS, transfer_characteristics, 0)
F(MATROSKA_ID_PRIMARIES, primaries, 0)
F(MATROSKA_ID_MAXCLL, max_cll, 0)
F(MATROSKA_ID_MAXFALL, max_fall, 0)
F(MATROSKA_ID_MASTERINGMETADATA, mastering_metadata, 0)
}};
#undef N

E("StereoMode", stereo_mode, EBML_TYPE_UINT)

E("ColourSpace", colour_space, EBML_TYPE_BINARY)

E("FrameRate", frame_rate, EBML_TYPE_FLOAT)

E("DisplayUnit", display_unit, EBML_TYPE_UINT)

E("DisplayHeight", display_height, EBML_TYPE_UINT)

E("DisplayWidth", display_width, EBML_TYPE_UINT)

E("PixelHeight", pixel_height, EBML_TYPE_UINT)

E("PixelWidth", pixel_width, EBML_TYPE_UINT)

E("FlagInterlaced", flag_interlaced, EBML_TYPE_UINT)

#define N video
E_S("Video", 11)
F(MATROSKA_ID_FLAGINTERLACED, flag_interlaced, 0)
F(MATROSKA_ID_PIXELWIDTH, pixel_width, 0)
F(MATROSKA_ID_PIXELHEIGHT, pixel_height, 0)
F(MATROSKA_ID_DISPLAYWIDTH, display_width, 0)
F(MATROSKA_ID_DISPLAYHEIGHT, display_height, 0)
F(MATROSKA_ID_DISPLAYUNIT, display_unit, 0)
F(MATROSKA_ID_FRAMERATE, frame_rate, 0)
F(MATROSKA_ID_COLOURSPACE, colour_space, 0)
F(MATROSKA_ID_STEREOMODE, stereo_mode, 0)
F(MATROSKA_ID_COLOUR, colour, 0)
F(MATROSKA_ID_PROJECTION, projection, 0)
}};
#undef N

E("SeekPreRoll", seek_pre_roll, EBML_TYPE_UINT)

E("CodecDelay", codec_delay, EBML_TYPE_UINT)

E("CodecDecodeAll", codec_decode_all, EBML_TYPE_UINT)

E("CodecName", codec_name, EBML_TYPE_STR)

E("CodecPrivate", codec_private, EBML_TYPE_BINARY)

E("CodecID", codec_id, EBML_TYPE_STR)

E("Language", language, EBML_TYPE_STR)

E("Name", name, EBML_TYPE_STR)

E("MaxBlockAdditionID", max_block_addition_id, EBML_TYPE_UINT)

E("TrackTimecodeScale", track_timecode_scale, EBML_TYPE_FLOAT)

E("DefaultDuration", default_duration, EBML_TYPE_UINT)

E("MaxCache", max_cache, EBML_TYPE_UINT)

E("MinCache", min_cache, EBML_TYPE_UINT)

E("FlagLacing", flag_lacing, EBML_TYPE_UINT)

E("FlagForced", flag_forced, EBML_TYPE_UINT)

E("FlagDefault", flag_default, EBML_TYPE_UINT)

E("FlagEnabled", flag_enabled, EBML_TYPE_UINT)

E("TrackType", track_type, EBML_TYPE_UINT)

E("TrackUID", track_uid, EBML_TYPE_UINT)

E("TrackNumber", track_number, EBML_TYPE_UINT)

#define N track_entry
E_S("TrackEntry", 23)
F(MATROSKA_ID_TRACKNUMBER, track_number, 0)
F(MATROSKA_ID_TRACKUID, track_uid, 0)
F(MATROSKA_ID_TRACKTYPE, track_type, 0)
F(MATROSKA_ID_FLAGENABLED, flag_enabled, 0)
F(MATROSKA_ID_FLAGDEFAULT, flag_default, 0)
F(MATROSKA_ID_FLAGFORCED, flag_forced, 0)
F(MATROSKA_ID_FLAGLACING, flag_lacing, 0)
F(MATROSKA_ID_MINCACHE, min_cache, 0)
F(MATROSKA_ID_MAXCACHE, max_cache, 0)
F(MATROSKA_ID_DEFAULTDURATION, default_duration, 0)
F(MATROSKA_ID_TRACKTIMECODESCALE, track_timecode_scale, 0)
F(MATROSKA_ID_MAXBLOCKADDITIONID, max_block_addition_id, 0)
F(MATROSKA_ID_NAME, name, 0)
F(MATROSKA_ID_LANGUAGE, language, 0)
F(MATROSKA_ID_CODECID, codec_id, 0)
F(MATROSKA_ID_CODECPRIVATE, codec_private, 0)
F(MATROSKA_ID_CODECNAME, codec_name, 0)
F(MATROSKA_ID_CODECDECODEALL, codec_decode_all, 0)
F(MATROSKA_ID_CODECDELAY, codec_delay, 0)
F(MATROSKA_ID_SEEKPREROLL, seek_pre_roll, 0)
F(MATROSKA_ID_VIDEO, video, 0)
F(MATROSKA_ID_AUDIO, audio, 0)
F(MATROSKA_ID_CONTENTENCODINGS, content_encodings, 0)
}};
#undef N

#define N tracks
E_S("Tracks", 1)
F(MATROSKA_ID_TRACKENTRY, track_entry, 1)
}};
#undef N

E("SimpleBlock", simple_block, EBML_TYPE_BINARY)

E("BlockAdditional", block_additional, EBML_TYPE_BINARY)

E("BlockAddID", block_add_id, EBML_TYPE_UINT)

#define N block_more
E_S("BlockMore", 2)
F(MATROSKA_ID_BLOCKADDID, block_add_id, 0)
F(MATROSKA_ID_BLOCKADDITIONAL, block_additional, 0)
}};
#undef N

#define N block_additions
E_S("BlockAdditions", 1)
F(MATROSKA_ID_BLOCKMORE, block_more, 1)
}};
#undef N

E("DiscardPadding", discard_padding, EBML_TYPE_SINT)

E("ReferenceBlock", reference_block, EBML_TYPE_SINT)

E("BlockDuration", block_duration, EBML_TYPE_UINT)

E("Block", block, EBML_TYPE_BINARY)

#define N block_group
E_S("BlockGroup", 5)
F(MATROSKA_ID_BLOCK, block, 0)
F(MATROSKA_ID_BLOCKDURATION, block_duration, 0)
F(MATROSKA_ID_REFERENCEBLOCK, reference_block, 1)
F(MATROSKA_ID_DISCARDPADDING, discard_padding, 0)
F(MATROSKA_ID_BLOCKADDITIONS, block_additions, 0)
}};
#undef N

E("Timecode", timecode, EBML_TYPE_UINT)

#define N cluster
E_S("Cluster", 3)
F(MATROSKA_ID_TIMECODE, timecode, 0)
F(MATROSKA_ID_BLOCKGROUP, block_group, 1)
F(MATROSKA_ID_SIMPLEBLOCK, simple_block, 1)
}};
#undef N

E("Duration", duration, EBML_TYPE_FLOAT)

E("WritingApp", writing_app, EBML_TYPE_STR)

E("MuxingApp", muxing_app, EBML_TYPE_STR)

E("Title", title, EBML_TYPE_STR)

E("DateUTC", date_utc, EBML_TYPE_SINT)

E("TimecodeScale", timecode_scale, EBML_TYPE_UINT)

E("NextUID", next_uid, EBML_TYPE_BINARY)

E("PrevUID", prev_uid, EBML_TYPE_BINARY)

E("SegmentUID", segment_uid, EBML_TYPE_BINARY)

#define N info
E_S("Info", 9)
F(MATROSKA_ID_SEGMENTUID, segment_uid, 0)
F(MATROSKA_ID_PREVUID, prev_uid, 0)
F(MATROSKA_ID_NEXTUID, next_uid, 0)
F(MATROSKA_ID_TIMECODESCALE, timecode_scale, 0)
F(MATROSKA_ID_DATEUTC, date_utc, 0)
F(MATROSKA_ID_TITLE, title, 0)
F(MATROSKA_ID_MUXINGAPP, muxing_app, 0)
F(MATROSKA_ID_WRITINGAPP, writing_app, 0)
F(MATROSKA_ID_DURATION, duration, 0)
}};
#undef N

E("SeekPosition", seek_position, EBML_TYPE_UINT)

E("SeekID", seek_id, EBML_TYPE_EBML_ID)

#define N seek
E_S("Seek", 2)
F(MATROSKA_ID_SEEKID, seek_id, 0)
F(MATROSKA_ID_SEEKPOSITION, seek_position, 0)
}};
#undef N

#define N seek_head
E_S("SeekHead", 1)
F(MATROSKA_ID_SEEK, seek, 1)
}};
#undef N

#define N segment
E_S("Segment", 8)
F(MATROSKA_ID_SEEKHEAD, seek_head, 1)
F(MATROSKA_ID_INFO, info, 1)
F(MATROSKA_ID_CLUSTER, cluster, 1)
F(MATROSKA_ID_TRACKS, tracks, 1)
F(MATROSKA_ID_CUES, cues, 0)
F(MATROSKA_ID_ATTACHMENTS, attachments, 0)
F(MATROSKA_ID_CHAPTERS, chapters, 0)
F(MATROSKA_ID_TAGS, tags, 1)
}};
#undef N

E("Void", void, EBML_TYPE_BINARY)

E("CRC32", crc32, EBML_TYPE_BINARY)

E("DocTypeReadVersion", doc_type_read_version, EBML_TYPE_UINT)

E("DocTypeVersion", doc_type_version, EBML_TYPE_UINT)

E("DocType", doc_type, EBML_TYPE_STR)

E("EBMLMaxSizeLength", ebml_max_size_length, EBML_TYPE_UINT)

E("EBMLMaxIDLength", ebml_max_id_length, EBML_TYPE_UINT)

E("EBMLReadVersion", ebml_read_version, EBML_TYPE_UINT)

E("EBMLVersion", ebml_version, EBML_TYPE_UINT)

#define N ebml
E_S("EBML", 7)
F(EBML_ID_EBMLVERSION, ebml_version, 0)
F(EBML_ID_EBMLREADVERSION, ebml_read_version, 0)
F(EBML_ID_EBMLMAXIDLENGTH, ebml_max_id_length, 0)
F(EBML_ID_EBMLMAXSIZELENGTH, ebml_max_size_length, 0)
F(EBML_ID_DOCTYPE, doc_type, 0)
F(EBML_ID_DOCTYPEVERSION, doc_type_version, 0)
F(EBML_ID_DOCTYPEREADVERSION, doc_type_read_version, 0)
}};
#undef N
#undef EVALARGS
#undef SN
#undef S
#undef FN
#undef F

// Used to read/write pointers to different struct types
struct generic;
#define generic_struct struct generic

static uint32_t ebml_parse_id(uint8_t *data, size_t data_len, int *length)
{
    *length = -1;
    uint8_t *end = data + data_len;
    if (data == end)
        return EBML_ID_INVALID;
    int len = 1;
    uint32_t id = *data++;
    for (int len_mask = 0x80; !(id & len_mask); len_mask >>= 1) {
        len++;
        if (len > 4)
            return EBML_ID_INVALID;
    }
    *length = len;
    while (--len && data < end)
        id = (id << 8) | *data++;
    return id;
}

static uint64_t ebml_parse_length(uint8_t *data, size_t data_len, int *length)
{
    *length = -1;
    uint8_t *end = data + data_len;
    if (data == end)
        return -1;
    uint64_t r = *data++;
    int len = 1;
    int len_mask;
    for (len_mask = 0x80; !(r & len_mask); len_mask >>= 1) {
        len++;
        if (len > 8)
            return -1;
    }
    r &= len_mask - 1;

    int num_allones = 0;
    if (r == len_mask - 1)
        num_allones++;
    for (int i = 1; i < len; i++) {
        if (data == end)
            return -1;
        if (*data == 255)
            num_allones++;
        r = (r << 8) | *data++;
    }
    // According to Matroska specs this means "unknown length"
    // Could be supported if there are any actual files using it
    if (num_allones == len)
        return -1;
    *length = len;
    return r;
}

static uint64_t ebml_parse_uint(uint8_t *data, int length)
{
    assert(length >= 0 && length <= 8);
    uint64_t r = 0;
    while (length--)
        r = (r << 8) + *data++;
    return r;
}

static int64_t ebml_parse_sint(uint8_t *data, int length)
{
    assert(length >= 0 && length <= 8);
    if (!length)
        return 0;
    uint64_t r = 0;
    if (*data & 0x80)
        r = -1;
    while (length--)
        r = (r << 8) | *data++;
    return (int64_t)r; // assume complement of 2
}

static double ebml_parse_float(uint8_t *data, int length)
{
    assert(length == 0 || length == 4 || length == 8);
    uint64_t i = ebml_parse_uint(data, length);
    if (length == 4)
        return av_int2float(i);
    else
        return av_int2double(i);
}


// target must be initialized to zero
static void ebml_parse_element(struct ebml_parse_ctx *ctx, void *target,
                               uint8_t *data, int size,
                               const struct ebml_elem_desc *type, int level)
{
    assert(type->type == EBML_TYPE_SUBELEMENTS);
    assert(level < 8);
    MP_TRACE(ctx, "%.*sParsing element %s\n", level, "       ", type->name);

    char *s = target;
    uint8_t *end = data + size;
    uint8_t *p = data;
    int num_elems[MAX_EBML_SUBELEMENTS] = {0};
    while (p < end) {
        uint8_t *startp = p;
        int len;
        uint32_t id = ebml_parse_id(p, end - p, &len);
        if (len > end - p)
            goto past_end_error;
        if (len < 0) {
            MP_ERR(ctx, "Error parsing subelement id\n");
            goto other_error;
        }
        p += len;
        uint64_t length = ebml_parse_length(p, end - p, &len);
        if (len > end - p)
            goto past_end_error;
        if (len < 0) {
            MP_ERR(ctx, "Error parsing subelement length\n");
            goto other_error;
        }
        p += len;

        int field_idx = -1;
        for (int i = 0; i < type->field_count; i++)
            if (type->fields[i].id == id) {
                field_idx = i;
                num_elems[i]++;
                if (num_elems[i] >= 0x70000000) {
                    MP_ERR(ctx, "Too many EBML subelements.\n");
                    goto other_error;
                }
                break;
            }

        if (length > end - p) {
            if (field_idx >= 0 && type->fields[field_idx].desc->type
                != EBML_TYPE_SUBELEMENTS) {
                MP_ERR(ctx, "Subelement content goes "
                       "past end of containing element\n");
                goto other_error;
            }
            // Try to parse what is possible from inside this partial element
            ctx->has_errors = true;
            length = end - p;
        }
        p += length;

        continue;

    past_end_error:
        MP_ERR(ctx, "Subelement headers go past end of containing element\n");
    other_error:
        ctx->has_errors = true;
        end = startp;
        break;
    }

    for (int i = 0; i < type->field_count; i++) {
        if (num_elems[i] && type->fields[i].multiple) {
            char *ptr = s + type->fields[i].offset;
            switch (type->fields[i].desc->type) {
            case EBML_TYPE_SUBELEMENTS: {
                size_t max = 1000000000 / type->fields[i].desc->size;
                if (num_elems[i] > max) {
                    MP_ERR(ctx, "Too many subelements.\n");
                    num_elems[i] = max;
                }
                int sz = num_elems[i] * type->fields[i].desc->size;
                *(generic_struct **) ptr = talloc_zero_size(ctx->talloc_ctx, sz);
                break;
            }
            case EBML_TYPE_UINT:
                *(uint64_t **) ptr = talloc_zero_array(ctx->talloc_ctx,
                                                       uint64_t, num_elems[i]);
                break;
            case EBML_TYPE_SINT:
                *(int64_t **) ptr = talloc_zero_array(ctx->talloc_ctx,
                                                      int64_t, num_elems[i]);
                break;
            case EBML_TYPE_FLOAT:
                *(double **) ptr = talloc_zero_array(ctx->talloc_ctx,
                                                     double, num_elems[i]);
                break;
            case EBML_TYPE_STR:
                *(char ***) ptr = talloc_zero_array(ctx->talloc_ctx,
                                                    char *, num_elems[i]);
                break;
            case EBML_TYPE_BINARY:
                *(struct bstr **) ptr = talloc_zero_array(ctx->talloc_ctx,
                                                          struct bstr,
                                                          num_elems[i]);
                break;
            case EBML_TYPE_EBML_ID:
                *(int32_t **) ptr = talloc_zero_array(ctx->talloc_ctx,
                                                      uint32_t, num_elems[i]);
                break;
            default:
                abort();
            }
        }
    }

    while (data < end) {
        int len;
        uint32_t id = ebml_parse_id(data, end - data, &len);
        if (len < 0 || len > end - data) {
            MP_ERR(ctx, "Error parsing subelement\n");
            break;
        }
        data += len;
        uint64_t length = ebml_parse_length(data, end - data, &len);
        if (len < 0 || len > end - data) {
            MP_ERR(ctx, "Error parsing subelement length\n");
            break;
        }
        data += len;
        if (length > end - data) {
            // Try to parse what is possible from inside this partial element
            length = end - data;
            MP_ERR(ctx, "Next subelement content goes "
                   "past end of containing element, will be truncated\n");
        }
        int field_idx = -1;
        for (int i = 0; i < type->field_count; i++)
            if (type->fields[i].id == id) {
                field_idx = i;
                break;
            }
        if (field_idx < 0) {
            if (id == 0xec) {
                MP_TRACE(ctx, "%.*sIgnoring Void element "
                         "size: %"PRIu64"\n", level+1, "        ", length);
            } else if (id == 0xbf) {
                MP_TRACE(ctx, "%.*sIgnoring CRC-32 "
                         "element size: %"PRIu64"\n", level+1, "        ",
                         length);
            } else {
                MP_DBG(ctx, "Ignoring unrecognized "
                       "subelement. ID: %x size: %"PRIu64"\n", id, length);
            }
            data += length;
            continue;
        }
        const struct ebml_field_desc *fd = &type->fields[field_idx];
        const struct ebml_elem_desc *ed = fd->desc;
        bool multiple = fd->multiple;
        int *countptr = (int *) (s + fd->count_offset);
        if (*countptr >= num_elems[field_idx]) {
            // Shouldn't happen on any sane file without bugs
            MP_ERR(ctx, "Too many subelements.\n");
            ctx->has_errors = true;
            data += length;
            continue;
        }
        if (*countptr > 0 && !multiple) {
            MP_WARN(ctx, "Another subelement of type "
                    "%x %s (size: %"PRIu64"). Only one allowed. Ignoring.\n",
                    id, ed->name, length);
            ctx->has_errors = true;
            data += length;
            continue;
        }
        MP_TRACE(ctx, "%.*sParsing %x %s size: %"PRIu64
                 " value: ", level+1, "        ", id, ed->name, length);

        char *fieldptr = s + fd->offset;
        switch (ed->type) {
        case EBML_TYPE_SUBELEMENTS:
            MP_TRACE(ctx, "subelements\n");
            char *subelptr;
            if (multiple) {
                char *array_start = (char *) *(generic_struct **) fieldptr;
                subelptr = array_start + *countptr * ed->size;
            } else
                subelptr = fieldptr;
            ebml_parse_element(ctx, subelptr, data, length, ed, level + 1);
            break;

        case EBML_TYPE_UINT:;
            uint64_t *uintptr;
#define GETPTR(subelptr, fieldtype)                                     \
            if (multiple)                                               \
                subelptr = *(fieldtype **) fieldptr + *countptr;        \
            else                                                        \
                subelptr = (fieldtype *) fieldptr
            GETPTR(uintptr, uint64_t);
            if (length < 1 || length > 8) {
                MP_ERR(ctx, "uint invalid length %"PRIu64"\n", length);
                goto error;
            }
            *uintptr = ebml_parse_uint(data, length);
            MP_TRACE(ctx, "uint %"PRIu64"\n", *uintptr);
            break;

        case EBML_TYPE_SINT:;
            int64_t *sintptr;
            GETPTR(sintptr, int64_t);
            if (length > 8) {
                MP_ERR(ctx, "sint invalid length %"PRIu64"\n", length);
                goto error;
            }
            *sintptr = ebml_parse_sint(data, length);
            MP_TRACE(ctx, "sint %"PRId64"\n", *sintptr);
            break;

        case EBML_TYPE_FLOAT:;
            double *floatptr;
            GETPTR(floatptr, double);
            if (length != 0 && length != 4 && length != 8) {
                MP_ERR(ctx, "float invalid length %"PRIu64"\n", length);
                goto error;
            }
            *floatptr = ebml_parse_float(data, length);
            MP_DBG(ctx, "float %f\n", *floatptr);
            break;

        case EBML_TYPE_STR:
            if (length > 1024 * 1024) {
                MP_ERR(ctx, "Not reading overly long string element.\n");
                break;
            }
            char **strptr;
            GETPTR(strptr, char *);
            *strptr = talloc_strndup(ctx->talloc_ctx, data, length);
            MP_TRACE(ctx, "string \"%s\"\n", *strptr);
            break;

        case EBML_TYPE_BINARY:;
            if (length > 0x80000000) {
                MP_ERR(ctx, "Not reading overly long EBML element.\n");
                break;
            }
            struct bstr *binptr;
            GETPTR(binptr, struct bstr);
            binptr->start = data;
            binptr->len = length;
            MP_TRACE(ctx, "binary %zd bytes\n", binptr->len);
            break;

        case EBML_TYPE_EBML_ID:;
            uint32_t *idptr;
            GETPTR(idptr, uint32_t);
            *idptr = ebml_parse_id(data, end - data, &len);
            if (len != length) {
                MP_ERR(ctx, "ebml_id broken value\n");
                goto error;
            }
            MP_TRACE(ctx, "ebml_id %x\n", (unsigned)*idptr);
            break;
        default:
            abort();
        }
        *countptr += 1;
    error:
        data += length;
    }
}

// target must be initialized to zero
int ebml_read_element(struct stream *s, struct ebml_parse_ctx *ctx,
                      void *target, const struct ebml_elem_desc *desc)
{
    ctx->has_errors = false;
    int msglevel = ctx->no_error_messages ? MSGL_DEBUG : MSGL_WARN;
    uint64_t length = ebml_read_length(s);
    if (s->eof) {
        MP_MSG(ctx, msglevel, "Unexpected end of file "
                   "- partial or corrupt file?\n");
        return -1;
    }
    if (length == EBML_UINT_INVALID) {
        MP_MSG(ctx, msglevel, "EBML element with unknown length - unsupported\n");
        return -1;
    }
    if (length > 1000000000) {
        MP_MSG(ctx, msglevel, "Refusing to read element over 100 MB in size\n");
        return -1;
    }
    ctx->talloc_ctx = talloc_size(NULL, length);
    int read_len = stream_read(s, ctx->talloc_ctx, length);
    if (read_len < length)
        MP_MSG(ctx, msglevel, "Unexpected end of file - partial or corrupt file?\n");
    ebml_parse_element(ctx, target, ctx->talloc_ctx, read_len, desc, 0);
    if (ctx->has_errors)
        MP_MSG(ctx, msglevel, "Error parsing element %s\n", desc->name);
    return 0;
}
