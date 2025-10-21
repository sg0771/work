// Copyright 2019 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LAYER_PACKING_X86_AVX_H
#define LAYER_PACKING_X86_AVX_H

#include "packing.h"

namespace ncnn {

class Packing_x86_avx : public Packing
{
public:
    Packing_x86_avx();

    virtual int forward(const Mat& bottom_blob, Mat& top_blob, const Option& opt) const;

protected:
    int forward_int8(const Mat& bottom_blob, Mat& top_blob, const Option& opt) const;
};

} // namespace ncnn

#endif // LAYER_PACKING_X86_AVX_H
