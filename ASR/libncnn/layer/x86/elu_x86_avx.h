// Copyright 2022 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LAYER_ELU_X86_AVX_H
#define LAYER_ELU_X86_AVX_H

#include "elu.h"

namespace ncnn {

class ELU_x86_avx : public ELU
{
public:
    ELU_x86_avx();

    virtual int forward_inplace(Mat& bottom_top_blob, const Option& opt) const;
};

} // namespace ncnn

#endif // LAYER_ELU_X86_AVX_H
