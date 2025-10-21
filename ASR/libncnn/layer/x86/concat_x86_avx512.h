// Copyright 2019 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LAYER_CONCAT_X86_AVX512_H
#define LAYER_CONCAT_X86_AVX512_H

#include "concat.h"

namespace ncnn {

class Concat_x86_avx512 : public Concat
{
public:
    Concat_x86_avx512();

    virtual int forward(const std::vector<Mat>& bottom_blobs, std::vector<Mat>& top_blobs, const Option& opt) const;
};

} // namespace ncnn

#endif // LAYER_CONCAT_X86_AVX512_H
