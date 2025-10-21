// Copyright 2020 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LAYER_MISH_X86_AVX_H
#define LAYER_MISH_X86_AVX_H

#include "mish.h"

namespace ncnn {

class Mish_x86_avx : public Mish
{
public:
    Mish_x86_avx();

    virtual int forward_inplace(Mat& bottom_top_blob, const Option& opt) const;
};

} // namespace ncnn

#endif // LAYER_MISH_X86_AVX_H
