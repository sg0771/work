#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <assert.h>
#include "libimagequant.h"
//#include "mimalloc.h"


void av_free(void* ptr);
void* av_malloc(size_t size);

#ifdef _OPENMP
    #include <omp.h>
#else
#define omp_get_max_threads() 1
#define omp_get_thread_num()   1
#endif

static int   s_gamma_lut_init = 0;
static float s_gamma_lut[256];

typedef struct liq_image  liq_image;
typedef struct liq_result liq_result;

typedef struct liq_color {
    unsigned char b, g, r, a;
} liq_color;

typedef struct liq_palette {
    unsigned int count;
    liq_color entries[256];
} liq_palette;

typedef enum liq_error {
    LIQ_OK = 0,
    LIQ_QUALITY_TOO_LOW = 99,
    LIQ_VALUE_OUT_OF_RANGE = 100,
    LIQ_OUT_OF_MEMORY,
    LIQ_NOT_READY,
    LIQ_BITMAP_NOT_AVAILABLE,
    LIQ_BUFFER_TOO_SMALL,
    LIQ_INVALID_POINTER,
} liq_error;

LIQ_EXPORT liq_error liq_set_speed(liq_attr* attr, int speed);
LIQ_EXPORT liq_error liq_set_quality(liq_attr* attr, int minimum, int maximum);
LIQ_EXPORT void liq_image_destroy(liq_image* img);
LIQ_EXPORT liq_result* liq_quantize_image(liq_attr* options);
LIQ_EXPORT liq_error liq_set_dithering_level(liq_attr* attr, float dither_level);
LIQ_EXPORT liq_error liq_set_output_gamma(liq_attr* attr, double gamma);
LIQ_EXPORT const liq_palette* liq_get_palette(liq_attr* attr);
LIQ_EXPORT liq_error liq_write_remapped_image_rows(liq_attr* attr, unsigned char** row_pointers);
LIQ_EXPORT double liq_get_quantization_error(liq_result* result);
LIQ_EXPORT int liq_get_quantization_quality(liq_result* result);
LIQ_EXPORT void liq_result_destroy(liq_result*);

#ifndef MAX
#  define MAX(a,b)  ((a) > (b)? (a) : (b))
#  define MIN(a,b)  ((a) < (b)? (a) : (b))
#endif

#define MAX_DIFF 1e20

#define USE_SSE 1


#if USE_SSE
#  include <xmmintrin.h>
#  ifdef _MSC_VER
#    include <intrin.h>
#    define SSE_ALIGN
#  else
#    define SSE_ALIGN __attribute__ ((aligned (16)))
#    if defined(__i386__) && defined(__PIC__)
#       define cpuid(func,ax,bx,cx,dx)\
        __asm__ __volatile__ ( \
        "push %%ebx\n" \
        "cpuid\n" \
        "mov %%ebx, %1\n" \
        "pop %%ebx\n" \
        : "=a" (ax), "=r" (bx), "=c" (cx), "=d" (dx) \
        : "a" (func));
