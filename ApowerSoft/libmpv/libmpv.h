
#ifndef MPV_CLIENT_API_H_
#define MPV_CLIENT_API_H_

#include <stddef.h>
#include <stdint.h>

/* New symbols must still be added to libmpv/mpv.def. */
#ifdef _WIN32
#ifdef MPV_EXPORTS
#define MPV_EXPORT __declspec(dllexport)
#else
#define MPV_EXPORT __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define MPV_EXPORT __attribute__((visibility("default")))
#else
#define MPV_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MPV_MAKE_VERSION(major, minor) (((major) << 16) | (minor) | 0UL)
#define MPV_CLIENT_API_VERSION MPV_MAKE_VERSION(2, 0)

#ifndef MPV_ENABLE_DEPRECATED
#define MPV_ENABLE_DEPRECATED 1
#endif

    /**
     * Return the MPV_CLIENT_API_VERSION the mpv source has been compiled with.
     */
    MPV_EXPORT unsigned long mpv_client_api_version(void);


    typedef struct mpv_handle mpv_handle;

    typedef enum mpv_error {

        MPV_ERROR_SUCCESS = 0,
        MPV_ERROR_EVENT_QUEUE_FULL = -1,
        MPV_ERROR_NOMEM = -2,
        MPV_ERROR_UNINITIALIZED = -3,
        MPV_ERROR_INVALID_PARAMETER = -4,
        MPV_ERROR_OPTION_NOT_FOUND = -5,
        MPV_ERROR_OPTION_FORMAT = -6,
        MPV_ERROR_OPTION_ERROR = -7,
        MPV_ERROR_PROPERTY_NOT_FOUND = -8,
        MPV_ERROR_PROPERTY_FORMAT = -9,
        MPV_ERROR_PROPERTY_UNAVAILABLE = -10,
        MPV_ERROR_PROPERTY_ERROR = -11,
        MPV_ERROR_COMMAND = -12,
        MPV_ERROR_LOADING_FAILED = -13,
        MPV_ERROR_AO_INIT_FAILED = -14,

        MPV_ERROR_VO_INIT_FAILED = -15,

        MPV_ERROR_NOTHING_TO_PLAY = -16,

        MPV_ERROR_UNKNOWN_FORMAT = -17,

        MPV_ERROR_UNSUPPORTED = -18,

        MPV_ERROR_NOT_IMPLEMENTED = -19,

        MPV_ERROR_GENERIC = -20
    } mpv_error;


    MPV_EXPORT const char* mpv_error_string(int error);
    MPV_EXPORT void mpv_free(void* data);
    MPV_EXPORT const char* mpv_client_name(mpv_handle* ctx);
    MPV_EXPORT int64_t mpv_client_id(mpv_handle* ctx);

    MPV_EXPORT mpv_handle* mpv_create(void);

    MPV_EXPORT int mpv_initialize(mpv_handle* ctx);

    MPV_EXPORT void mpv_destroy(mpv_handle* ctx);

    MPV_EXPORT void mpv_terminate_destroy(mpv_handle* ctx);

    MPV_EXPORT mpv_handle* mpv_create_client(mpv_handle* ctx, const char* name);

    MPV_EXPORT mpv_handle* mpv_create_weak_client(mpv_handle* ctx, const char* name);

    MPV_EXPORT int mpv_load_config_file(mpv_handle* ctx, const char* filename);

    MPV_EXPORT int64_t mpv_get_time_us(mpv_handle* ctx);

    typedef enum mpv_format {
        /**
         * Invalid. Sometimes used for empty values. This is always defined to 0,
         * so a normal 0-init of mpv_format (or e.g. mpv_node) is guaranteed to set
         * this it to MPV_FORMAT_NONE (which makes some things saner as consequence).
         */
        MPV_FORMAT_NONE = 0,
        /**
         * The basic type is char*. It returns the raw property string, like
         * using ${=property} in input.conf (see input.rst).
         *
         * NULL isn't an allowed value.
         *
         * Warning: although the encoding is usually UTF-8, this is not always the
         *          case. File tags often store strings in some legacy codepage,
         *          and even filenames don't necessarily have to be in UTF-8 (at
         *          least on Linux). If you pass the strings to code that requires
         *          valid UTF-8, you have to sanitize it in some way.
         *          On Windows, filenames are always UTF-8, and libmpv converts
         *          between UTF-8 and UTF-16 when using win32 API functions. See
         *          the "Encoding of filenames" section for details.
         *
         * Example for reading:
         *
         *     char *result = NULL;
         *     if (mpv_get_property(ctx, "property", MPV_FORMAT_STRING, &result) < 0)
         *         goto error;
         *     printf("%s\n", result);
         *     mpv_free(result);
         *
         * Or just use mpv_get_property_string().
         *
         * Example for writing:
         *
         *     char *value = "the new value";
         *     // yep, you pass the address to the variable
         *     // (needed for symmetry with other types and mpv_get_property)
         *     mpv_set_property(ctx, "property", MPV_FORMAT_STRING, &value);
         *
         * Or just use mpv_set_property_string().
         *
         */
        MPV_FORMAT_STRING = 1,
        /**
         * The basic type is char*. It returns the OSD property string, like
         * using ${property} in input.conf (see input.rst). In many cases, this
         * is the same as the raw string, but in other cases it's formatted for
         * display on OSD. It's intended to be human readable. Do not attempt to
         * parse these strings.
         *
         * Only valid when doing read access. The rest works like MPV_FORMAT_STRING.
         */
        MPV_FORMAT_OSD_STRING = 2,
        /**
         * The basic type is int. The only allowed values are 0 ("no")
         * and 1 ("yes").
         *
         * Example for reading:
         *
         *     int result;
         *     if (mpv_get_property(ctx, "property", MPV_FORMAT_FLAG, &result) < 0)
         *         goto error;
         *     printf("%s\n", result ? "true" : "false");
         *
         * Example for writing:
         *
         *     int flag = 1;
         *     mpv_set_property(ctx, "property", MPV_FORMAT_FLAG, &flag);
         */
        MPV_FORMAT_FLAG = 3,
        /**
         * The basic type is int64_t.
         */
        MPV_FORMAT_INT64 = 4,
        /**
         * The basic type is double.
         */
        MPV_FORMAT_DOUBLE = 5,
        /**
         * The type is mpv_node.
         *
         * For reading, you usually would pass a pointer to a stack-allocated
         * mpv_node value to mpv, and when you're done you call
         * mpv_free_node_contents(&node).
         * You're expected not to write to the data - if you have to, copy it
         * first (which you have to do manually).
         *
         * For writing, you construct your own mpv_node, and pass a pointer to the
         * API. The API will never write to your data (and copy it if needed), so
         * you're free to use any form of allocation or memory management you like.
         *
         * Warning: when reading, always check the mpv_node.format member. For
         *          example, properties might change their type in future versions
         *          of mpv, or sometimes even during runtime.
         *
         * Example for reading:
         *
         *     mpv_node result;
         *     if (mpv_get_property(ctx, "property", MPV_FORMAT_NODE, &result) < 0)
         *         goto error;
         *     printf("format=%d\n", (int)result.format);
         *     mpv_free_node_contents(&result).
         *
         * Example for writing:
         *
         *     mpv_node value;
         *     value.format = MPV_FORMAT_STRING;
         *     value.u.string = "hello";
         *     mpv_set_property(ctx, "property", MPV_FORMAT_NODE, &value);
         */
        MPV_FORMAT_NODE = 6,
        /**
         * Used with mpv_node only. Can usually not be used directly.
         */
        MPV_FORMAT_NODE_ARRAY = 7,
        /**
         * See MPV_FORMAT_NODE_ARRAY.
         */
        MPV_FORMAT_NODE_MAP = 8,
        /**
         * A raw, untyped byte array. Only used only with mpv_node, and only in
         * some very specific situations. (Some commands use it.)
         */
        MPV_FORMAT_BYTE_ARRAY = 9
    } mpv_format;

    typedef struct mpv_node {
        union {
            char* string;   /** valid if format==MPV_FORMAT_STRING */
            int flag;       /** valid if format==MPV_FORMAT_FLAG   */
            int64_t int64;  /** valid if format==MPV_FORMAT_INT64  */
            double double_; /** valid if format==MPV_FORMAT_DOUBLE */
            /**
             * valid if format==MPV_FORMAT_NODE_ARRAY
             *    or if format==MPV_FORMAT_NODE_MAP
             */
            struct mpv_node_list* list;
            /**
             * valid if format==MPV_FORMAT_BYTE_ARRAY
             */
            struct mpv_byte_array* ba;
        } u;
        /**
         * Type of the data stored in this struct. This value rules what members in
         * the given union can be accessed. The following formats are currently
         * defined to be allowed in mpv_node:
         *
         *  MPV_FORMAT_STRING       (u.string)
         *  MPV_FORMAT_FLAG         (u.flag)
         *  MPV_FORMAT_INT64        (u.int64)
         *  MPV_FORMAT_DOUBLE       (u.double_)
         *  MPV_FORMAT_NODE_ARRAY   (u.list)
         *  MPV_FORMAT_NODE_MAP     (u.list)
         *  MPV_FORMAT_BYTE_ARRAY   (u.ba)
         *  MPV_FORMAT_NONE         (no member)
         *
         * If you encounter a value you don't know, you must not make any
         * assumptions about the contents of union u.
         */
        mpv_format format;
    } mpv_node;

    typedef struct mpv_node_list {
        /**
         * Number of entries. Negative values are not allowed.
         */
        int num;
        /**
         * MPV_FORMAT_NODE_ARRAY:
         *  values[N] refers to value of the Nth item
         *
         * MPV_FORMAT_NODE_MAP:
         *  values[N] refers to value of the Nth key/value pair
         *
         * If num > 0, values[0] to values[num-1] (inclusive) are valid.
         * Otherwise, this can be NULL.
         */
        mpv_node* values;
        /**
         * MPV_FORMAT_NODE_ARRAY:
         *  unused (typically NULL), access is not allowed
         *
         * MPV_FORMAT_NODE_MAP:
         *  keys[N] refers to key of the Nth key/value pair. If num > 0, keys[0] to
         *  keys[num-1] (inclusive) are valid. Otherwise, this can be NULL.
         *  The keys are in random order. The only guarantee is that keys[N] belongs
         *  to the value values[N]. NULL keys are not allowed.
         */
        char** keys;
    } mpv_node_list;

    typedef struct mpv_byte_array {
        /**
         * Pointer to the data. In what format the data is stored is up to whatever
         * uses MPV_FORMAT_BYTE_ARRAY.
         */
        void* data;
        /**
         * Size of the data pointed to by ptr.
         */
        size_t size;
    } mpv_byte_array;

    MPV_EXPORT void mpv_free_node_contents(mpv_node* node);

    MPV_EXPORT int mpv_set_option(mpv_handle* ctx, const char* name, mpv_format format,
        void* data);

    MPV_EXPORT int mpv_set_option_string(mpv_handle* ctx, const char* name, const char* data);

    MPV_EXPORT int mpv_command(mpv_handle* ctx, const char** args);

    MPV_EXPORT int mpv_command_node(mpv_handle* ctx, mpv_node* args, mpv_node* result);


    MPV_EXPORT int mpv_command_ret(mpv_handle* ctx, const char** args, mpv_node* result);


    MPV_EXPORT int mpv_command_string(mpv_handle* ctx, const char* args);


    MPV_EXPORT int mpv_command_async(mpv_handle* ctx, uint64_t reply_userdata,
        const char** args);


    MPV_EXPORT int mpv_command_node_async(mpv_handle* ctx, uint64_t reply_userdata,
        mpv_node* args);


    MPV_EXPORT void mpv_abort_async_command(mpv_handle* ctx, uint64_t reply_userdata);


    MPV_EXPORT int mpv_set_property(mpv_handle* ctx, const char* name, mpv_format format,
        void* data);


    MPV_EXPORT int mpv_set_property_string(mpv_handle* ctx, const char* name, const char* data);


    MPV_EXPORT int mpv_set_property_async(mpv_handle* ctx, uint64_t reply_userdata,
        const char* name, mpv_format format, void* data);


    MPV_EXPORT int mpv_get_property(mpv_handle* ctx, const char* name, mpv_format format,
        void* data);


    MPV_EXPORT char* mpv_get_property_string(mpv_handle* ctx, const char* name);


    MPV_EXPORT char* mpv_get_property_osd_string(mpv_handle* ctx, const char* name);


    MPV_EXPORT int mpv_get_property_async(mpv_handle* ctx, uint64_t reply_userdata,
        const char* name, mpv_format format);


    MPV_EXPORT int mpv_observe_property(mpv_handle* mpv, uint64_t reply_userdata,
        const char* name, mpv_format format);

    MPV_EXPORT int mpv_unobserve_property(mpv_handle* mpv, uint64_t registered_reply_userdata);

    typedef enum mpv_event_id {
        MPV_EVENT_NONE = 0,
        MPV_EVENT_SHUTDOWN = 1,
        MPV_EVENT_LOG_MESSAGE = 2,
        MPV_EVENT_GET_PROPERTY_REPLY = 3,
        MPV_EVENT_SET_PROPERTY_REPLY = 4,
        MPV_EVENT_COMMAND_REPLY = 5,
        MPV_EVENT_START_FILE = 6,
        MPV_EVENT_END_FILE = 7,
        MPV_EVENT_FILE_LOADED = 8,
#if MPV_ENABLE_DEPRECATED
        MPV_EVENT_IDLE = 11,
        MPV_EVENT_TICK = 14,
#endif

        MPV_EVENT_CLIENT_MESSAGE = 16,
        MPV_EVENT_VIDEO_RECONFIG = 17,
        MPV_EVENT_AUDIO_RECONFIG = 18,
        MPV_EVENT_SEEK = 20,
        MPV_EVENT_PLAYBACK_RESTART = 21,
        MPV_EVENT_PROPERTY_CHANGE = 22,
        MPV_EVENT_QUEUE_OVERFLOW = 24,
        MPV_EVENT_HOOK = 25,
    } mpv_event_id;


    MPV_EXPORT const char* mpv_event_name(mpv_event_id event);

    typedef struct mpv_event_property {

        const char* name;

        mpv_format format;

        void* data;
    } mpv_event_property;


    typedef enum mpv_log_level {
        MPV_LOG_LEVEL_NONE = 0,    /// "no"    - disable absolutely all messages
        MPV_LOG_LEVEL_FATAL = 10,   /// "fatal" - critical/aborting errors
        MPV_LOG_LEVEL_ERROR = 20,   /// "error" - simple errors
        MPV_LOG_LEVEL_WARN = 30,   /// "warn"  - possible problems
        MPV_LOG_LEVEL_INFO = 40,   /// "info"  - informational message
        MPV_LOG_LEVEL_V = 50,   /// "v"     - noisy informational message
        MPV_LOG_LEVEL_DEBUG = 60,   /// "debug" - very noisy technical information
        MPV_LOG_LEVEL_TRACE = 70,   /// "trace" - extremely noisy
    } mpv_log_level;

    typedef struct mpv_event_log_message {

        const char* prefix;

        const char* level;

        const char* text;

        mpv_log_level log_level;
    } mpv_event_log_message;

    /// Since API version 1.9.
    typedef enum mpv_end_file_reason {

        MPV_END_FILE_REASON_EOF = 0,

        MPV_END_FILE_REASON_STOP = 2,

        MPV_END_FILE_REASON_QUIT = 3,

        MPV_END_FILE_REASON_ERROR = 4,

        MPV_END_FILE_REASON_REDIRECT = 5,
    } mpv_end_file_reason;

    /// Since API version 1.108.
    typedef struct mpv_event_start_file {

        int64_t playlist_entry_id;
    } mpv_event_start_file;

    typedef struct mpv_event_end_file {

        mpv_end_file_reason reason;

        int error;

        int64_t playlist_entry_id;

        int64_t playlist_insert_id;
        int playlist_insert_num_entries;
    } mpv_event_end_file;

    typedef struct mpv_event_client_message {
        int num_args;
        const char** args;
    } mpv_event_client_message;

    typedef struct mpv_event_hook {
        const char* name;
        uint64_t id;
    } mpv_event_hook;

    // Since API version 1.102.
    typedef struct mpv_event_command {
        mpv_node result;
    } mpv_event_command;

    typedef struct mpv_event {

        mpv_event_id event_id;

        int error;

        uint64_t reply_userdata;

        void* data;
    } mpv_event;


    MPV_EXPORT int mpv_event_to_node(mpv_node* dst, mpv_event* src);

    MPV_EXPORT int mpv_request_event(mpv_handle* ctx, mpv_event_id event, int enable);

    MPV_EXPORT int mpv_request_log_messages(mpv_handle* ctx, const char* min_level);

    MPV_EXPORT mpv_event* mpv_wait_event(mpv_handle* ctx, double timeout);

    MPV_EXPORT void mpv_wakeup(mpv_handle* ctx);

    MPV_EXPORT void mpv_set_wakeup_callback(mpv_handle* ctx, void (*cb)(void* d), void* d);


    MPV_EXPORT void mpv_wait_async_requests(mpv_handle* ctx);

    MPV_EXPORT int mpv_hook_add(mpv_handle* ctx, uint64_t reply_userdata,
        const char* name, int priority);

    MPV_EXPORT int mpv_hook_continue(mpv_handle* ctx, uint64_t id);

