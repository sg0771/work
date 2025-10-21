/*
 * Various utilities for command line tools
 * copyright (c) 2003 Fabrice Bellard
 *
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

#ifndef CMDUTILS_H
#define CMDUTILS_H

#include <stdint.h>

#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include <libavutil/pixdesc.h>
#include <libavutil/opt.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/avstring.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>



#ifdef _WIN32
#undef main /* We don't want SDL to override our main() */
#endif

/**
 * program name, defined by the program for show_version().
 */
extern const char program_name[];

/**
 * program birth year, defined by the program for show_banner()
 */
extern const int program_birth_year;

extern AVCodecContext *avcodec_opts[AVMEDIA_TYPE_NB];
extern AVFormatContext *avformat_opts;
extern AVDictionary *sws_dict;
extern AVDictionary *swr_opts;
extern AVDictionary *format_opts, *codec_opts, *resample_opts;


void register_exit(void (*cb)(int ret));

void exit_program(int ret) av_noreturn;

void init_opts(void);

void uninit_opts(void);

int opt_default(void *optctx, const char *opt, const char *arg);

int opt_max_alloc(void *optctx, const char *opt, const char *arg);


double parse_number_or_die(const char *context, const char *numstr, int type,
                           double min, double max);

int64_t parse_time_or_die(const char *context, const char *timestr,
                          int is_duration);

typedef struct SpecifierOpt {
    char *specifier;    /**< stream/chapter/program/... specifier */
    union {
        uint8_t *str;
        int        i;
        int64_t  i64;
        float      f;
        double   dbl;
    } u;
} SpecifierOpt;

typedef struct OptionDef {
    const char *name;
    int flags;
#define HAS_ARG    0x0001
#define OPT_BOOL   0x0002
#define OPT_EXPERT 0x0004
#define OPT_STRING 0x0008
#define OPT_VIDEO  0x0010
#define OPT_AUDIO  0x0020
#define OPT_INT    0x0080
#define OPT_FLOAT  0x0100
#define OPT_SUBTITLE 0x0200
#define OPT_INT64  0x0400
#define OPT_EXIT   0x0800
#define OPT_DATA   0x1000
#define OPT_PERFILE  0x2000     /* the option is per-file (currently ffmpeg-only).
                                   implied by OPT_OFFSET or OPT_SPEC */
#define OPT_OFFSET 0x4000       /* option is specified as an offset in a passed optctx */
#define OPT_SPEC   0x8000       /* option is to be stored in an array of SpecifierOpt.
                                   Implies OPT_OFFSET. Next element after the offset is
                                   an int containing element count in the array. */
#define OPT_TIME  0x10000
#define OPT_DOUBLE 0x20000
#define OPT_INPUT  0x40000
#define OPT_OUTPUT 0x80000
#define OPT_EXTERN_AUDIO  0x160000
     union {
        void *dst_ptr;
        int (*func_arg)(void *, const char *, const char *);
        size_t off;
    } u;
    const char *help;
    const char *argname;
} OptionDef;


#define CMDUTILS_COMMON_OPTIONS                                                                                         \
    { "L",           OPT_EXIT,             { .func_arg = show_help },     "show license" },                          \
    { "h",           OPT_EXIT,             { .func_arg = show_help },        "show help", "topic" },                    \
    { "?",           OPT_EXIT,             { .func_arg = show_help },        "show help", "topic" },                    \
    { "help",        OPT_EXIT,             { .func_arg = show_help },        "show help", "topic" },                    \
    { "-help",       OPT_EXIT,             { .func_arg = show_help },        "show help", "topic" },                    \
    { "version",     OPT_EXIT,             { .func_arg = show_help },     "show version" },                          \
    { "buildconf",   OPT_EXIT,             { .func_arg = show_help },   "show build configuration" },              \
    { "formats",     OPT_EXIT,             { .func_arg = show_help },     "show available formats" },                \
    { "muxers",      OPT_EXIT,             { .func_arg = show_help },      "show available muxers" },                 \
    { "demuxers",    OPT_EXIT,             { .func_arg = show_help },    "show available demuxers" },               \
    { "devices",     OPT_EXIT,             { .func_arg = show_help },     "show available devices" },                \
    { "codecs",      OPT_EXIT,             { .func_arg = show_help },      "show available codecs" },                 \
    { "decoders",    OPT_EXIT,             { .func_arg = show_help },    "show available decoders" },               \
    { "encoders",    OPT_EXIT,             { .func_arg = show_help },    "show available encoders" },               \
    { "bsfs",        OPT_EXIT,             { .func_arg = show_help },        "show available bit stream filters" },     \
    { "protocols",   OPT_EXIT,             { .func_arg = show_help },   "show available protocols" },              \
    { "filters",     OPT_EXIT,             { .func_arg = show_help },     "show available filters" },                \
    { "pix_fmts",    OPT_EXIT,             { .func_arg = show_help },    "show available pixel formats" },          \
    { "layouts",     OPT_EXIT,             { .func_arg = show_help },     "show standard channel layouts" },         \
    { "sample_fmts", OPT_EXIT,             { .func_arg = show_help }, "show available audio sample formats" },   \
    { "colors",      OPT_EXIT,             { .func_arg = show_help },      "show available color names" },            \
    { "loglevel",    HAS_ARG,              { .func_arg = show_help },     "set logging level", "loglevel" },         \
    { "v",           HAS_ARG,              { .func_arg = show_help },     "set logging level", "loglevel" },         \
    { "report",      OPT_BOOL | OPT_EXPERT, {&dummy},            "generate a report" },                     \
    { "max_alloc",   HAS_ARG,              { .func_arg = show_help },    "set maximum size of a single allocated block", "bytes" }, \
    { "cpuflags",    HAS_ARG | OPT_EXPERT, { .func_arg = show_help },     "force specific cpu flags", "flags" },     \
    { "hide_banner", OPT_BOOL | OPT_EXPERT, {&dummy},     "do not show program banner", "hide_banner" },   