#    else
#       define cpuid(func,ax,bx,cx,dx)\
        __asm__ __volatile__ ("cpuid":\
        "=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));
#    endif
#endif
#else
#  define SSE_ALIGN
#endif

#if defined(__GNUC__) || defined (__llvm__)
#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define NEVER_INLINE __attribute__ ((noinline))
#elif defined(_MSC_VER)
#define inline __inline
#define restrict __restrict
#define ALWAYS_INLINE __forceinline
#define NEVER_INLINE __declspec(noinline)
#else
#define ALWAYS_INLINE inline
#define NEVER_INLINE
#endif

/* from pam.h */

typedef struct {
    unsigned char b, g, r, a;
} rgba_pixel;

typedef struct {
    float a, r, g, b;
} SSE_ALIGN f_pixel;

static const double internal_gamma = 0.5499;

void to_f_set_gamma( const double gamma);

/**
 Converts 8-bit color to internal gamma and premultiplied alpha.
 (premultiplied color space is much better for blending of semitransparent colors)
 */
inline static f_pixel to_f(const float gamma_lut[], const rgba_pixel px)
{
    float a = px.a / 255.f;

    return (f_pixel) {
        .a = a,
        .r = gamma_lut[px.r] * a,
        .g = gamma_lut[px.g] * a,
        .b = gamma_lut[px.b] * a,
    };
}

inline static rgba_pixel to_rgb(const float gamma, const f_pixel px)
{
    if (px.a < 1.f / 256.f) {
        return (rgba_pixel) { 0, 0, 0, 0 };
    }

    float r = px.r / px.a,
        g = px.g / px.a,
        b = px.b / px.a,
        a = px.a;

    r = powf(r, gamma / internal_gamma);
    g = powf(g, gamma / internal_gamma);
    b = powf(b, gamma / internal_gamma);

    // 256, because numbers are in range 1..255.9999… rounded down
    r *= 256.f;
    g *= 256.f;
    b *= 256.f;
    a *= 256.f;

    return (rgba_pixel) {
        .r = r >= 255.f ? 255 : r,
            .g = g >= 255.f ? 255 : g,
            .b = b >= 255.f ? 255 : b,
            .a = a >= 255.f ? 255 : a,
    };
}

ALWAYS_INLINE static double colordifference_ch(const double x, const double y, const double alphas);
inline static double colordifference_ch(const double x, const double y, const double alphas)
{
    // maximum of channel blended on white, and blended on black
    // premultiplied alpha and backgrounds 0/1 shorten the formula
    const double black = x - y, white = black + alphas;
    return black * black + white * white;
}

ALWAYS_INLINE static float colordifference_stdc(const f_pixel px, const f_pixel py);
inline static float colordifference_stdc(const f_pixel px, const f_pixel py)
{
    // px_b.rgb = px.rgb + 0*(1-px.a) // blend px on black
    // px_b.a   = px.a   + 1*(1-px.a)
    // px_w.rgb = px.rgb + 1*(1-px.a) // blend px on white
    // px_w.a   = px.a   + 1*(1-px.a)

    // px_b.rgb = px.rgb              // difference same as in opaque RGB
    // px_b.a   = 1
    // px_w.rgb = px.rgb - px.a       // difference simplifies to formula below
    // px_w.a   = 1

    // (px.rgb - px.a) - (py.rgb - py.a)
    // (px.rgb - py.rgb) + (py.a - px.a)

    const double alphas = py.a - px.a;
    return colordifference_ch(px.r, py.r, alphas) +
        colordifference_ch(px.g, py.g, alphas) +
        colordifference_ch(px.b, py.b, alphas);
}

ALWAYS_INLINE static double min_colordifference_ch(const double x, const double y, const double alphas);
inline static double min_colordifference_ch(const double x, const double y, const double alphas)
{
    const double black = x - y, white = black + alphas;
    return MIN(black * black, white * white) * 2.f;
}

/* least possible difference between colors (difference varies depending on background they're blended on) */
ALWAYS_INLINE static float min_colordifference(const f_pixel px, const f_pixel py);
inline static float min_colordifference(const f_pixel px, const f_pixel py)
{
    const double alphas = py.a - px.a;
    return min_colordifference_ch(px.r, py.r, alphas) +
        min_colordifference_ch(px.g, py.g, alphas) +
        min_colordifference_ch(px.b, py.b, alphas);
}

ALWAYS_INLINE static float colordifference(f_pixel px, f_pixel py);
inline static float colordifference(f_pixel px, f_pixel py)
{
#if USE_SSE
    const __m128 vpx = _mm_loadu_ps((const float*)&px);
    const __m128 vpy = _mm_loadu_ps((const float*)&py);

    // y.a - x.a
    __m128 alphas = _mm_sub_ss(vpy, vpx);
    alphas = _mm_shuffle_ps(alphas, alphas, 0); // copy first to all four

    __m128 onblack = _mm_sub_ps(vpx, vpy); // x - y
    __m128 onwhite = _mm_add_ps(onblack, alphas); // x - y + (y.a - x.a)

    onblack = _mm_mul_ps(onblack, onblack);
    onwhite = _mm_mul_ps(onwhite, onwhite);
    const __m128 max = _mm_add_ps(onwhite, onblack);

    // add rgb, not a
    const __m128 maxhl = _mm_movehl_ps(max, max);
    const __m128 tmp = _mm_add_ps(max, maxhl);
    const __m128 sum = _mm_add_ss(maxhl, _mm_shuffle_ps(tmp, tmp, 1));

    const float res = _mm_cvtss_f32(sum);
    assert(fabs(res - colordifference_stdc(px, py)) < 0.001);
    return res;
#else
    return colordifference_stdc(px, py);
#endif
}

/* from pamcmap.h */
union rgba_as_int {
    rgba_pixel rgba;
    unsigned int l;
};

typedef struct {
    f_pixel acolor;
    float adjusted_weight,   // perceptual weight changed to tweak how mediancut selects colors
        perceptual_weight; // number of pixels weighted by importance of different areas of the picture

    float color_weight;      // these two change every time histogram subset is sorted
    union {
        unsigned int sort_value;
        unsigned char likely_colormap_index;
    } tmp;
} hist_item;

typedef struct {
    hist_item* achv;
    double total_perceptual_weight;
    unsigned int size;
    unsigned int ignorebits;
} histogram;

typedef struct {
    f_pixel acolor;
    float popularity;
} colormap_item;

typedef struct colormap {
    unsigned int colors;
    struct colormap* subset_palette;
    colormap_item palette[];
} colormap;

struct acolorhist_arr_item {
    union rgba_as_int color;
    float perceptual_weight;
};

struct acolorhist_arr_head {
    unsigned int used, capacity;
    struct {
        union rgba_as_int color;
        float perceptual_weight;
    } inline1, inline2;
    struct acolorhist_arr_item* other_items;
};

struct acolorhash_table {
    struct mempool* mempool;
    unsigned int ignorebits, maxcolors, colors, cols, rows;
    unsigned int hash_size;
    unsigned int freestackp;
    struct acolorhist_arr_item* freestack[512];
    struct acolorhist_arr_head buckets[];
};

 void pam_freeacolorhash(struct acolorhash_table* acht);
 struct acolorhash_table* pam_allocacolorhash(unsigned int maxcolors, unsigned int surface, unsigned int ignorebits);
 histogram* pam_acolorhashtoacolorhist(const struct acolorhash_table* acht, const double gamma);
 bool pam_computeacolorhash(struct acolorhash_table* acht, const rgba_pixel* const pixels[], unsigned int cols, unsigned int rows, const unsigned char* importance_map);

 void pam_freeacolorhist(histogram* h);

 colormap* pam_colormap(unsigned int colors);
 colormap* pam_duplicate_colormap(colormap* map);
 void pam_freecolormap(colormap* c);

 colormap* mediancut(histogram* hist, const float min_opaque_val, unsigned int newcolors, const double target_mse, const double max_mse);

 struct nearest_map;
 struct nearest_map* nearest_init(const colormap* palette, const bool fast);
 unsigned int nearest_search(const struct nearest_map* map, const f_pixel px, const int palette_index_guess, const float min_opaque, float* diff);
 void nearest_deinit(struct nearest_map* map);

 void liq_blur(unsigned char* src, unsigned char* tmp, unsigned char* dst, unsigned int width, unsigned int height, unsigned int size);
 void liq_max3(unsigned char* src, unsigned char* dst, unsigned int width, unsigned int height);
 void liq_min3(unsigned char* src, unsigned char* dst, unsigned int width, unsigned int height);

// Spread memory touched by different threads at least 64B apart which I assume is the cache line size. This should avoid memory write contention.
#define VITER_CACHE_LINE_GAP ((64+sizeof(viter_state)-1)/sizeof(viter_state))

typedef struct {
    double a, r, g, b, total;
} viter_state;

typedef void (*viter_callback)(hist_item* item, float diff);

 void viter_init(const colormap* map, const unsigned int max_threads, viter_state state[]);
 void viter_update_color(const f_pixel acolor, const float value, const colormap* map, unsigned int match, const unsigned int thread, viter_state average_color[]);
 void viter_finalize(colormap* map, const unsigned int max_threads, const viter_state state[]);
 double viter_do_iteration(histogram* hist, colormap* const map, const float min_opaque_val, viter_callback callback, const bool fast_palette);

#define LIQ_HIGH_MEMORY_LIMIT (1<<26)  /* avoid allocating buffers larger than 64MB */

// each structure has a pointer as a unique identifier that allows type checking at run time
static const char *const liq_attr_magic = "liq_attr", *const liq_image_magic = "liq_image",
     *const liq_result_magic = "liq_result", *const liq_remapping_result_magic = "liq_remapping_result",
     *const liq_freed_magic = "free";
#define CHECK_STRUCT_TYPE(attr, kind) liq_crash_if_invalid_handle_pointer_given((const liq_attr*)attr, kind ## _magic)
#define CHECK_USER_POINTER(ptr) liq_crash_if_invalid_pointer_given(ptr)


struct liq_image {
    const char* magic_header;
    f_pixel* f_pixels;
    rgba_pixel** rows;
    double gamma;
    unsigned int width, height;
    unsigned char* noise, * edges, * dither_map;
    rgba_pixel* pixels, * temp_row;
    f_pixel* temp_f_row;
    float min_opaque_val;
    bool free_pixels, free_rows, free_rows_internal;

};
struct liq_attr {
    const char *magic_header;
    double target_mse, max_mse, voronoi_iteration_limit;
    float min_opaque_val;
    unsigned int max_colors, max_histogram_entries;
    unsigned int min_posterization_output /* user setting */, min_posterization_input /* speed setting */;
    unsigned int voronoi_iterations, feedback_loop_trials;
    bool last_index_transparent, use_contrast_maps, use_dither_map, fast_palette;
    unsigned int speed;
    int width;
    int height;
    liq_image* m_image;

    liq_result* m_remap;
};


typedef struct liq_remapping_result {
    const char *magic_header;
    unsigned char *pixels;
    colormap *palette;
    liq_palette int_palette;
    double gamma, palette_error;
    float dither_level;
    bool use_dither_map;
} liq_remapping_result;

struct liq_result {
    const char *magic_header;
    liq_remapping_result *remapping;
    colormap *palette;
    liq_palette int_palette;
    float dither_level;
    double gamma, palette_error;
    int min_posterization_output;
    bool use_dither_map, fast_palette;
};

static liq_result *pngquant_quantize(histogram *hist, const liq_attr *options, double gamma);
static void modify_alpha(liq_image *input_image, rgba_pixel *const row_pixels);
static void contrast_maps(liq_image *image);
static histogram *get_histogram(liq_image *input_image, const liq_attr *options);
static const rgba_pixel *liq_image_get_row_rgba(liq_image *input_image, unsigned int row);
static const f_pixel *liq_image_get_row_f(liq_image *input_image, unsigned int row);
static void liq_remapping_result_destroy(liq_remapping_result *result);


/* make it clear in backtrace when user-supplied handle points to invalid memory */
NEVER_INLINE LIQ_EXPORT bool liq_crash_if_invalid_handle_pointer_given(const liq_attr *user_supplied_pointer, const char *const expected_magic_header);
LIQ_EXPORT bool liq_crash_if_invalid_handle_pointer_given(const liq_attr *user_supplied_pointer, const char *const expected_magic_header)
{
    if (!user_supplied_pointer) {
        return false;
    }

    if (user_supplied_pointer->magic_header == liq_freed_magic) {
        fprintf(stderr, "%s used after being freed", expected_magic_header);
        // this is not normal error handling, this is programmer error that should crash the program.
        // program cannot safely continue if memory has been used after it's been freed.
        // abort() is nasty, but security vulnerability may be worse.
        abort();
    }

    return user_supplied_pointer->magic_header == expected_magic_header;
}

NEVER_INLINE LIQ_EXPORT bool liq_crash_if_invalid_pointer_given(void *pointer);
LIQ_EXPORT bool liq_crash_if_invalid_pointer_given(void *pointer)
{
    if (!pointer) {
        return false;
    }
    // Force a read from the given (potentially invalid) memory location in order to check early whether this crashes the program or not.
    // It doesn't matter what value is read, the code here is just to shut the compiler up about unused read.
    char test_access = *((volatile char *)pointer);
    return test_access || true;
}

static double quality_to_mse(long quality)
{
    if (quality == 0) {
        return MAX_DIFF;
    }
    if (quality == 100) {
        return 0;
    }

    // curve fudged to be roughly similar to quality of libjpeg
    // except lowest 10 for really low number of colors
    const double extra_low_quality_fudge = MAX(0,0.016/(0.001+quality) - 0.001);
    return extra_low_quality_fudge + 2.5/pow(210.0 + quality, 1.2) * (100.1-quality)/100.0;
}

static unsigned int mse_to_quality(double mse)
{
    for(int i=100; i > 0; i--) {
        if (mse <= quality_to_mse(i) + 0.000001) { // + epsilon for floating point errors
            return i;
        }
    }
    return 0;
}

LIQ_EXPORT liq_error liq_set_quality(liq_attr* attr, int minimum, int target)
{
    if (!CHECK_STRUCT_TYPE(attr, liq_attr)) return LIQ_INVALID_POINTER;
    if (target < 0 || target > 100 || target < minimum || minimum < 0) return LIQ_VALUE_OUT_OF_RANGE;

    attr->target_mse = quality_to_mse(target);
    attr->max_mse = quality_to_mse(minimum);
    return LIQ_OK;
}

LIQ_EXPORT int liq_get_min_quality(const liq_attr *attr)
{
    if (!CHECK_STRUCT_TYPE(attr, liq_attr)) return -1;
    return mse_to_quality(attr->max_mse);
}

LIQ_EXPORT int liq_get_max_quality(const liq_attr *attr)
{
    if (!CHECK_STRUCT_TYPE(attr, liq_attr)) return -1;
    return mse_to_quality(attr->target_mse);
}

LIQ_EXPORT liq_error liq_set_speed(liq_attr* attr, int speed)
{
    if (!CHECK_STRUCT_TYPE(attr, liq_attr)) return LIQ_INVALID_POINTER;
    if (speed < 1 || speed > 10) return LIQ_VALUE_OUT_OF_RANGE;

    int iterations = MAX(8-speed,0); iterations += iterations * iterations/2;
    attr->voronoi_iterations = iterations;
    attr->voronoi_iteration_limit = 1.0/(double)(1<<(23-speed));
    attr->feedback_loop_trials = MAX(56-9*speed, 0);

    attr->max_histogram_entries = (1<<17) + (1<<18)*(10-speed);
    attr->min_posterization_input = (speed >= 8) ? 1 : 0;
    attr->fast_palette = (speed >= 7);
    attr->use_dither_map = (speed <= (omp_get_max_threads() > 1 ? 7 : 5)); // parallelized dither map might speed up floyd remapping
    attr->use_contrast_maps = (speed <= 7) || attr->use_dither_map;
    attr->speed = speed;
    return LIQ_OK;
}

LIQ_EXPORT liq_error liq_set_output_gamma(liq_attr* attr, double gamma)
{
    if (gamma <= 0 || gamma >= 1.0) return LIQ_VALUE_OUT_OF_RANGE;

    if (attr->m_remap->remapping) {
        liq_remapping_result_destroy(attr->m_remap->remapping);
        attr->m_remap->remapping = NULL;
    }

    attr->m_remap->gamma = gamma;
    return LIQ_OK;
}

LIQ_EXPORT void liq_attr_destroy(liq_attr *attr)
{
    if (!CHECK_STRUCT_TYPE(attr, liq_attr)) {
        return;
    }
    attr->magic_header = liq_freed_magic;

    liq_image_destroy(attr->m_image);

    liq_result_destroy(attr->m_remap);
    av_free(attr);
}

LIQ_EXPORT liq_attr* liq_attr_create(int width, int height)
{
    liq_attr* attr = av_malloc(sizeof(liq_attr));

    attr->magic_header = liq_attr_magic;
    attr->max_colors = 256;
    attr->min_opaque_val = 1; // whether preserve opaque colors for IE (1.0=no, does not affect alpha)
    attr->last_index_transparent = false; // puts transparent color at last index. This is workaround for blu-ray subtitles.
    attr->target_mse = 0;
    attr->max_mse = MAX_DIFF;
    attr->width = width;
    attr->height = height;
    liq_set_speed(attr, 5);
    liq_set_quality(attr, 65, 85);

    attr->m_image = av_malloc(sizeof(liq_image));
    attr->m_image->magic_header = liq_image_magic;
    attr->m_image->width = width;
    attr->m_image->height = height;
    attr->m_image->gamma = 0.45455;
    attr->m_image->rows = av_malloc(sizeof(rgba_pixel*) * height);
    attr->m_image->min_opaque_val = attr->min_opaque_val;
    if (attr->min_opaque_val < 1.f) {
        attr->m_image->temp_row = av_malloc(sizeof(attr->m_image->temp_row[0]) * width * omp_get_max_threads());
        if (!attr->m_image->temp_row) return NULL;
    }
    attr->m_image->free_rows = true;
    attr->m_image->free_rows_internal = true;

    attr->m_remap = av_malloc(sizeof(liq_result));

    return attr;
}


//更新RGBA数据
LIQ_EXPORT void liq_attr_process(liq_attr* attr, uint8_t* src, uint8_t** dst1, uint8_t* pal) {
    rgba_pixel* pixels = src;
    for (int i = 0; i < attr->height; i++) {
        attr->m_image->rows[i] = pixels + attr->width * i;
    }
    liq_quantize_image(attr);//量化结果
    liq_set_output_gamma(attr, 0.45455);
    liq_set_dithering_level(attr, 1.0f);
    liq_write_remapped_image_rows(attr, dst1);
    const liq_palette* palette = liq_get_palette(attr);
    memcpy(pal, palette->entries, 256 * sizeof(int));
}


static bool liq_image_use_low_memory(liq_image *img)
{
    img->temp_f_row = av_malloc(sizeof(img->f_pixels[0]) * img->width * omp_get_max_threads());
    return img->temp_f_row != NULL;
}

static bool liq_image_should_use_low_memory(liq_image *img, const bool low_memory_hint)
{
    return img->width * img->height * sizeof(f_pixel) > (low_memory_hint ? LIQ_HIGH_MEMORY_LIMIT/8 : LIQ_HIGH_MEMORY_LIMIT);
}



inline static bool liq_image_can_use_rows(liq_image *img)
{
    const bool iebug = img->min_opaque_val < 1.f;
    return (img->rows && !iebug);
}

static const rgba_pixel *liq_image_get_row_rgba(liq_image *img, unsigned int row)
{
    if (liq_image_can_use_rows(img)) {
        return img->rows[row];
    }
    rgba_pixel *temp_row = img->temp_row + img->width * omp_get_thread_num();
    memcpy(temp_row, img->rows[row], img->width * sizeof(temp_row[0]));
    if (img->min_opaque_val < 1.f) modify_alpha(img, temp_row);
    return temp_row;
}

static void convert_row_to_f(liq_image *img, f_pixel *row_f_pixels, const unsigned int row, const float gamma_lut[])
{
    assert(row_f_pixels);
    assert(!USE_SSE || 0 == ((uintptr_t)row_f_pixels & 15));

    const rgba_pixel *const row_pixels = liq_image_get_row_rgba(img, row);

    for(unsigned int col=0; col < img->width; col++) {
        row_f_pixels[col] = to_f(gamma_lut, row_pixels[col]);
    }
}

static const f_pixel *liq_image_get_row_f(liq_image *img, unsigned int row)
{
    if (!img->f_pixels) {
        if (img->temp_f_row) {
            
            to_f_set_gamma(img->gamma);
            f_pixel *row_for_thread = img->temp_f_row + img->width * omp_get_thread_num();
            convert_row_to_f(img, row_for_thread, row, s_gamma_lut);
            return row_for_thread;
        }

        //assert(omp_get_thread_num() == 0);
        if (!liq_image_should_use_low_memory(img, false)) {
            img->f_pixels = av_malloc(sizeof(img->f_pixels[0]) * img->width * img->height);
        }
        if (!img->f_pixels) {
            if (!liq_image_use_low_memory(img)) return NULL;
            return liq_image_get_row_f(img, row);
        }

        to_f_set_gamma(img->gamma);
        for(unsigned int i=0; i < img->height; i++) {
            convert_row_to_f(img, &img->f_pixels[i*img->width], i, s_gamma_lut);
        }
    }
    return img->f_pixels + img->width * row;
}




static void liq_image_free_rgba_source(liq_image *input_image)
{
    if (input_image->free_pixels && input_image->pixels) {
        av_free(input_image->pixels);
        input_image->pixels = NULL;
    }

    if (input_image->free_rows && input_image->rows) {
        av_free(input_image->rows);
        input_image->rows = NULL;
    }
}

LIQ_EXPORT void liq_image_destroy(liq_image *input_image)
{
    if (!CHECK_STRUCT_TYPE(input_image, liq_image)) return;

    liq_image_free_rgba_source(input_image);

    if (input_image->noise) {
        av_free(input_image->noise);
    }

    if (input_image->edges) {
        av_free(input_image->edges);
    }

    if (input_image->dither_map) {
        av_free(input_image->dither_map);
    }

    if (input_image->f_pixels) {
        av_free(input_image->f_pixels);
    }

    if (input_image->temp_row) {
        av_free(input_image->temp_row);
    }

    input_image->magic_header = liq_freed_magic;
    av_free(input_image);
}

LIQ_EXPORT liq_result *liq_quantize_image(liq_attr *attr)
{
    histogram *hist = get_histogram(attr->m_image, attr);
    pngquant_quantize(hist, attr, attr->m_image->gamma);

    pam_freeacolorhist(hist);
    return attr->m_remap;
}

LIQ_EXPORT liq_error liq_set_dithering_level(liq_attr* attr, float dither_level)
{
    //if (!CHECK_STRUCT_TYPE(res, liq_result)) return LIQ_INVALID_POINTER;

    if (attr->m_remap->remapping) {
        liq_remapping_result_destroy(attr->m_remap->remapping);
        attr->m_remap->remapping = NULL;
    }

    if (attr->m_remap->dither_level < 0 || attr->m_remap->dither_level > 1.0f) return LIQ_VALUE_OUT_OF_RANGE;
    attr->m_remap->dither_level = dither_level;
    return LIQ_OK;
}

static liq_remapping_result *liq_remapping_result_create(liq_result *result)
{
    if (!CHECK_STRUCT_TYPE(result, liq_result)) {
        return NULL;
    }

    liq_remapping_result *res = av_malloc(sizeof(liq_remapping_result));
    if (!res) return NULL;
    *res = (liq_remapping_result) {
        .magic_header = liq_remapping_result_magic,
        .dither_level = result->dither_level,
        .use_dither_map = result->use_dither_map,
        .palette_error = result->palette_error,
        .gamma = result->gamma,
        .palette = pam_duplicate_colormap(result->palette),
    };
    return res;
}



static void liq_remapping_result_destroy(liq_remapping_result *result)
{
    if (!CHECK_STRUCT_TYPE(result, liq_remapping_result)) return;

    if (result->palette) pam_freecolormap(result->palette);
    if (result->pixels) av_free(result->pixels);

    result->magic_header = liq_freed_magic;
    av_free(result);
}

LIQ_EXPORT void liq_result_destroy(liq_result *res)
{
    if (!CHECK_STRUCT_TYPE(res, liq_result)) return;

    memset(&res->int_palette, 0, sizeof(liq_palette));

    if (res->remapping) {
        memset(&res->remapping->int_palette, 0, sizeof(liq_palette));
        liq_remapping_result_destroy(res->remapping);
    }

    pam_freecolormap(res->palette);

    res->magic_header = liq_freed_magic;
    av_free(res);
}

LIQ_EXPORT double liq_get_quantization_error(liq_result *result)
{
    if (!CHECK_STRUCT_TYPE(result, liq_result)) return -1;

    if (result->palette_error >= 0) {
        return result->palette_error*65536.0/6.0;
    }

    if (result->remapping && result->remapping->palette_error >= 0) {
        return result->remapping->palette_error*65536.0/6.0;
    }

    return result->palette_error;
}

LIQ_EXPORT int liq_get_quantization_quality(liq_result *result)
{
    if (!CHECK_STRUCT_TYPE(result, liq_result)) return -1;

    if (result->palette_error >= 0) {
        return mse_to_quality(result->palette_error);
    }

    if (result->remapping && result->remapping->palette_error >= 0) {
        return mse_to_quality(result->remapping->palette_error);
    }

    return result->palette_error;
}

static int compare_popularity(const void *ch1, const void *ch2)
{
    const float v1 = ((const colormap_item*)ch1)->popularity;
    const float v2 = ((const colormap_item*)ch2)->popularity;
    return v1 > v2 ? -1 : 1;
}

static void sort_palette_qsort(colormap *map, int start, int nelem)
{
    qsort(map->palette + start, nelem, sizeof(map->palette[0]), compare_popularity);
}

#define SWAP_PALETTE(map, a,b) { \
    const colormap_item tmp = (map)->palette[(a)]; \
    (map)->palette[(a)] = (map)->palette[(b)]; \
    (map)->palette[(b)] = tmp; }

