// Copyright 2017 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LAYER_SIGMOID_X86_AVX_H
#define LAYER_SIGMOID_X86_AVX_H

#include "sigmoid.h"

namespace ncnn {

class Sigmoid_x86_avx : public Sigmoid
{
public:
    Sigmoid_x86_avx();

    virtual int forward_inplace(Mat& bottom_top_blob, const Option& opt) const;
};

} // namespace ncnn

#endif // LAYER_SIGMOID_X86_AVX_H