#if MPV_ENABLE_DEPRECATED

    MPV_EXPORT int mpv_get_wakeup_pipe(mpv_handle* ctx);

#endif




    typedef struct mpv_render_context mpv_render_context;


    typedef enum mpv_render_param_type {

        MPV_RENDER_PARAM_INVALID = 0,

        MPV_RENDER_PARAM_API_TYPE = 1,

        MPV_RENDER_PARAM_OPENGL_INIT_PARAMS = 2,

        MPV_RENDER_PARAM_OPENGL_FBO = 3,

        MPV_RENDER_PARAM_FLIP_Y = 4,

        MPV_RENDER_PARAM_DEPTH = 5,

        MPV_RENDER_PARAM_ICC_PROFILE = 6,

        MPV_RENDER_PARAM_AMBIENT_LIGHT = 7,

        MPV_RENDER_PARAM_X11_DISPLAY = 8,

        MPV_RENDER_PARAM_WL_DISPLAY = 9,

        MPV_RENDER_PARAM_ADVANCED_CONTROL = 10,

        MPV_RENDER_PARAM_NEXT_FRAME_INFO = 11,

        MPV_RENDER_PARAM_BLOCK_FOR_TARGET_TIME = 12,

        MPV_RENDER_PARAM_SKIP_RENDERING = 13,

        MPV_RENDER_PARAM_DRM_DISPLAY = 14,

        MPV_RENDER_PARAM_DRM_DRAW_SURFACE_SIZE = 15,

        MPV_RENDER_PARAM_DRM_DISPLAY_V2 = 16,

        MPV_RENDER_PARAM_SW_SIZE = 17,

        MPV_RENDER_PARAM_SW_FORMAT = 18,

        MPV_RENDER_PARAM_SW_STRIDE = 19,

        MPV_RENDER_PARAM_SW_POINTER = 20,
    } mpv_render_param_type;


