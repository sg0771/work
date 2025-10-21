// Copyright 2017 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LAYER_BATCHNORM_X86_AVX_H
#define LAYER_BATCHNORM_X86_AVX_H

#include "batchnorm.h"

namespace ncnn {

class BatchNorm_x86_avx : public BatchNorm
{
public:
    BatchNorm_x86_avx();

    virtual int forward_inplace(Mat& bottom_top_blob, const Option& opt) const;
};

} // namespace ncnn

#endif // LAYER_BATCHNORM_X86_AVX_H
