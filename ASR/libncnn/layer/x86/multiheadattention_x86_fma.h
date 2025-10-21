// Copyright 2023 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#ifndef LAYER_MULTIHEADATTENTION_X86_FMA_H
#define LAYER_MULTIHEADATTENTION_X86_FMA_H

#include "multiheadattention.h"

namespace ncnn {

class MultiHeadAttention_x86_fma : public MultiHeadAttention
{
public:
    MultiHeadAttention_x86_fma();

    virtual int create_pipeline(const Option& opt);
    virtual int destroy_pipeline(const Option& opt);

    virtual int forward(const std::vector<Mat>& bottom_blobs, std::vector<Mat>& top_blobs, const Option& opt) const;

public:
    Layer* q_gemm;
    Layer* k_gemm;
    Layer* v_gemm;
    Layer* o_gemm;

    Layer* qk_gemm;
    Layer* qkv_gemm;

    Layer* qk_softmax;
};

} // namespace ncnn

#endif // LAYER_MULTIHEADATTENTION_X86_FMA_H