#define MPV_RENDER_PARAM_DRM_OSD_SIZE MPV_RENDER_PARAM_DRM_DRAW_SURFACE_SIZE


    typedef struct mpv_render_param {
        enum mpv_render_param_type type;
        void* data;
    } mpv_render_param;

    // See render_gl.h
#define MPV_RENDER_API_TYPE_OPENGL "opengl"
// See section "Software renderer"
#define MPV_RENDER_API_TYPE_SW "sw"


    typedef enum mpv_render_frame_info_flag {

        MPV_RENDER_FRAME_INFO_PRESENT = 1 << 0,

        MPV_RENDER_FRAME_INFO_REDRAW = 1 << 1,

        MPV_RENDER_FRAME_INFO_REPEAT = 1 << 2,

        MPV_RENDER_FRAME_INFO_BLOCK_VSYNC = 1 << 3,
    } mpv_render_frame_info_flag;


    typedef struct mpv_render_frame_info {

        uint64_t flags;

        int64_t target_time;
    } mpv_render_frame_info;


    MPV_EXPORT int mpv_render_context_create(mpv_render_context** res, mpv_handle* mpv,
        mpv_render_param* params);


    MPV_EXPORT int mpv_render_context_set_parameter(mpv_render_context* ctx,
        mpv_render_param param);


    MPV_EXPORT int mpv_render_context_get_info(mpv_render_context* ctx,
        mpv_render_param param);

    typedef void (*mpv_render_update_fn)(void* cb_ctx);


    MPV_EXPORT void mpv_render_context_set_update_callback(mpv_render_context* ctx,
        mpv_render_update_fn callback,
        void* callback_ctx);


    MPV_EXPORT uint64_t mpv_render_context_update(mpv_render_context* ctx);


    typedef enum mpv_render_update_flag {
        /**
         * A new video frame must be rendered. mpv_render_context_render() must be
         * called.
         */
        MPV_RENDER_UPDATE_FRAME = 1 << 0,
    } mpv_render_context_flag;

    MPV_EXPORT int mpv_render_context_render(mpv_render_context* ctx, mpv_render_param* params);


    MPV_EXPORT void mpv_render_context_report_swap(mpv_render_context* ctx);

    MPV_EXPORT void mpv_render_context_free(mpv_render_context* ctx);



    typedef struct mpv_opengl_init_params {

        void* (*get_proc_address)(void* ctx, const char* name);

        void* get_proc_address_ctx;
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
        struct _drmModeAtomicReq** atomic_request_ptr;
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
        struct _drmModeAtomicReq** atomic_request_ptr;

        /**
         * DRM render node. Used for VAAPI interop.
         * Set to -1 if invalid.
         */
        int render_fd;
    } mpv_opengl_drm_params_v2;