static void sort_palette(colormap *map, const liq_attr *options)
{
    /*
    ** Step 3.5 [GRR]: remap the palette colors so that all entries with
    ** the maximal alpha value (i.e., fully opaque) are at the end and can
    ** therefore be omitted from the tRNS chunk.
    */
    if (options->last_index_transparent) {
    	for(unsigned int i=0; i < map->colors; i++) {
    	    if (map->palette[i].acolor.a < 1.0/256.0) {
        		const unsigned int old = i, transparent_dest = map->colors-1;

        		SWAP_PALETTE(map, transparent_dest, old);

        		/* colors sorted by popularity make pngs slightly more compressible */
        		sort_palette_qsort(map, 0, map->colors-1);
        		return;
            }
        }
    }
    /* move transparent colors to the beginning to shrink trns chunk */
    unsigned int num_transparent=0;
    for(unsigned int i=0; i < map->colors; i++) {
        if (map->palette[i].acolor.a < 255.0/256.0) {
            // current transparent color is swapped with earlier opaque one
            if (i != num_transparent) {
                SWAP_PALETTE(map, num_transparent, i);
                i--;
            }
            num_transparent++;
        }
    }

    /* colors sorted by popularity make pngs slightly more compressible
     * opaque and transparent are sorted separately
     */
    sort_palette_qsort(map, 0, num_transparent);
    sort_palette_qsort(map, num_transparent, map->colors-num_transparent);

    if (map->colors > 16) {
        SWAP_PALETTE(map, 7, 1); // slightly improves compression
        SWAP_PALETTE(map, 8, 2);
        SWAP_PALETTE(map, 9, 3);
    }
}

inline static unsigned int posterize_channel(unsigned int color, unsigned int bits)
{
    return (color & ~((1<<bits)-1)) | (color >> (8-bits));
}

static void set_rounded_palette(liq_palette *const dest, colormap *const map, const double gamma, unsigned int posterize)
{
    to_f_set_gamma(gamma);

    dest->count = map->colors;
    for(unsigned int x = 0; x < map->colors; ++x) {
        rgba_pixel px = to_rgb(gamma, map->palette[x].acolor);

        px.r = posterize_channel(px.r, posterize);
        px.g = posterize_channel(px.g, posterize);
        px.b = posterize_channel(px.b, posterize);
        px.a = posterize_channel(px.a, posterize);

        map->palette[x].acolor = to_f(s_gamma_lut, px); /* saves rounding error introduced by to_rgb, which makes remapping & dithering more accurate */

        if (!px.a) {
            px.r = 'L'; px.g = 'i'; px.b = 'q';
        }

        dest->entries[x] = (liq_color){.r=px.r,.g=px.g,.b=px.b,.a=px.a};
    }
}

LIQ_EXPORT const liq_palette *liq_get_palette(liq_attr* attr)
{
    //if (!CHECK_STRUCT_TYPE(result, liq_result)) return NULL;

    if (attr->m_remap->remapping && attr->m_remap->remapping->int_palette.count) {
        return &attr->m_remap->remapping->int_palette;
    }

    if (!attr->m_remap->int_palette.count) {
        set_rounded_palette(&attr->m_remap->int_palette,
            attr->m_remap->palette,
            attr->m_remap->gamma,
            attr->m_remap->min_posterization_output);
    }
    return &attr->m_remap->int_palette;
}

static float remap_to_palette(liq_image *const input_image, unsigned char *const *const output_pixels, colormap *const map, const bool fast)
{
    const int rows = input_image->height;
    const unsigned int cols = input_image->width;
    const float min_opaque_val = input_image->min_opaque_val;
    double remapping_error=0;

    if (!liq_image_get_row_f(input_image, 0)) { // trigger lazy conversion
        return -1;
    }

    struct nearest_map *const n = nearest_init(map, fast);

    const unsigned int max_threads = omp_get_max_threads();
    viter_state *average_color = _alloca((VITER_CACHE_LINE_GAP+map->colors) * max_threads * sizeof(viter_state));
    viter_init(map, max_threads, average_color);

    int row;
    #pragma omp parallel for if (rows*cols > 3000) \
        schedule(static) default(none) shared(average_color) reduction(+:remapping_error)
    for( row = 0; row < rows; ++row) {
        const f_pixel *const row_pixels = liq_image_get_row_f(input_image, row);
        unsigned int last_match=0;
        for(unsigned int col = 0; col < cols; ++col) {
            f_pixel px = row_pixels[col];
            float diff;

            output_pixels[row][col] = last_match = nearest_search(n, px, last_match, min_opaque_val, &diff);

            remapping_error += diff;
            viter_update_color(px, 1.0, map, last_match, omp_get_thread_num(), average_color);
        }
    }

    viter_finalize(map, max_threads, average_color);

    nearest_deinit(n);

    return remapping_error / (input_image->width * input_image->height);
}

inline static f_pixel get_dithered_pixel(const float dither_level, const float max_dither_error, const f_pixel thiserr, const f_pixel px)
{
    /* Use Floyd-Steinberg errors to adjust actual color. */
    const float sr = thiserr.r * dither_level,
                sg = thiserr.g * dither_level,
                sb = thiserr.b * dither_level,
                sa = thiserr.a * dither_level;

    float ratio = 1.0;

    // allowing some overflow prevents undithered bands caused by clamping of all channels
         if (px.r + sr > 1.03) ratio = MIN(ratio, (1.03-px.r)/sr);
    else if (px.r + sr < 0)    ratio = MIN(ratio, px.r/-sr);
         if (px.g + sg > 1.03) ratio = MIN(ratio, (1.03-px.g)/sg);
    else if (px.g + sg < 0)    ratio = MIN(ratio, px.g/-sg);
         if (px.b + sb > 1.03) ratio = MIN(ratio, (1.03-px.b)/sb);
    else if (px.b + sb < 0)    ratio = MIN(ratio, px.b/-sb);

    float a = px.a + sa;
         if (a > 1.0) { a = 1.0; }
    else if (a < 0)   { a = 0; }

    // If dithering error is crazy high, don't propagate it that much
    // This prevents crazy geen pixels popping out of the blue (or red or black! ;)
    const float dither_error = sr*sr + sg*sg + sb*sb + sa*sa;
    if (dither_error > max_dither_error) {
        ratio *= 0.8;
    } else if (dither_error < 2.f/256.f/256.f) {
        // don't dither areas that don't have noticeable error — makes file smaller
        return px;
    }

     return (f_pixel){
         .r=px.r + sr * ratio,
         .g=px.g + sg * ratio,
         .b=px.b + sb * ratio,
         .a=a,
     };
}

/**
  Uses edge/noise map to apply dithering only to flat areas. Dithering on edges creates jagged lines, and noisy areas are "naturally" dithered.

  If output_image_is_remapped is true, only pixels noticeably changed by error diffusion will be written to output image.
 */
