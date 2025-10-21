#include <math.h>
#include "frei0r.h"
#include <stdlib.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "frei0r.h"

#define SIZE_RGBA 4

static inline int MAX(int a, int b)
{
    return (a > b ? a : b);
}

static inline int MIN(int a, int b)
{
    return (a < b ? a : b);
}

static inline void subtract_acc(uint32_t* dst, const uint32_t* src)
{
    int n = SIZE_RGBA;
    while (n--)
        *dst++ -= *src++;
}

static inline void add_acc(uint32_t* dst, const uint32_t* src)
{
    int n = SIZE_RGBA;
    while (n--)
        *dst++ += *src++;
}

static inline void divide(uint8_t* dst, const uint32_t* src, const unsigned int val)
{
    int n = SIZE_RGBA;
    while (n--)
        *dst++ = *src++ / val;
}

typedef struct squareblur_instance
{
    unsigned int width;
    unsigned int height;
    double kernel; /* the kernel size, as a percentage of the biggest of width and height */
    uint32_t* mem; /* memory accumulation matrix of uint32_t (size = acc_width*acc_height*SIZE_RGBA) */
    uint32_t** acc; /* accumulation matrix of pointers to SIZE_RGBA consecutive uint32_t in mem (size = acc_width*acc_height) */
} squareblur_instance_t;

/* Updates the summed area table. */
static void update_summed_area_table(squareblur_instance_t* inst, const uint32_t* src)
{
    register uint8_t* iter_data;
    register uint32_t* iter_mem;
    register unsigned int i, x, y;

    uint32_t acc_buffer[SIZE_RGBA]; /* accumulation buffer */

    unsigned int row_width;
    unsigned int width, height;
    unsigned int cell_size;

    /* Compute basic params. */
    width = inst->width + 1;
    height = inst->height + 1;
    row_width = SIZE_RGBA * width;
    cell_size = SIZE_RGBA * sizeof(uint32_t);

    /* Init iterators. */
    iter_data = (uint8_t*)src;
    iter_mem = inst->mem;

    /* Process first row (all zeros). */
    memset(iter_mem, 0, row_width * cell_size);
    iter_mem += row_width;

    if (height >= 1)
    {
        /* Process second row. */
        memset(acc_buffer, 0, cell_size);
        memset(iter_mem, 0, cell_size); /* first column is void */
        iter_mem += SIZE_RGBA;
        for (x = 1; x < width; ++x)
        {
            for (i = 0; i < SIZE_RGBA; ++i)
                *iter_mem++ = (acc_buffer[i] += *iter_data++);
        }

        /* Process other rows. */
        for (y = 2; y < height; ++y)
        {
            /* Copy upper line. */
            memcpy(iter_mem, iter_mem - row_width, row_width * sizeof(uint32_t));

            /* Process row. */
            memset(acc_buffer, 0, cell_size);
            memset(iter_mem, 0, cell_size); /* first column is void */
            iter_mem += SIZE_RGBA;
            for (x = 1; x < width; ++x)
            {
                for (i = 0; i < SIZE_RGBA; ++i)
                    *iter_mem++ += (acc_buffer[i] += *iter_data++);
            }
        }
    }
}

static void blur_get_param_info(f0r_param_info_t* info, int param_index)
{
    switch (param_index)
    {
    case 0:
        info->name = "Kernel size";
        info->type = F0R_PARAM_DOUBLE;
        info->explanation = "The size of the kernel, as a proportion to its coverage of the image";
        break;
    }
}

static f0r_instance_t blur_construct(unsigned int width, unsigned int height)
{
    squareblur_instance_t* inst =
        (squareblur_instance_t*)malloc(sizeof(squareblur_instance_t));
    unsigned int i;
    unsigned int acc_width, acc_height = height + 1;
    uint32_t* iter_mem;
    uint32_t** iter_acc;
    /* set params */
    inst->width = width; inst->height = height;
    acc_width = width + 1; acc_height = height + 1;
    inst->kernel = 0.0;
    /* allocate memory for the summed-area-table */
    inst->mem = (uint32_t*)malloc(acc_width * acc_height * SIZE_RGBA * sizeof(uint32_t));
    inst->acc = (uint32_t**)malloc(acc_width * acc_height * sizeof(uint32_t*));
    /* point at the right place */
    iter_mem = inst->mem;
    iter_acc = inst->acc;
    for (i = 0; i < acc_width * acc_height; ++i)
    {
        *iter_acc++ = iter_mem;
        iter_mem += SIZE_RGBA;
    }
    return (f0r_instance_t)inst;
}

static void blur_destruct(f0r_instance_t instance)
{
    squareblur_instance_t* inst =
        (squareblur_instance_t*)instance;
    free(inst->acc);
    free(inst->mem);
    free(instance);
}

static void blur_set_param_value(f0r_instance_t instance,
    f0r_param_t param, int param_index)
{
    assert(instance);
    squareblur_instance_t* inst = (squareblur_instance_t*)instance;

    switch (param_index)
    {
    case 0:
        /* kernel size */
        inst->kernel = *((double*)param);
        break;
    }
}

