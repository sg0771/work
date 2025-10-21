// Copyright 2022 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LAYER_GELU_X86_AVX512_H
#define LAYER_GELU_X86_AVX512_H

#include "gelu.h"

namespace ncnn {

class GELU_x86_avx512 : public GELU
{
public:
    GELU_x86_avx512();

    virtual int create_pipeline(const Option& opt);
    virtual int forward_inplace(Mat& bottom_top_blob, const Option& opt) const;
};

} // namespace ncnn

#endif // LAYER_GELU_X86_AVX512_H