static void remap_to_palette_floyd(liq_image *input_image, unsigned char *const output_pixels[], const colormap *map, const float max_dither_error, const bool use_dither_map, const bool output_image_is_remapped, float base_dithering_level)
{
    const unsigned int rows = input_image->height, cols = input_image->width;
    const unsigned char *dither_map = use_dither_map ? (input_image->dither_map ? input_image->dither_map : input_image->edges) : NULL;
    const float min_opaque_val = input_image->min_opaque_val;

    const colormap_item *acolormap = map->palette;

    struct nearest_map *const n = nearest_init(map, false);

    /* Initialize Floyd-Steinberg error vectors. */
    f_pixel *restrict thiserr, *restrict nexterr;
    thiserr = av_malloc((cols + 2) * sizeof(*thiserr) * 2); // +2 saves from checking out of bounds access
    nexterr = thiserr + (cols + 2);
    srand(12345); /* deterministic dithering is better for comparing results */
    if (!thiserr) return;

    for (unsigned int col = 0; col < cols + 2; ++col) {
        const double rand_max = RAND_MAX;
        thiserr[col].r = ((double)rand() - rand_max/2.0)/rand_max/255.0;
        thiserr[col].g = ((double)rand() - rand_max/2.0)/rand_max/255.0;
        thiserr[col].b = ((double)rand() - rand_max/2.0)/rand_max/255.0;
        thiserr[col].a = ((double)rand() - rand_max/2.0)/rand_max/255.0;
    }

    // response to this value is non-linear and without it any value < 0.8 would give almost no dithering
    base_dithering_level = 1.0 - (1.0-base_dithering_level)*(1.0-base_dithering_level)*(1.0-base_dithering_level);

    if (dither_map) {
        base_dithering_level *= 1.0/255.0; // convert byte to float
    }
    base_dithering_level *= 15.0/16.0; // prevent small errors from accumulating

    bool fs_direction = true;
    unsigned int last_match=0;
    for (unsigned int row = 0; row < rows; ++row) {
        memset(nexterr, 0, (cols + 2) * sizeof(*nexterr));

        unsigned int col = (fs_direction) ? 0 : (cols - 1);
        const f_pixel *const row_pixels = liq_image_get_row_f(input_image, row);

        do {
            float dither_level = base_dithering_level;
            if (dither_map) {
                dither_level *= dither_map[row*cols + col];
            }

            const f_pixel spx = get_dithered_pixel(dither_level, max_dither_error, thiserr[col + 1], row_pixels[col]);

            const unsigned int guessed_match = output_image_is_remapped ? output_pixels[row][col] : last_match;
            output_pixels[row][col] = last_match = nearest_search(n, spx, guessed_match, min_opaque_val, NULL);

            const f_pixel xp = acolormap[last_match].acolor;
            f_pixel err = {
                .r = (spx.r - xp.r),
                .g = (spx.g - xp.g),
                .b = (spx.b - xp.b),
                .a = (spx.a - xp.a),
            };

            // If dithering error is crazy high, don't propagate it that much
            // This prevents crazy geen pixels popping out of the blue (or red or black! ;)
            if (err.r*err.r + err.g*err.g + err.b*err.b + err.a*err.a > max_dither_error) {
                dither_level *= 0.75;
            }

            const float colorimp = (3.0f + acolormap[last_match].acolor.a)/4.0f * dither_level;
            err.r *= colorimp;
            err.g *= colorimp;
            err.b *= colorimp;
            err.a *= dither_level;

            /* Propagate Floyd-Steinberg error terms. */
            if (fs_direction) {
                thiserr[col + 2].a += err.a * (7.f/16.f);
                thiserr[col + 2].r += err.r * (7.f/16.f);
                thiserr[col + 2].g += err.g * (7.f/16.f);
                thiserr[col + 2].b += err.b * (7.f/16.f);

                nexterr[col + 2].a  = err.a * (1.f/16.f);
                nexterr[col + 2].r  = err.r * (1.f/16.f);
                nexterr[col + 2].g  = err.g * (1.f/16.f);
                nexterr[col + 2].b  = err.b * (1.f/16.f);

                nexterr[col + 1].a += err.a * (5.f/16.f);
                nexterr[col + 1].r += err.r * (5.f/16.f);
                nexterr[col + 1].g += err.g * (5.f/16.f);
                nexterr[col + 1].b += err.b * (5.f/16.f);

                nexterr[col    ].a += err.a * (3.f/16.f);
                nexterr[col    ].r += err.r * (3.f/16.f);
                nexterr[col    ].g += err.g * (3.f/16.f);
                nexterr[col    ].b += err.b * (3.f/16.f);

            } else {
                thiserr[col    ].a += err.a * (7.f/16.f);
                thiserr[col    ].r += err.r * (7.f/16.f);
                thiserr[col    ].g += err.g * (7.f/16.f);
                thiserr[col    ].b += err.b * (7.f/16.f);

                nexterr[col    ].a  = err.a * (1.f/16.f);
                nexterr[col    ].r  = err.r * (1.f/16.f);
                nexterr[col    ].g  = err.g * (1.f/16.f);
                nexterr[col    ].b  = err.b * (1.f/16.f);

                nexterr[col + 1].a += err.a * (5.f/16.f);
                nexterr[col + 1].r += err.r * (5.f/16.f);
                nexterr[col + 1].g += err.g * (5.f/16.f);
                nexterr[col + 1].b += err.b * (5.f/16.f);

                nexterr[col + 2].a += err.a * (3.f/16.f);
                nexterr[col + 2].r += err.r * (3.f/16.f);
                nexterr[col + 2].g += err.g * (3.f/16.f);
                nexterr[col + 2].b += err.b * (3.f/16.f);
            }

            // remapping is done in zig-zag
            if (fs_direction) {
                ++col;
                if (col >= cols) break;
            } else {
                if (col <= 0) break;
                --col;
            }
        } while(1);

        f_pixel *const temperr = thiserr;
        thiserr = nexterr;
        nexterr = temperr;
        fs_direction = !fs_direction;
    }

    av_free(MIN(thiserr, nexterr)); // MIN because pointers were swapped
    nearest_deinit(n);
}


/* histogram contains information how many times each color is present in the image, weighted by importance_map */
static histogram *get_histogram(liq_image *input_image, const liq_attr *options)
{
    unsigned int ignorebits=MAX(options->min_posterization_output, options->min_posterization_input);
    const unsigned int cols = input_image->width, rows = input_image->height;

    if (!input_image->noise && options->use_contrast_maps) {
        contrast_maps(input_image);
    }

    unsigned int maxcolors = options->max_histogram_entries;

    struct acolorhash_table *acht;
    const bool all_rows_at_once = liq_image_can_use_rows(input_image);
    do {
        acht = pam_allocacolorhash(maxcolors, rows*cols, ignorebits);
        if (!acht) return NULL;

        // histogram uses noise contrast map for importance. Color accuracy in noisy areas is not very important.
        // noise map does not include edges to avoid ruining anti-aliasing
        for(unsigned int row=0; row < rows; row++) {
            bool added_ok;
            if (all_rows_at_once) {
                added_ok = pam_computeacolorhash(acht, (const rgba_pixel *const *)input_image->rows, cols, rows, input_image->noise);
                if (added_ok) break;
            } else {
                const rgba_pixel* rows_p[1] = { liq_image_get_row_rgba(input_image, row) };
                added_ok = pam_computeacolorhash(acht, rows_p, cols, 1, input_image->noise ? &input_image->noise[row * cols] : NULL);
            }
            if (!added_ok) {
                ignorebits++;
               // liq_verbose_printf(options, "  too many colors! Scaling colors to improve clustering... %d", ignorebits);
                pam_freeacolorhash(acht);
                acht = NULL;
                break;
            }
        }
    } while(!acht);

    if (input_image->noise) {
        av_free(input_image->noise);
        input_image->noise = NULL;
    }

    if (input_image->free_pixels && input_image->f_pixels) {
        liq_image_free_rgba_source(input_image); // bow can free the RGBA source if copy has been made in f_pixels
    }

    histogram *hist = pam_acolorhashtoacolorhist(acht, input_image->gamma);
    pam_freeacolorhash(acht);
    return hist;
}

static void modify_alpha(liq_image *input_image, rgba_pixel *const row_pixels)
{
    /* IE6 makes colors with even slightest transparency completely transparent,
       thus to improve situation in IE, make colors that are less than ~10% transparent
       completely opaque */

    const float min_opaque_val = input_image->min_opaque_val;
    const float almost_opaque_val = min_opaque_val * 169.f/256.f;
    const unsigned int almost_opaque_val_int = (min_opaque_val * 169.f/256.f)*255.f;

    for(unsigned int col = 0; col < input_image->width; col++) {
        const rgba_pixel px = row_pixels[col];

        /* ie bug: to avoid visible step caused by forced opaqueness, linearily raise opaqueness of almost-opaque colors */
        if (px.a >= almost_opaque_val_int) {
            float al = px.a / 255.f;
            al = almost_opaque_val + (al-almost_opaque_val) * (1.f-almost_opaque_val) / (min_opaque_val-almost_opaque_val);
            al *= 256.f;
            row_pixels[col].a = al >= 255.f ? 255 : al;
        }
    }
}

/**
 Builds two maps:
    noise - approximation of areas with high-frequency noise, except straight edges. 1=flat, 0=noisy.
    edges - noise map including all edges
 */
static void contrast_maps(liq_image *image)
{
    const int cols = image->width, rows = image->height;
    if (cols < 4 || rows < 4 || (3*cols*rows) > LIQ_HIGH_MEMORY_LIMIT) {
        return;
    }

    unsigned char *restrict noise = av_malloc(cols*rows);
    unsigned char *restrict edges = av_malloc(cols*rows);
    unsigned char *restrict tmp = av_malloc(cols*rows);

    if (!noise || !edges || !tmp) {
        return;
    }

    const f_pixel *curr_row, *prev_row, *next_row;
    curr_row = prev_row = next_row = liq_image_get_row_f(image, 0);

    for (int j=0; j < rows; j++) {
        prev_row = curr_row;
        curr_row = next_row;
        next_row = liq_image_get_row_f(image, MIN(rows-1,j+1));

        f_pixel prev, curr = curr_row[0], next=curr;
        for (int i=0; i < cols; i++) {
            prev=curr;
            curr=next;
            next = curr_row[MIN(cols-1,i+1)];

            // contrast is difference between pixels neighbouring horizontally and vertically
            const float a = fabsf(prev.a+next.a - curr.a*2.f),
                        r = fabsf(prev.r+next.r - curr.r*2.f),
                        g = fabsf(prev.g+next.g - curr.g*2.f),
                        b = fabsf(prev.b+next.b - curr.b*2.f);

            const f_pixel prevl = prev_row[i];
            const f_pixel nextl = next_row[i];

            const float a1 = fabsf(prevl.a+nextl.a - curr.a*2.f),
                        r1 = fabsf(prevl.r+nextl.r - curr.r*2.f),
                        g1 = fabsf(prevl.g+nextl.g - curr.g*2.f),
                        b1 = fabsf(prevl.b+nextl.b - curr.b*2.f);

            const float horiz = MAX(MAX(a,r),MAX(g,b));
            const float vert = MAX(MAX(a1,r1),MAX(g1,b1));
            const float edge = MAX(horiz,vert);
            float z = edge - fabsf(horiz-vert)*.5f;
            z = 1.f - MAX(z,MIN(horiz,vert));
            z *= z; // noise is amplified
            z *= z;

            z *= 256.f;
            noise[j*cols+i] = z < 256 ? z : 255;
            z = (1.f-edge)*256.f;
            edges[j*cols+i] = z < 256 ? z : 255;
        }
    }

    // noise areas are shrunk and then expanded to remove thin edges from the map
    liq_max3(noise, tmp, cols, rows);
    liq_max3(tmp, noise, cols, rows);

    liq_blur(noise, tmp, noise, cols, rows, 3);

    liq_max3(noise, tmp, cols, rows);

    liq_min3(tmp, noise, cols, rows);
    liq_min3(noise, tmp, cols, rows);
    liq_min3(tmp, noise, cols, rows);

    liq_min3(edges, tmp, cols, rows);
    liq_max3(tmp, edges, cols, rows);
    for(int i=0; i < cols*rows; i++) edges[i] = MIN(noise[i], edges[i]);

    av_free(tmp);

    image->noise = noise;
    image->edges = edges;
}

/**
 * Builds map of neighbor pixels mapped to the same palette entry
 *
 * For efficiency/simplicity it mainly looks for same consecutive pixels horizontally
 * and peeks 1 pixel above/below. Full 2d algorithm doesn't improve it significantly.
 * Correct flood fill doesn't have visually good properties.
 */
static void update_dither_map(unsigned char *const *const row_pointers, liq_image *input_image)
{
    const unsigned int width = input_image->width;
    const unsigned int height = input_image->height;
    unsigned char *const edges = input_image->edges;

    for(unsigned int row=0; row < height; row++) {
        unsigned char lastpixel = row_pointers[row][0];
        unsigned int lastcol=0;

        for(unsigned int col=1; col < width; col++) {
            const unsigned char px = row_pointers[row][col];

            if (px != lastpixel || col == width-1) {
                float neighbor_count = 2.5f + col-lastcol;

                unsigned int i=lastcol;
                while(i < col) {
                    if (row > 0) {
                        unsigned char pixelabove = row_pointers[row-1][i];
                        if (pixelabove == lastpixel) neighbor_count += 1.f;
                    }
                    if (row < height-1) {
                        unsigned char pixelbelow = row_pointers[row+1][i];
                        if (pixelbelow == lastpixel) neighbor_count += 1.f;
                    }
                    i++;
                }

                while(lastcol <= col) {
                    float e = edges[row*width + lastcol] / 255.f;
                    e *= 1.f - 2.5f/neighbor_count;
                    edges[row*width + lastcol++] = e * 255.f;
                }
                lastpixel = px;
            }
        }
    }
    input_image->dither_map = input_image->edges;
    input_image->edges = NULL;
}

static void adjust_histogram_callback(hist_item *item, float diff)
{
    item->adjusted_weight = (item->perceptual_weight+item->adjusted_weight) * (sqrtf(1.f+diff));
}

/**
 Repeats mediancut with different histogram weights to find palette with minimum error.

 feedback_loop_trials controls how long the search will take. < 0 skips the iteration.
 */