#define mpv_opengl_drm_osd_size mpv_opengl_drm_draw_surface_size


    typedef int64_t(*mpv_stream_cb_read_fn)(void* cookie, char* buf, uint64_t nbytes);


    typedef int64_t(*mpv_stream_cb_seek_fn)(void* cookie, int64_t offset);


    typedef int64_t(*mpv_stream_cb_size_fn)(void* cookie);


    typedef void (*mpv_stream_cb_close_fn)(void* cookie);


    typedef void (*mpv_stream_cb_cancel_fn)(void* cookie);


    typedef struct mpv_stream_cb_info {
        void* cookie;
        mpv_stream_cb_read_fn read_fn;
        mpv_stream_cb_seek_fn seek_fn;
        mpv_stream_cb_size_fn size_fn;
        mpv_stream_cb_close_fn close_fn;
        mpv_stream_cb_cancel_fn cancel_fn; /* since API 1.106 */
    } mpv_stream_cb_info;


    typedef int (*mpv_stream_cb_open_ro_fn)(void* user_data, char* uri,
        mpv_stream_cb_info* info);

    MPV_EXPORT int mpv_stream_cb_add_ro(mpv_handle* ctx, const char* protocol, void* user_data,
        mpv_stream_cb_open_ro_fn open_fn);


#ifdef __cplusplus
}
#endif

#endif
