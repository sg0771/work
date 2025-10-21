// Copyright 2019 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LAYER_DROPOUT_X86_AVX_H
#define LAYER_DROPOUT_X86_AVX_H

#include "dropout.h"

namespace ncnn {

class Dropout_x86_avx : public Dropout
{
public:
    Dropout_x86_avx();

    virtual int forward_inplace(Mat& bottom_top_blob, const Option& opt) const;
};

} // namespace ncnn

#endif // LAYER_DROPOUT_X86_AVX_H