static colormap *find_best_palette(histogram *hist, const liq_attr *options, double *palette_error_p)
{
    unsigned int max_colors = options->max_colors;
    // if output is posterized it doesn't make sense to aim for perfrect colors, so increase target_mse
    // at this point actual gamma is not set, so very conservative posterization estimate is used
    const double target_mse = MAX(options->target_mse, pow((1<<options->min_posterization_output)/1024.0, 2));
    int feedback_loop_trials = options->feedback_loop_trials;
    colormap *acolormap = NULL;
    double least_error = MAX_DIFF;
    double target_mse_overshoot = feedback_loop_trials>0 ? 1.05 : 1.0;
    const double percent = (double)(feedback_loop_trials>0?feedback_loop_trials:1)/100.0;

    do {
        colormap *newmap = mediancut(hist, options->min_opaque_val, max_colors,
            target_mse * target_mse_overshoot, MAX(MAX(90.0/65536.0, target_mse), 
                least_error)*1.2);
        if (!newmap) {
            return NULL;
        }

        if (feedback_loop_trials <= 0) {
            return newmap;
        }

        // after palette has been created, total error (MSE) is calculated to keep the best palette
        // at the same time Voronoi iteration is done to improve the palette
        // and histogram weights are adjusted based on remapping error to give more weight to poorly matched colors

        const bool first_run_of_target_mse = !acolormap && target_mse > 0;
        double total_error = viter_do_iteration(hist, newmap, options->min_opaque_val, first_run_of_target_mse ? NULL : adjust_histogram_callback, !acolormap || options->fast_palette);

        // goal is to increase quality or to reduce number of colors used if quality is good enough
        if (!acolormap || total_error < least_error || (total_error <= target_mse && newmap->colors < max_colors)) {
            if (acolormap) pam_freecolormap(acolormap);
            acolormap = newmap;

            if (total_error < target_mse && total_error > 0) {
                // voronoi iteration improves quality above what mediancut aims for
                // this compensates for it, making mediancut aim for worse
                target_mse_overshoot = MIN(target_mse_overshoot*1.25, target_mse/total_error);
            }

            least_error = total_error;

            // if number of colors could be reduced, try to keep it that way
            // but allow extra color as a bit of wiggle room in case quality can be improved too
            max_colors = MIN(newmap->colors+1, max_colors);

            feedback_loop_trials -= 1; // asymptotic improvement could make it go on forever
        } else {
            for(unsigned int j=0; j < hist->size; j++) {
                hist->achv[j].adjusted_weight = (hist->achv[j].perceptual_weight + hist->achv[j].adjusted_weight)/2.0;
            }

            target_mse_overshoot = 1.0;
            feedback_loop_trials -= 6;
            // if error is really bad, it's unlikely to improve, so end sooner
            if (total_error > least_error*4) feedback_loop_trials -= 3;
            pam_freecolormap(newmap);
        }

        //liq_verbose_printf(options, "  selecting colors...%d%%",100-MAX(0,(int)(feedback_loop_trials/percent)));
    }
    while(feedback_loop_trials > 0);

    // likely_colormap_index (used and set in viter_do_iteration) can't point to index outside colormap
    if (acolormap->colors < 256) {
	for(unsigned int j=0; j < hist->size; j++) {
	    if (hist->achv[j].tmp.likely_colormap_index >= acolormap->colors) {
		hist->achv[j].tmp.likely_colormap_index = 0; // actual value doesn't matter, as the guess is out of date anyway
	    }
	}
    }
    *palette_error_p = least_error;
    return acolormap;
}

static liq_result *pngquant_quantize(histogram *hist, const liq_attr *options, const double gamma)
{
    colormap *acolormap;
    double palette_error = -1;

    // no point having perfect match with imperfect colors (ignorebits > 0)
    const bool fast_palette = options->fast_palette || hist->ignorebits > 0;

    // If image has few colors to begin with (and no quality degradation is required)
    // then it's possible to skip quantization entirely
    if (hist->size <= options->max_colors && options->target_mse == 0) {
        acolormap = pam_colormap(hist->size);
        for(unsigned int i=0; i < hist->size; i++) {
            acolormap->palette[i].acolor = hist->achv[i].acolor;
            acolormap->palette[i].popularity = hist->achv[i].perceptual_weight;
        }
        palette_error = 0;
    } else {
        acolormap = find_best_palette(hist, options, &palette_error);
        if (!acolormap) {
            return NULL;
        }

        // Voronoi iteration approaches local minimum for the palette
        const double max_mse = options->max_mse;
        const double iteration_limit = options->voronoi_iteration_limit;
        unsigned int iterations = options->voronoi_iterations;

        if (!iterations && palette_error < 0 && max_mse < MAX_DIFF) iterations = 1; // otherwise total error is never calculated and MSE limit won't work

        if (iterations) {
  
            double previous_palette_error = MAX_DIFF;

            for(unsigned int i=0; i < iterations; i++) {
                palette_error = viter_do_iteration(hist, acolormap, options->min_opaque_val, NULL, i==0 || options->fast_palette);

                if (fabs(previous_palette_error-palette_error) < iteration_limit) {
                    break;
                }

                if (palette_error > max_mse*1.5) { // probably hopeless
                    if (palette_error > max_mse*3.0) break; // definitely hopeless
                    iterations++;
                }

                previous_palette_error = palette_error;
            }
        }

        if (palette_error > max_mse) {
            pam_freecolormap(acolormap);
            return NULL;
        }
    }

    sort_palette(acolormap, options);

  
    options->m_remap->magic_header = liq_result_magic;
    options->m_remap->palette = acolormap;
    options->m_remap->palette_error = palette_error;
    options->m_remap->fast_palette = fast_palette;
    options->m_remap->use_dither_map = options->use_dither_map;
    options->m_remap->gamma = gamma;
    options->m_remap->min_posterization_output = options->min_posterization_output;

    return options->m_remap;
}

LIQ_EXPORT liq_error liq_write_remapped_image_rows(liq_attr* attr, 
    unsigned char **row_pointers)
{
    if (attr->m_remap->remapping) {
        liq_remapping_result_destroy(attr->m_remap->remapping);
    }
    liq_remapping_result *const result = attr->m_remap->remapping = 
        liq_remapping_result_create(attr->m_remap);
    if (!result) return LIQ_OUT_OF_MEMORY;

    if (!attr->m_image->edges && !attr->m_image->dither_map && attr->m_remap->use_dither_map) {
        contrast_maps(attr->m_image);
    }

    /*
     ** Step 4: map the colors in the image to their closest match in the
     ** new colormap, and write 'em out.
     */

    float remapping_error = result->palette_error;
    if (result->dither_level == 0) {
        set_rounded_palette(&result->int_palette, result->palette, result->gamma, 
            attr->m_remap->min_posterization_output);
        remapping_error = remap_to_palette(attr->m_image, row_pointers,
            result->palette, attr->m_remap->fast_palette);
    } else {
        const bool generate_dither_map = result->use_dither_map && (attr->m_image->edges && 
            !attr->m_image->dither_map);
        if (generate_dither_map) {
            // If dithering (with dither map) is required, this image is used to find areas that require dithering
            remapping_error = remap_to_palette(attr->m_image, row_pointers, result->palette, 
                attr->m_remap->fast_palette);
            update_dither_map(row_pointers, attr->m_image);
        }

        // remapping above was the last chance to do voronoi iteration, hence the final palette is set after remapping
        set_rounded_palette(&result->int_palette, result->palette, result->gamma,
            attr->m_remap->min_posterization_output);

        remap_to_palette_floyd(attr->m_image, row_pointers, result->palette,
            MAX(remapping_error*2.4, 16.f/256.f), result->use_dither_map, generate_dither_map, result->dither_level);
    }

    // remapping error from dithered image is absurd, so always non-dithered value is used
    // palette_error includes some perceptual weighting from histogram which is closer correlated with dssim
    // so that should be used when possible.
    if (result->palette_error < 0) {
        result->palette_error = remapping_error;
    }

    return LIQ_OK;
}

/*
 Blurs image horizontally (width 2*size+1) and writes it transposed to dst (called twice gives 2d blur)
 */
static void transposing_1d_blur(unsigned char* restrict src, unsigned char* restrict dst, unsigned int width, unsigned int height, const unsigned int size)
{
    for (unsigned int j = 0; j < height; j++) {
        unsigned char* restrict row = src + j * width;

        // accumulate sum for pixels outside line
        unsigned int sum;
        sum = row[0] * size;
        for (unsigned int i = 0; i < size; i++) {
            sum += row[i];
        }

        // blur with left side outside line
        for (unsigned int i = 0; i < size; i++) {
            sum -= row[0];
            sum += row[i + size];

            dst[i * height + j] = sum / (size * 2);
        }

        for (unsigned int i = size; i < width - size; i++) {
            sum -= row[i - size];
            sum += row[i + size];

            dst[i * height + j] = sum / (size * 2);
        }

        // blur with right side outside line
        for (unsigned int i = width - size; i < width; i++) {
            sum -= row[i - size];
            sum += row[width - 1];

            dst[i * height + j] = sum / (size * 2);
        }
    }
}

/**
 * Picks maximum of neighboring pixels (blur + lighten)
 */
 void liq_max3(unsigned char* src, unsigned char* dst, unsigned int width, unsigned int height)
{
    for (unsigned int j = 0; j < height; j++) {
        const unsigned char* row = src + j * width,
            * prevrow = src + (j > 1 ? j - 1 : 0) * width,
            * nextrow = src + MIN(height - 1, j + 1) * width;

        unsigned char prev, curr = row[0], next = row[0];

        for (unsigned int i = 0; i < width - 1; i++) {
            prev = curr;
            curr = next;
            next = row[i + 1];

            unsigned char t1 = MAX(prev, next);
            unsigned char t2 = MAX(nextrow[i], prevrow[i]);
            *dst++ = MAX(curr, MAX(t1, t2));
        }
        unsigned char t1 = MAX(curr, next);
        unsigned char t2 = MAX(nextrow[width - 1], prevrow[width - 1]);
        *dst++ = MAX(t1, t2);
    }
}

/**
 * Picks minimum of neighboring pixels (blur + darken)
 */
 void liq_min3(unsigned char* src, unsigned char* dst, unsigned int width, unsigned int height)
{
    for (unsigned int j = 0; j < height; j++) {
        const unsigned char* row = src + j * width,
            * prevrow = src + (j > 1 ? j - 1 : 0) * width,
            * nextrow = src + MIN(height - 1, j + 1) * width;

        unsigned char prev, curr = row[0], next = row[0];

        for (unsigned int i = 0; i < width - 1; i++) {
            prev = curr;
            curr = next;
            next = row[i + 1];

            unsigned char t1 = MIN(prev, next);
            unsigned char t2 = MIN(nextrow[i], prevrow[i]);
            *dst++ = MIN(curr, MIN(t1, t2));
        }
        unsigned char t1 = MIN(curr, next);
        unsigned char t2 = MIN(nextrow[width - 1], prevrow[width - 1]);
        *dst++ = MIN(t1, t2);
    }
}

/*
 Filters src image and saves it to dst, overwriting tmp in the process.
 Image must be width*height pixels high. Size controls radius of box blur.
 */
 void liq_blur(unsigned char* src, unsigned char* tmp, unsigned char* dst, unsigned int width, unsigned int height, unsigned int size)
{
    assert(size > 0);
    if (width < 2 * size + 1 || height < 2 * size + 1) {
        return;
    }
    transposing_1d_blur(src, tmp, width, height, size);
    transposing_1d_blur(tmp, dst, height, width, size);
}

#define index_of_channel(ch) (offsetof(f_pixel,ch)/sizeof(float))

static f_pixel averagepixels(unsigned int clrs, const hist_item achv[], float min_opaque_val, const f_pixel center);

struct box {
    f_pixel color;
    f_pixel variance;
    double sum, total_error, max_error;
    unsigned int ind;
    unsigned int colors;
};

ALWAYS_INLINE static double variance_diff(double val, const double good_enough);
inline static double variance_diff(double val, const double good_enough)
{
    val *= val;
    if (val < good_enough * good_enough) return val * 0.25;
    return val;
}

/** Weighted per-channel variance of the box. It's used to decide which channel to split by */
static f_pixel box_variance(const hist_item achv[], const struct box* box)
{
    f_pixel mean = box->color;
    double variancea = 0, variancer = 0, varianceg = 0, varianceb = 0;

    for (unsigned int i = 0; i < box->colors; ++i) {
        f_pixel px = achv[box->ind + i].acolor;
        double weight = achv[box->ind + i].adjusted_weight;
        variancea += variance_diff(mean.a - px.a, 2.0 / 256.0) * weight;
        variancer += variance_diff(mean.r - px.r, 1.0 / 256.0) * weight;
        varianceg += variance_diff(mean.g - px.g, 1.0 / 256.0) * weight;
        varianceb += variance_diff(mean.b - px.b, 1.0 / 256.0) * weight;
    }

    return (f_pixel) {
        .a = variancea * (4.0 / 16.0),
            .r = variancer * (7.0 / 16.0),
            .g = varianceg * (9.0 / 16.0),
            .b = varianceb * (5.0 / 16.0),
    };
}

static double box_max_error(const hist_item achv[], const struct box* box)
{
    f_pixel mean = box->color;
    double max_error = 0;

    for (unsigned int i = 0; i < box->colors; ++i) {
        const double diff = colordifference(mean, achv[box->ind + i].acolor);
        if (diff > max_error) {
            max_error = diff;
        }
    }
    return max_error;
}

ALWAYS_INLINE static double color_weight(f_pixel median, hist_item h);

static inline void hist_item_swap(hist_item* l, hist_item* r)
{
    if (l != r) {
        hist_item t = *l;
        *l = *r;
        *r = t;
    }
}

ALWAYS_INLINE static unsigned int qsort_pivot(const hist_item* const base, const unsigned int len);
inline static unsigned int qsort_pivot(const hist_item* const base, const unsigned int len)
{
    if (len < 32) {
        return len / 2;
    }

    const unsigned int aidx = 8, bidx = len / 2, cidx = len - 1;
    const unsigned int a = base[aidx].tmp.sort_value, b = base[bidx].tmp.sort_value, c = base[cidx].tmp.sort_value;
    return (a < b) ? ((b < c) ? bidx : ((a < c) ? cidx : aidx))
        : ((b > c) ? bidx : ((a < c) ? aidx : cidx));
}