static void blur_get_param_value(f0r_instance_t instance,
    f0r_param_t param, int param_index)
{
    assert(instance);
    squareblur_instance_t* inst = (squareblur_instance_t*)instance;

    switch (param_index)
    {
    case 0:
        *((double*)param) = inst->kernel;
        break;
    }
}

static void blur_update(f0r_instance_t instance, double time,
    const uint32_t* inframe, uint32_t* outframe)
{
    assert(instance);
    squareblur_instance_t* inst = (squareblur_instance_t*)instance;

    unsigned int width = inst->width;
    unsigned int height = inst->height;
    unsigned int acc_width = width + 1; /* width of the summed area table */
    unsigned int max = MAX(width, height);
    unsigned int kernel_size = (unsigned int)(inst->kernel * max / 2.0);

    unsigned int x, y;
    unsigned int x0, x1, y0, y1;
    unsigned int area;

    if (kernel_size <= 0)
    {
        /* No blur, just copy image. */
        memcpy(outframe, inframe, width * height * sizeof(uint32_t));
    }
    else
    {
        assert(inst->acc);
        uint8_t* dst = (uint8_t*)outframe;
        uint32_t** acc = inst->acc;
        uint32_t sum[SIZE_RGBA];
        unsigned int y0_offset, y1_offset;

        /* Compute the summed area table. */
        update_summed_area_table(inst, inframe);

        /* Loop through the image's pixels. */
        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                /* The kernel's coordinates. */
                x0 = MAX(x - kernel_size, 0);
                x1 = MIN(x + kernel_size + 1, width);
                y0 = MAX(y - kernel_size, 0);
                y1 = MIN(y + kernel_size + 1, height);

                /* Get the sum in the current kernel. */
                area = (x1 - x0) * (y1 - y0);

                y0_offset = y0 * acc_width;
                y1_offset = y1 * acc_width;

                /* it is assumed that (x0,y0) <= (x1,y1) */
                memcpy(sum, acc[y1_offset + x1], SIZE_RGBA * sizeof(uint32_t));
                subtract_acc(sum, acc[y1_offset + x0]);
                subtract_acc(sum, acc[y0_offset + x1]);
                add_acc(sum, acc[y0_offset + x0]);

                /* Take the mean and copy it to output. */
                divide(dst, sum, area);

                /* Increment iterator. */
                dst += SIZE_RGBA;
            }
        }
    }
}


typedef struct glow_instance {
	double blur;
	int w, h;
	uint32_t* blurred;
	f0r_instance_t* blur_instance;
} glow_instance_t;

static int f0r_init()
{
  return 1;
}
static void f0r_deinit()
{ /* empty */ }

static void f0r_get_plugin_info( f0r_plugin_info_t* info )
{

}
static void f0r_get_param_info( f0r_param_info_t* info, int param_index )
{
	switch ( param_index ) {
		case 0:
			info->name = "Blur";
			info->type = F0R_PARAM_DOUBLE;
			info->explanation = "Blur of the glow";
			break;
	}
}

static f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
	glow_instance_t* inst = (glow_instance_t*)calloc(1, sizeof(*inst));
	inst->w = width;
	inst->h = height;
	inst->blurred = (uint32_t*)malloc( width * height * sizeof(uint32_t) );
	inst->blur_instance = (f0r_instance_t *)blur_construct( width, height );
	blur_set_param_value(inst->blur_instance, &inst->blur, 0 );
	return (f0r_instance_t)inst;
}
static void f0r_destruct(f0r_instance_t instance)
{
	glow_instance_t* inst = (glow_instance_t*)instance;
	blur_destruct(inst->blur_instance);
	free(inst->blurred);
	free(instance);
}
static void f0r_set_param_value(f0r_instance_t instance, 
                         f0r_param_t param, int param_index)
{
	glow_instance_t* inst = (glow_instance_t*)instance;
	switch ( param_index ) {
		case 0:
			inst->blur = (*((double*)param)) / 20.0;
			blur_set_param_value(inst->blur_instance, &inst->blur, 0 );
			break;
	}
}
static void f0r_get_param_value(f0r_instance_t instance,
                         f0r_param_t param, int param_index)
{
	glow_instance_t* inst = (glow_instance_t*)instance;
	switch ( param_index ) {
		case 0:
			*((double*)param) = inst->blur * 20.0;
			break;
	}
}
static void f0r_update(f0r_instance_t instance, double time,
                const uint32_t* inframe, uint32_t* outframe)
{
	glow_instance_t* inst = (glow_instance_t*)instance;

	uint8_t* dst = (uint8_t*)outframe;
	const uint8_t* src = (uint8_t*)inframe;
	const uint8_t* blur = (uint8_t*)inst->blurred;

	int len = inst->w * inst->h * 4;

	blur_update(inst->blur_instance, 0.0, inframe, inst->blurred );
	
	int i;
	for ( i = 0; i < len; i++ ) {
		*dst = 255 - ( ( 255 - *src ) * ( 255 - *blur ) ) / 255;
		// 1 - ( ( 1 - A ) * ( 1 - B ) )
		dst++;
		src++;
		blur++;
	}

}



//==================================================================================================
//export
filter_dest(glow,
	F0R_PLUGIN_TYPE_FILTER,
	F0R_COLOR_MODEL_RGBA8888,
	0,
	1,
	1,
	f0r_update,
	NULL);