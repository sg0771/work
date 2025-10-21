// Copyright 2017 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LAYER_HARDSIGMOID_X86_AVX_H
#define LAYER_HARDSIGMOID_X86_AVX_H

#include "hardsigmoid.h"

namespace ncnn {

class HardSigmoid_x86_avx : public HardSigmoid
{
public:
    HardSigmoid_x86_avx();

    virtual int forward_inplace(Mat& bottom_top_blob, const Option& opt) const;
};

} // namespace ncnn

#endif // LAYER_HARDSIGMOID_X86_AVX_H