ALWAYS_INLINE static unsigned int qsort_partition(hist_item* const base, const unsigned int len);
inline static unsigned int qsort_partition(hist_item* const base, const unsigned int len)
{
    unsigned int l = 1, r = len;
    if (len >= 8) {
        hist_item_swap(&base[0], &base[qsort_pivot(base, len)]);
    }

    const unsigned int pivot_value = base[0].tmp.sort_value;
    while (l < r) {
        if (base[l].tmp.sort_value >= pivot_value) {
            l++;
        }
        else {
            while (l < --r && base[r].tmp.sort_value <= pivot_value) {}
            hist_item_swap(&base[l], &base[r]);
        }
    }
    l--;
    hist_item_swap(&base[0], &base[l]);

    return l;
}

/** quick select algorithm */
static void hist_item_sort_range(hist_item* base, unsigned int len, unsigned int sort_start)
{
    for (;;) {
        const unsigned int l = qsort_partition(base, len), r = l + 1;

        if (l > 0 && sort_start < l) {
            len = l;
        }
        else if (r < len && sort_start > r) {
            base += r; len -= r; sort_start -= r;
        }
        else break;
    }
}

/** sorts array to make sum of weights lower than halfvar one side, returns edge between <halfvar and >halfvar parts of the set */
static hist_item* hist_item_sort_halfvar(hist_item* base, unsigned int len, double* const lowervar, const double halfvar)
{
    do {
        const unsigned int l = qsort_partition(base, len), r = l + 1;

        // check if sum of left side is smaller than half,
        // if it is, then it doesn't need to be sorted
        unsigned int t = 0; double tmpsum = *lowervar;
        while (t <= l && tmpsum < halfvar) tmpsum += base[t++].color_weight;

        if (tmpsum < halfvar) {
            *lowervar = tmpsum;
        }
        else {
            if (l > 0) {
                hist_item* res = hist_item_sort_halfvar(base, l, lowervar, halfvar);
                if (res) return res;
            }
            else {
                // End of left recursion. This will be executed in order from the first element.
                *lowervar += base[0].color_weight;
                if (*lowervar > halfvar) return &base[0];
            }
        }

        if (len > r) {
            base += r; len -= r; // tail-recursive "call"
        }
        else {
            *lowervar += base[r].color_weight;
            return (*lowervar > halfvar) ? &base[r] : NULL;
        }
    } while (1);
}

static f_pixel get_median(const struct box* b, hist_item achv[]);

typedef struct {
    unsigned int chan; float variance;
} channelvariance;

static int comparevariance(const void* ch1, const void* ch2)
{
    return ((const channelvariance*)ch1)->variance > ((const channelvariance*)ch2)->variance ? -1 :
        (((const channelvariance*)ch1)->variance < ((const channelvariance*)ch2)->variance ? 1 : 0);
}

/** Finds which channels need to be sorted first and preproceses achv for fast sort */
static double prepare_sort(struct box* b, hist_item achv[])
{
    /*
     ** Sort dimensions by their variance, and then sort colors first by dimension with highest variance
     */
    channelvariance channels[4] = {
        {index_of_channel(r), b->variance.r},
        {index_of_channel(g), b->variance.g},
        {index_of_channel(b), b->variance.b},
        {index_of_channel(a), b->variance.a},
    };

    qsort(channels, 4, sizeof(channels[0]), comparevariance);

    for (unsigned int i = 0; i < b->colors; i++) {
        const float* chans = (const float*)&achv[b->ind + i].acolor;
        // Only the first channel really matters. When trying median cut many times
        // with different histogram weights, I don't want sort randomness to influence outcome.
        achv[b->ind + i].tmp.sort_value = ((unsigned int)(chans[channels[0].chan] * 65535.0) << 16) |
            (unsigned int)((chans[channels[2].chan] + chans[channels[1].chan] / 2.0 + chans[channels[3].chan] / 4.0) * 65535.0);
    }

    const f_pixel median = get_median(b, achv);

    // box will be split to make color_weight of each side even
    const unsigned int ind = b->ind, end = ind + b->colors;
    double totalvar = 0;
    for (unsigned int j = ind; j < end; j++) totalvar += (achv[j].color_weight = color_weight(median, achv[j]));
    return totalvar / 2.0;
}

/** finds median in unsorted set by sorting only minimum required */
static f_pixel get_median(const struct box* b, hist_item achv[])
{
    const unsigned int median_start = (b->colors - 1) / 2;

    hist_item_sort_range(&(achv[b->ind]), b->colors,
        median_start);

    if (b->colors & 1) return achv[b->ind + median_start].acolor;

    // technically the second color is not guaranteed to be sorted correctly
    // but most of the time it is good enough to be useful
    return averagepixels(2, &achv[b->ind + median_start], 1.0, (f_pixel) { 0.5, 0.5, 0.5, 0.5 });
}

/*
 ** Find the best splittable box. -1 if no boxes are splittable.
 */
static int best_splittable_box(struct box* bv, unsigned int boxes, const double max_mse)
{
    int bi = -1; double maxsum = 0;
    for (unsigned int i = 0; i < boxes; i++) {
        if (bv[i].colors < 2) {
            continue;
        }

        // looks only at max variance, because it's only going to split by it
        const double cv = MAX(bv[i].variance.r, MAX(bv[i].variance.g, bv[i].variance.b));
        double thissum = bv[i].sum * MAX(bv[i].variance.a, cv);

        if (bv[i].max_error > max_mse) {
            thissum = thissum * bv[i].max_error / max_mse;
        }

        if (thissum > maxsum) {
            maxsum = thissum;
            bi = i;
        }
    }
    return bi;
}

inline static double color_weight(f_pixel median, hist_item h)
{
    float diff = colordifference(median, h.acolor);
    // if color is "good enough", don't split further
    if (diff < 2.f / 256.f / 256.f) diff /= 2.f;
    return sqrt(diff) * (sqrt(1.0 + h.adjusted_weight) - 1.0);
}

static void set_colormap_from_boxes(colormap* map, struct box* bv, unsigned int boxes, hist_item* achv);
static void adjust_histogram(hist_item* achv, const colormap* map, const struct box* bv, unsigned int boxes);

double box_error(const struct box* box, const hist_item achv[])
{
    f_pixel avg = box->color;

    double total_error = 0;
    for (unsigned int i = 0; i < box->colors; ++i) {
        total_error += colordifference(avg, achv[box->ind + i].acolor) * achv[box->ind + i].perceptual_weight;
    }

    return total_error;
}


static bool total_box_error_below_target(double target_mse, struct box bv[], unsigned int boxes, const histogram* hist)
{
    target_mse *= hist->total_perceptual_weight;
    double total_error = 0;

    for (unsigned int i = 0; i < boxes; i++) {
        // error is (re)calculated lazily
        if (bv[i].total_error >= 0) {
            total_error += bv[i].total_error;
        }
        if (total_error > target_mse) return false;
    }

    for (unsigned int i = 0; i < boxes; i++) {
        if (bv[i].total_error < 0) {
            bv[i].total_error = box_error(&bv[i], hist->achv);
            total_error += bv[i].total_error;
        }
        if (total_error > target_mse) return false;
    }

    return true;
}

/*
 ** Here is the fun part, the median-cut colormap generator.  This is based
 ** on Paul Heckbert's paper, "Color Image Quantization for Frame Buffer
 ** Display," SIGGRAPH 1982 Proceedings, page 297.
 */
 colormap* mediancut(histogram* hist, const float min_opaque_val, unsigned int newcolors,
     const double target_mse, const double max_mse)
{
    hist_item* achv = hist->achv;
    struct box* bv = _alloca(newcolors * sizeof(struct box));

    /*
     ** Set up the initial box.
     */
    bv[0].ind = 0;
    bv[0].colors = hist->size;
    bv[0].color = averagepixels(bv[0].colors, &achv[bv[0].ind], min_opaque_val, (f_pixel) { 0.5, 0.5, 0.5, 0.5 });
    bv[0].variance = box_variance(achv, &bv[0]);
    bv[0].max_error = box_max_error(achv, &bv[0]);
    bv[0].sum = 0;
    bv[0].total_error = -1;
    for (unsigned int i = 0; i < bv[0].colors; i++) bv[0].sum += achv[i].adjusted_weight;

    unsigned int boxes = 1;

    // remember smaller palette for fast searching
    colormap* representative_subset = NULL;
    unsigned int subset_size = ceilf(powf(newcolors, 0.7f));

    /*
     ** Main loop: split boxes until we have enough.
     */
    while (boxes < newcolors) {

        if (boxes == subset_size) {
            representative_subset = pam_colormap(boxes);
            set_colormap_from_boxes(representative_subset, bv, boxes, achv);
        }

        // first splits boxes that exceed quality limit (to have colors for things like odd green pixel),
        // later raises the limit to allow large smooth areas/gradients get colors.
        const double current_max_mse = max_mse + (boxes / (double)newcolors) * 16.0 * max_mse;
        const int bi = best_splittable_box(bv, boxes, current_max_mse);
        if (bi < 0)
            break;        /* ran out of colors! */

        unsigned int indx = bv[bi].ind;
        unsigned int clrs = bv[bi].colors;

        /*
         Classic implementation tries to get even number of colors or pixels in each subdivision.

         Here, instead of popularity I use (sqrt(popularity)*variance) metric.
         Each subdivision balances number of pixels (popular colors) and low variance -
         boxes can be large if they have similar colors. Later boxes with high variance
         will be more likely to be split.

         Median used as expected value gives much better results than mean.
         */

        const double halfvar = prepare_sort(&bv[bi], achv);
        double lowervar = 0;

        // hist_item_sort_halfvar sorts and sums lowervar at the same time
        // returns item to break at …minus one, which does smell like an off-by-one error.
        hist_item* break_p = hist_item_sort_halfvar(&achv[indx], clrs, &lowervar, halfvar);
        unsigned int break_at = MIN(clrs - 1, break_p - &achv[indx] + 1);

        /*
         ** Split the box.
         */
        double sm = bv[bi].sum;
        double lowersum = 0;
        for (unsigned int i = 0; i < break_at; i++) lowersum += achv[indx + i].adjusted_weight;

        const f_pixel previous_center = bv[bi].color;
        bv[bi].colors = break_at;
        bv[bi].sum = lowersum;
        bv[bi].color = averagepixels(bv[bi].colors, &achv[bv[bi].ind], min_opaque_val, previous_center);
        bv[bi].total_error = -1;
        bv[bi].variance = box_variance(achv, &bv[bi]);
        bv[bi].max_error = box_max_error(achv, &bv[bi]);
        bv[boxes].ind = indx + break_at;
        bv[boxes].colors = clrs - break_at;
        bv[boxes].sum = sm - lowersum;
        bv[boxes].color = averagepixels(bv[boxes].colors, &achv[bv[boxes].ind], min_opaque_val, previous_center);
        bv[boxes].total_error = -1;
        bv[boxes].variance = box_variance(achv, &bv[boxes]);
        bv[boxes].max_error = box_max_error(achv, &bv[boxes]);

        ++boxes;

        if (total_box_error_below_target(target_mse, bv, boxes, hist)) {
            break;
        }
    }

    colormap* map = pam_colormap(boxes);
    set_colormap_from_boxes(map, bv, boxes, achv);

    map->subset_palette = representative_subset;
    adjust_histogram(achv, map, bv, boxes);

    return map;
}

static void set_colormap_from_boxes(colormap* map, struct box* bv, unsigned int boxes, hist_item* achv)
{
    /*
     ** Ok, we've got enough boxes.  Now choose a representative color for
     ** each box.  There are a number of possible ways to make this choice.
     ** One would be to choose the center of the box; this ignores any structure
     ** within the boxes.  Another method would be to average all the colors in
     ** the box - this is the method specified in Heckbert's paper.
     */

    for (unsigned int bi = 0; bi < boxes; ++bi) {
        map->palette[bi].acolor = bv[bi].color;

        /* store total color popularity (perceptual_weight is approximation of it) */
        map->palette[bi].popularity = 0;
        for (unsigned int i = bv[bi].ind; i < bv[bi].ind + bv[bi].colors; i++) {
            map->palette[bi].popularity += achv[i].perceptual_weight;
        }
    }
}

/* increase histogram popularity by difference from the final color (this is used as part of feedback loop) */
static void adjust_histogram(hist_item* achv, const colormap* map, const struct box* bv, unsigned int boxes)
{
    for (unsigned int bi = 0; bi < boxes; ++bi) {
        for (unsigned int i = bv[bi].ind; i < bv[bi].ind + bv[bi].colors; i++) {
            achv[i].adjusted_weight *= sqrt(1.0 + colordifference(map->palette[bi].acolor, achv[i].acolor) / 4.0);
            achv[i].tmp.likely_colormap_index = bi;
        }
    }
}