int show_help(void *optctx, const char *opt, const char *arg);

void parse_options(void *optctx, int argc, char **argv, const OptionDef *options,
                   void (* parse_arg_function)(void *optctx, const char*));

int parse_option(void *optctx, const char *opt, const char *arg,
                 const OptionDef *options);

typedef struct Option {
    const OptionDef  *opt;
    const char       *key;
    const char       *val;
} Option;

typedef struct OptionGroupDef {
    const char *name;
    const char *sep;
    int flags;
} OptionGroupDef;

typedef struct OptionGroup {
    const OptionGroupDef *group_def;
    const char *arg;

    Option *opts;
    int  nb_opts;

    AVDictionary *codec_opts;
    AVDictionary *format_opts;
    AVDictionary *resample_opts;
    AVDictionary *sws_dict;
    AVDictionary *swr_opts;
} OptionGroup;

typedef struct OptionGroupList {
    const OptionGroupDef *group_def;

    OptionGroup *groups;
    int       nb_groups;
} OptionGroupList;

typedef struct OptionParseContext {
    OptionGroup global_opts;

    OptionGroupList *groups;
    int           nb_groups;

    /* parsing state */
    OptionGroup cur_group;
} OptionParseContext;

int parse_optgroup(void *optctx, OptionGroup *g);

int split_commandline(OptionParseContext *octx, int argc, char *argv[],
                      const OptionDef *options,
                      const OptionGroupDef *groups, int nb_groups);

void uninit_parse_context(OptionParseContext *octx);


int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec);

AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                AVFormatContext *s, AVStream *st, AVCodec *codec);

AVDictionary** setup_find_stream_info_opts(AVFormatContext* s, AVDictionary* codec_opts);

void print_error(const char *filename, int err);

void *grow_array(void *array, int elem_size, int *size, int new_size);

#define media_type_string av_get_media_type_string

#define GROW_ARRAY(array, nb_elems)\
    array = grow_array(array, sizeof(*array), &nb_elems, nb_elems + 1)

#define GET_PIX_FMT_NAME(pix_fmt)\
    const char *name = av_get_pix_fmt_name(pix_fmt);

#define GET_SAMPLE_FMT_NAME(sample_fmt)\
    const char *name = av_get_sample_fmt_name(sample_fmt)

#define GET_SAMPLE_RATE_NAME(rate)\
    char name[16];\
    snprintf(name, sizeof(name), "%d", rate);

#define GET_CH_LAYOUT_NAME(ch_layout)\
    char name[16];\
    snprintf(name, sizeof(name), "0x%"PRIx64, ch_layout);

#define GET_CH_LAYOUT_DESC(ch_layout)\
    char name[128];\
    av_get_channel_layout_string(name, sizeof(name), 0, ch_layout);

double get_rotation(AVStream *st);

void finish_group(OptionParseContext *octx, int group_idx,
	const char *arg);

const OptionDef *find_option(const OptionDef *po, const char *name);

void add_opt(OptionParseContext* octx, const OptionDef* opt,
    const char* key, const char* val);

#endif /* CMDUTILS_H */

