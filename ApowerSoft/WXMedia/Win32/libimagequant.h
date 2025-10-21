#ifndef LIBIMAGEQUANT_H
#define LIBIMAGEQUANT_H

#ifndef LIQ_EXPORT
#define LIQ_EXPORT extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

//更新RGBA数据
typedef struct liq_attr   liq_attr;
LIQ_EXPORT liq_attr* liq_attr_create(int width, int height);
LIQ_EXPORT void liq_attr_process(liq_attr* attr, uint8_t* src, uint8_t** dst1, uint8_t* pal);
LIQ_EXPORT void liq_attr_destroy(liq_attr* attr);


#ifdef __cplusplus
}
#endif

#endif