static f_pixel averagepixels(unsigned int clrs, const hist_item achv[], const float min_opaque_val, const f_pixel center)
{
    double r = 0, g = 0, b = 0, a = 0, new_a = 0, sum = 0;
    float maxa = 0;

    // first find final opacity in order to blend colors at that opacity
    for (unsigned int i = 0; i < clrs; ++i) {
        const f_pixel px = achv[i].acolor;
        new_a += px.a * achv[i].adjusted_weight;
        sum += achv[i].adjusted_weight;

        /* find if there are opaque colors, in case we're supposed to preserve opacity exactly (ie_bug) */
        if (px.a > maxa) maxa = px.a;
    }

    if (sum) new_a /= sum;

    /** if there was at least one completely opaque color, "round" final color to opaque */
    if (new_a >= min_opaque_val && maxa >= (255.0 / 256.0)) new_a = 1;

    sum = 0;
    // reverse iteration for cache locality with previous loop
    for (int i = clrs - 1; i >= 0; i--) {
        double tmp, weight = 1.0f;
        f_pixel px = achv[i].acolor;

        /* give more weight to colors that are further away from average
         this is intended to prevent desaturation of images and fading of whites
         */
        tmp = (center.r - px.r);
        weight += tmp * tmp;
        tmp = (center.g - px.g);
        weight += tmp * tmp;
        tmp = (center.b - px.b);
        weight += tmp * tmp;

        weight *= achv[i].adjusted_weight;
        sum += weight;

        if (px.a) {
            px.r /= px.a;
            px.g /= px.a;
            px.b /= px.a;
        }

        r += px.r * new_a * weight;
        g += px.g * new_a * weight;
        b += px.b * new_a * weight;
        a += new_a * weight;
    }

    if (sum) {
        a /= sum;
        r /= sum;
        g /= sum;
        b /= sum;
    }

    assert(!isnan(r) && !isnan(g) && !isnan(b) && !isnan(a));

    return (f_pixel) { .r = r, .g = g, .b = b, .a = a };
}

struct mempool;
typedef struct mempool* mempool;

 void* mempool_create(mempool* mptr, unsigned int size, unsigned int capacity);
 void* mempool_alloc(mempool* mptr, unsigned int size, unsigned int capacity);
 void mempool_destroy(mempool m);


#define ALIGN_MASK 15UL
#define MEMPOOL_RESERVED ((sizeof(struct mempool)+ALIGN_MASK) & ~ALIGN_MASK)

struct mempool {
    unsigned int used, size;
    struct mempool* next;
};
 void* mempool_create(mempool* mptr, const unsigned int size, unsigned int max_size)
{
    if (*mptr && ((*mptr)->used + size) <= (*mptr)->size) {
        unsigned int prevused = (*mptr)->used;
        (*mptr)->used += (size + 15UL) & ~0xFUL;
        return ((char*)(*mptr)) + prevused;
    }

    mempool old = *mptr;
    if (!max_size) max_size = (1 << 17);
    max_size = size + ALIGN_MASK > max_size ? size + ALIGN_MASK : max_size;

    *mptr = av_malloc(MEMPOOL_RESERVED + max_size);
    if (!*mptr) return NULL;
    **mptr = (struct mempool){
        .size = MEMPOOL_RESERVED + max_size,
        .used = sizeof(struct mempool),
        .next = old,
    };
    uintptr_t mptr_used_start = (uintptr_t)(*mptr) + (*mptr)->used;
    (*mptr)->used += (ALIGN_MASK + 1 - (mptr_used_start & ALIGN_MASK)) & ALIGN_MASK; // reserve bytes required to make subsequent allocations aligned
    assert(!(((uintptr_t)(*mptr) + (*mptr)->used) & ALIGN_MASK));

    return mempool_alloc(mptr, size, size);
}

 void* mempool_alloc(mempool* mptr, unsigned int size, unsigned int max_size)
{
    if (((*mptr)->used + size) <= (*mptr)->size) {
        unsigned int prevused = (*mptr)->used;
        (*mptr)->used += (size + ALIGN_MASK) & ~ALIGN_MASK;
        return ((char*)(*mptr)) + prevused;
    }

    return mempool_create(mptr, size, max_size);
}

 void mempool_destroy(mempool m)
{
    while (m) {
        mempool next = m->next;
        av_free(m);
        m = next;
    }
}

struct sorttmp {
    float radius;
    unsigned int index;
};

struct head {
    // colors less than radius away from vantage_point color will have best match in candidates
    f_pixel vantage_point;
    float radius;
    unsigned int num_candidates;
    f_pixel* candidates_color;
    unsigned short* candidates_index;
};

struct nearest_map {
    const colormap* map;
    float nearest_other_color_dist[256];
    mempool mempool;
    struct head heads[];
};

static unsigned int find_slow(const f_pixel px, const colormap* map)
{
    unsigned int best = 0;
    float bestdiff = colordifference(px, map->palette[0].acolor);

    for (unsigned int i = 1; i < map->colors; i++) {
        float diff = colordifference(px, map->palette[i].acolor);
        if (diff < bestdiff) {
            best = i;
            bestdiff = diff;
        }
    }
    return best;
}

static float distance_from_nearest_other_color(const colormap* map, const unsigned int i)
{
    float second_best = MAX_DIFF;
    for (unsigned int j = 0; j < map->colors; j++) {
        if (i == j) continue;
        float diff = colordifference(map->palette[i].acolor, map->palette[j].acolor);
        if (diff <= second_best) {
            second_best = diff;
        }
    }
    return second_best;
}

static int compareradius(const void* ap, const void* bp)
{
    float a = ((const struct sorttmp*)ap)->radius;
    float b = ((const struct sorttmp*)bp)->radius;
    return a > b ? 1 : (a < b ? -1 : 0);
}

static struct head build_head(f_pixel px, const colormap* map, unsigned int num_candidates, mempool* m, float error_margin, bool skip_index[], unsigned int* skipped)
{
    //map->colors 256
    struct sorttmp colors[256];
    unsigned int colorsused = 0;

    for (unsigned int i = 0; i < map->colors; i++) {
        if (skip_index[i]) continue; // colors in skip_index have been eliminated already in previous heads
        colors[colorsused].index = i;
        colors[colorsused].radius = colordifference(px, map->palette[i].acolor);
        colorsused++;
    }

    qsort(&colors, colorsused, sizeof(colors[0]), compareradius);
    assert(colorsused < 2 || colors[0].radius <= colors[1].radius); // closest first

    num_candidates = MIN(colorsused, num_candidates);

    struct head h = {
        .candidates_color = mempool_alloc(m, num_candidates * sizeof(h.candidates_color[0]), 0),
        .candidates_index = mempool_alloc(m, num_candidates * sizeof(h.candidates_index[0]), 0),
        .vantage_point = px,
        .num_candidates = num_candidates,
    };
    for (unsigned int i = 0; i < num_candidates; i++) {
        h.candidates_color[i] = map->palette[colors[i].index].acolor;
        h.candidates_index[i] = colors[i].index;
    }
    // if all colors within this radius are included in candidates, then there cannot be any other better match
    // farther away from the vantage point than half of the radius. Due to alpha channel must assume pessimistic radius.
    h.radius = min_colordifference(px, h.candidates_color[num_candidates - 1]) / 4.0f; // /4 = half of radius, but radius is squared

    for (unsigned int i = 0; i < num_candidates; i++) {
        // divide again as that's matching certain subset within radius-limited subset
        // - 1/256 is a tolerance for miscalculation (seems like colordifference isn't exact)
        if (colors[i].radius < h.radius / 4.f - error_margin) {
            skip_index[colors[i].index] = true;
            (*skipped)++;
        }
    }
    return h;
}

static colormap* get_subset_palette(const colormap* map)
{
    if (map->subset_palette) {
        return map->subset_palette;
    }

    unsigned int subset_size = (map->colors + 3) / 4;
    colormap* subset_palette = pam_colormap(subset_size);

    for (unsigned int i = 0; i < subset_size; i++) {
        subset_palette->palette[i] = map->palette[i];
    }

    return subset_palette;
}

 struct nearest_map* nearest_init(const colormap* map, bool fast)
{
    colormap* subset_palette = get_subset_palette(map);
    const unsigned int num_vantage_points = map->colors > 16 ? MIN(map->colors / 4, subset_palette->colors) : 0;
    const unsigned long heads_size = sizeof(struct head) * (num_vantage_points + 1); // +1 is fallback head

    const unsigned long mempool_size = (sizeof(f_pixel) + sizeof(unsigned int)) * subset_palette->colors * map->colors / 5 + (1 << 14);
    mempool m = NULL;
    struct nearest_map* centroids = mempool_create(&m, sizeof(*centroids) + heads_size /* heads array is appended to it */, mempool_size);
    centroids->mempool = m;

    for (unsigned int i = 0; i < map->colors; i++) {
        const float dist = distance_from_nearest_other_color(map, i);
        centroids->nearest_other_color_dist[i] = dist / 4.f; // half of squared distance
    }

    centroids->map = map;

    unsigned int skipped = 0;
    assert(map->colors > 0);
    bool* skip_index = _alloca(map->colors * sizeof(bool));
    for (unsigned int j = 0; j < map->colors; j++)
        skip_index[j] = false;

    // floats and colordifference calculations are not perfect
    const float error_margin = fast ? 0 : 8.f / 256.f / 256.f;
    unsigned int h = 0;
    for (; h < num_vantage_points; h++) {
        unsigned int num_candiadtes = 1 + (map->colors - skipped) / ((1 + num_vantage_points - h) / 2);

        centroids->heads[h] = build_head(subset_palette->palette[h].acolor, map, num_candiadtes, &centroids->mempool, error_margin, skip_index, &skipped);
        if (centroids->heads[h].num_candidates == 0) {
            break;
        }
    }

    // assumption that there is no better color within radius of vantage point color
    // holds true only for colors within convex hull formed by palette colors.
    // since finding proper convex hull is more than a few lines, this
    // is a cheap shot at finding just few key points.
    const f_pixel extrema[] = {
        {.a = 0,0,0,0},

        {.a = .5,0,0,0}, {.a = .5,1,0,0},
        {.a = .5,0,0,1}, {.a = .5,1,0,1},
        {.a = .5,0,1,0}, {.a = .5,1,1,0},
        {.a = .5,0,1,1}, {.a = .5,1,1,1},

        {.a = 1,0,0,0}, {.a = 1,1,0,0},
        {.a = 1,0,0,1}, {.a = 1,1,0,1},
        {.a = 1,0,1,0}, {.a = 1,1,1,0},
        {.a = 1,0,1,1}, {.a = 1,1,1,1},

        {.a = 1,.5, 0, 0}, {.a = 1, 0,.5, 0}, {.a = 1, 0, 0, .5},
        {.a = 1,.5, 0, 1}, {.a = 1, 0,.5, 1}, {.a = 1, 0, 1, .5},
        {.a = 1,.5, 1, 0}, {.a = 1, 1,.5, 0}, {.a = 1, 1, 0, .5},
        {.a = 1,.5, 1, 1}, {.a = 1, 1,.5, 1}, {.a = 1, 1, 1, .5},
    };
    for (unsigned int i = 0; i < sizeof(extrema) / sizeof(extrema[0]); i++) {
        skip_index[find_slow(extrema[i], map)] = 0;
    }

    centroids->heads[h] = build_head((f_pixel) { 0, 0, 0, 0 }, map, map->colors, & centroids->mempool, error_margin, skip_index, & skipped);
    centroids->heads[h].radius = MAX_DIFF;

    // get_subset_palette could have created a copy
    if (subset_palette != map->subset_palette) {
        pam_freecolormap(subset_palette);
    }

    return centroids;
}

 unsigned int nearest_search(const struct nearest_map* centroids, const f_pixel px, int likely_colormap_index, const float min_opaque_val, float* diff)
{
    const bool iebug = px.a > min_opaque_val;

    const struct head* const heads = centroids->heads;

    assert(likely_colormap_index < centroids->map->colors);
    const float guess_diff = colordifference(centroids->map->palette[likely_colormap_index].acolor, px);
    if (guess_diff < centroids->nearest_other_color_dist[likely_colormap_index]) {
        if (diff) *diff = guess_diff;
        return likely_colormap_index;
    }

    for (unsigned int i = 0; /* last head will always be selected */; i++) {
        float vantage_point_dist = colordifference(px, heads[i].vantage_point);

        if (vantage_point_dist <= heads[i].radius) {
            assert(heads[i].num_candidates);
            unsigned int ind = 0;
            float dist = colordifference(px, heads[i].candidates_color[0]);

            /* penalty for making holes in IE */
            if (iebug && heads[i].candidates_color[0].a < 1) {
                dist += 1.f / 1024.f;
            }

            for (unsigned int j = 1; j < heads[i].num_candidates; j++) {
                float newdist = colordifference(px, heads[i].candidates_color[j]);

                /* penalty for making holes in IE */
                if (iebug && heads[i].candidates_color[j].a < 1) {
                    newdist += 1.f / 1024.f;
                }

                if (newdist < dist) {
                    dist = newdist;
                    ind = j;
                }
            }
            if (diff) *diff = dist;
            return heads[i].candidates_index[ind];
        }
    }
}

 void nearest_deinit(struct nearest_map* centroids)
{
    mempool_destroy(centroids->mempool);
}

 bool pam_computeacolorhash(struct acolorhash_table* acht, const rgba_pixel* const pixels[], unsigned int cols, unsigned int rows, const unsigned char* importance_map)
{
    const unsigned int maxacolors = acht->maxcolors, ignorebits = acht->ignorebits;
    const unsigned int channel_mask = 255U >> ignorebits << ignorebits;
    const unsigned int channel_hmask = (255U >> ignorebits) ^ 0xFFU;
    const unsigned int posterize_mask = channel_mask << 24 | channel_mask << 16 | channel_mask << 8 | channel_mask;
    const unsigned int posterize_high_mask = channel_hmask << 24 | channel_hmask << 16 | channel_hmask << 8 | channel_hmask;
    struct acolorhist_arr_head* const buckets = acht->buckets;

    unsigned int colors = acht->colors;
    const unsigned int hash_size = acht->hash_size;

    const unsigned int stacksize = sizeof(acht->freestack) / sizeof(acht->freestack[0]);
    struct acolorhist_arr_item** freestack = acht->freestack;
    unsigned int freestackp = acht->freestackp;

    /* Go through the entire image, building a hash table of colors. */
    for (unsigned int row = 0; row < rows; ++row) {

        float boost = 1.0;
        for (unsigned int col = 0; col < cols; ++col) {
            if (importance_map) {
                boost = 0.5f + (double)*importance_map++ / 255.f;
            }

            // RGBA color is casted to long for easier hasing/comparisons
            union rgba_as_int px = { pixels[row][col] };
            unsigned int hash;
            if (!px.rgba.a) {
                // "dirty alpha" has different RGBA values that end up being the same fully transparent color
                px.l = 0; hash = 0;
            }
            else {
                // mask posterizes all 4 channels in one go
                px.l = (px.l & posterize_mask) | ((px.l & posterize_high_mask) >> (8 - ignorebits));
                // fancier hashing algorithms didn't improve much
                hash = px.l % hash_size;
            }

            /* head of the hash function stores first 2 colors inline (achl->used = 1..2),
               to reduce number of allocations of achl->other_items.
             */
            struct acolorhist_arr_head* achl = &buckets[hash];
            if (achl->inline1.color.l == px.l && achl->used) {
                achl->inline1.perceptual_weight += boost;
                continue;
            }
            if (achl->used) {
                if (achl->used > 1) {
                    if (achl->inline2.color.l == px.l) {
                        achl->inline2.perceptual_weight += boost;
                        continue;
                    }
                    // other items are stored as an array (which gets reallocated if needed)
                    struct acolorhist_arr_item* other_items = achl->other_items;
                    unsigned int i = 0;
                    for (; i < achl->used - 2; i++) {
                        if (other_items[i].color.l == px.l) {
                            other_items[i].perceptual_weight += boost;
                            goto continue_outer_loop;
                        }
                    }

                    // the array was allocated with spare items
                    if (i < achl->capacity) {
                        other_items[i] = (struct acolorhist_arr_item){
                            .color = px,
                            .perceptual_weight = boost,
                        };
                        achl->used++;
                        ++colors;
                        continue;
                    }

                    if (++colors > maxacolors) {
                        acht->colors = colors;
                        acht->freestackp = freestackp;
                        return false;
                    }

                    struct acolorhist_arr_item* new_items;
                    unsigned int capacity;
                    if (!other_items) { // there was no array previously, alloc "small" array
                        capacity = 8;
                        if (freestackp <= 0) {
                            // estimate how many colors are going to be + headroom
                            const int mempool_size = ((acht->rows + rows - row) * 2 * colors / (acht->rows + row + 1) + 1024) * sizeof(struct acolorhist_arr_item);
                            new_items = mempool_alloc(&acht->mempool, sizeof(struct acolorhist_arr_item) * capacity, mempool_size);
                        }
                        else {
                            // freestack stores previously freed (reallocated) arrays that can be reused
                            // (all pesimistically assumed to be capacity = 8)
                            new_items = freestack[--freestackp];
                        }
                    }
                    else {
                        // simply reallocs and copies array to larger capacity
                        capacity = achl->capacity * 2 + 16;
                        if (freestackp < stacksize - 1) {
                            freestack[freestackp++] = other_items;
                        }
                        const int mempool_size = ((acht->rows + rows - row) * 2 * colors / (acht->rows + row + 1) + 32 * capacity) * sizeof(struct acolorhist_arr_item);
                        new_items = mempool_alloc(&acht->mempool, sizeof(struct acolorhist_arr_item) * capacity, mempool_size);
                        if (!new_items) return false;
                        memcpy(new_items, other_items, sizeof(other_items[0]) * achl->capacity);
                    }

                    achl->other_items = new_items;
                    achl->capacity = capacity;
                    new_items[i] = (struct acolorhist_arr_item){
                        .color = px,
                        .perceptual_weight = boost,
                    };
                    achl->used++;
                }
                else {
                    // these are elses for first checks whether first and second inline-stored colors are used
                    achl->inline2.color.l = px.l;
                    achl->inline2.perceptual_weight = boost;
                    achl->used = 2;
                    ++colors;
                }
            }
            else {
                achl->inline1.color.l = px.l;
                achl->inline1.perceptual_weight = boost;
                achl->used = 1;
                ++colors;
            }

        continue_outer_loop:;
        }

    }
    acht->colors = colors;
    acht->cols = cols;
    acht->rows += rows;
    acht->freestackp = freestackp;
    return true;
}

 struct acolorhash_table* pam_allocacolorhash(unsigned int maxcolors, 
     unsigned int surface, unsigned int ignorebits)
{
    const unsigned int estimated_colors = MIN(maxcolors, surface / (ignorebits + (surface > 512 * 512 ? 5 : 4)));
    const unsigned int hash_size = estimated_colors < 66000 ? 6673 : (estimated_colors < 200000 ? 12011 : 24019);

    mempool m = NULL;
    const unsigned int buckets_size = hash_size * sizeof(struct acolorhist_arr_head);
    const unsigned int mempool_size = sizeof(struct acolorhash_table) + buckets_size + estimated_colors * sizeof(struct acolorhist_arr_item);
    struct acolorhash_table* t = mempool_create(&m, sizeof(*t) + buckets_size, mempool_size);
    if (!t) return NULL;
    *t = (struct acolorhash_table){
        .mempool = m,
        .hash_size = hash_size,
        .maxcolors = maxcolors,
        .ignorebits = ignorebits,
    };
    memset(t->buckets, 0, hash_size * sizeof(struct acolorhist_arr_head));
    return t;
}

#define PAM_ADD_TO_HIST(entry) { \
    hist->achv[j].acolor = to_f(s_gamma_lut, entry.color.rgba); \
    total_weight += hist->achv[j].adjusted_weight = hist->achv[j].perceptual_weight = MIN(entry.perceptual_weight, max_perceptual_weight); \
    ++j; \
}

 histogram* pam_acolorhashtoacolorhist(const struct acolorhash_table* acht,
     const double gamma)
{
    histogram* hist = av_malloc(sizeof(hist[0]));
    if (!hist || !acht) return NULL;
    *hist = (histogram){
        .achv = av_malloc(acht->colors * sizeof(hist->achv[0])),
        .size = acht->colors,
        .ignorebits = acht->ignorebits,
    };
    if (!hist->achv) return NULL;

    to_f_set_gamma(gamma);

    /* Limit perceptual weight to 1/10th of the image surface area to prevent
       a single color from dominating all others. */
    float max_perceptual_weight = 0.1f * acht->cols * acht->rows;
    double total_weight = 0;

    for (unsigned int j = 0, i = 0; i < acht->hash_size; ++i) {
        const struct acolorhist_arr_head* const achl = &acht->buckets[i];
        if (achl->used) {
            PAM_ADD_TO_HIST(achl->inline1);

            if (achl->used > 1) {
                PAM_ADD_TO_HIST(achl->inline2);

                for (unsigned int k = 0; k < achl->used - 2; k++) {
                    PAM_ADD_TO_HIST(achl->other_items[k]);
                }
            }
        }
    }

    hist->total_perceptual_weight = total_weight;
    return hist;
}


 void pam_freeacolorhash(struct acolorhash_table* acht)
{
    mempool_destroy(acht->mempool);
}

 void pam_freeacolorhist(histogram* hist)
{
    av_free(hist->achv);
    av_free(hist);
}

 colormap* pam_colormap(unsigned int colors)
{
    assert(colors > 0 && colors < 65536);

    colormap* map;
    const size_t colors_size = colors * sizeof(map->palette[0]);
    map = av_malloc(sizeof(colormap) + colors_size);
    if (!map) return NULL;
    *map = (colormap){
        .subset_palette = NULL,
        .colors = colors,
    };
    memset(map->palette, 0, colors_size);
    return map;
}

 colormap* pam_duplicate_colormap(colormap* map)
{
    colormap* dupe = pam_colormap(map->colors);
    for (unsigned int i = 0; i < map->colors; i++) {
        dupe->palette[i] = map->palette[i];
    }
    if (map->subset_palette) {
        dupe->subset_palette = pam_duplicate_colormap(map->subset_palette);
    }
    return dupe;
}

 void pam_freecolormap(colormap* c)
{
    if (c->subset_palette) pam_freecolormap(c->subset_palette);
    av_free(c);
}

 void to_f_set_gamma(const double gamma)
{
     if (s_gamma_lut_init == 0) {
         s_gamma_lut_init = 1;
         for (int i = 0; i < 256; i++) {
             s_gamma_lut[i] = pow((double)i / 255.0, internal_gamma / gamma);
         }
     }
}

/*
 * Voronoi iteration: new palette color is computed from weighted average of colors that map to that palette entry.
 */
 void viter_init(const colormap* map, const unsigned int max_threads, viter_state average_color[])
{
    memset(average_color, 0, sizeof(average_color[0]) * (VITER_CACHE_LINE_GAP + map->colors) * max_threads);
}

 void viter_update_color(const f_pixel acolor, const float value, const colormap* map, unsigned int match, const unsigned int thread, viter_state average_color[])
{
    match += thread * (VITER_CACHE_LINE_GAP + map->colors);
    average_color[match].a += acolor.a * value;
    average_color[match].r += acolor.r * value;
    average_color[match].g += acolor.g * value;
    average_color[match].b += acolor.b * value;
    average_color[match].total += value;
}

 void viter_finalize(colormap* map, const unsigned int max_threads, const viter_state average_color[])
{
    for (unsigned int i = 0; i < map->colors; i++) {
        double a = 0, r = 0, g = 0, b = 0, total = 0;

        // Aggregate results from all threads
        for (unsigned int t = 0; t < max_threads; t++) {
            const unsigned int offset = (VITER_CACHE_LINE_GAP + map->colors) * t + i;

            a += average_color[offset].a;
            r += average_color[offset].r;
            g += average_color[offset].g;
            b += average_color[offset].b;
            total += average_color[offset].total;
        }

        if (total) {
            map->palette[i].acolor = (f_pixel){
                .a = a / total,
                .r = r / total,
                .g = g / total,
                .b = b / total,
            };
        }
        map->palette[i].popularity = total;
    }
}

 double viter_do_iteration(histogram* hist, colormap* const map, const float min_opaque_val, viter_callback callback, const bool fast_palette)
{
    const unsigned int max_threads = omp_get_max_threads();
    viter_state* average_color = _alloca((VITER_CACHE_LINE_GAP + map->colors) * max_threads * sizeof(viter_state));
    viter_init(map, max_threads, average_color);
    struct nearest_map* const n = nearest_init(map, fast_palette);
    hist_item* const achv = hist->achv;
    const int hist_size = hist->size;

    double total_diff = 0;

    int j;
#pragma omp parallel for if (hist_size > 3000) schedule(static) default(none) shared(average_color,callback) reduction(+:total_diff)
    for (j = 0; j < hist_size; j++) {
        float diff;
        unsigned int match = nearest_search(n, achv[j].acolor, achv[j].tmp.likely_colormap_index, min_opaque_val, &diff);
        achv[j].tmp.likely_colormap_index = match;
        total_diff += diff * achv[j].perceptual_weight;

        viter_update_color(achv[j].acolor, achv[j].perceptual_weight, map, match, omp_get_thread_num(), average_color);

        if (callback) callback(&achv[j], diff);
    }

    nearest_deinit(n);
    viter_finalize(map, max_threads, average_color);

    return total_diff / hist->total_perceptual_weight;
}


