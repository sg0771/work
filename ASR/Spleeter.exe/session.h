// session.h
//
// Copyright (c)  2023  Xiaomi Corporation

#ifndef SHERPA_ONNX_CSRC_SESSION_H_
#define SHERPA_ONNX_CSRC_SESSION_H_

#include <string>

#include "onnxruntime\include\onnxruntime_cxx_api.h"  // NOLINT
#include "offline-lm-config.h"
#include "online-lm-config.h"
#include "online-model-config.h"

namespace sherpa_onnx {

Ort::SessionOptions GetSessionOptionsImpl(
    int32_t num_threads, const std::string &provider_str,
    const ProviderConfig *provider_config = nullptr);

Ort::SessionOptions GetSessionOptions(const OfflineLMConfig &config);
Ort::SessionOptions GetSessionOptions(const OnlineLMConfig &config);

Ort::SessionOptions GetSessionOptions(const OnlineModelConfig &config);

Ort::SessionOptions GetSessionOptions(const OnlineModelConfig &config,
                                      const std::string &model_type);

Ort::SessionOptions GetSessionOptions(int32_t num_threads,
                                      const std::string &provider_str);

template <typename T>
Ort::SessionOptions GetSessionOptions(const T &config) {
  return GetSessionOptionsImpl(config.num_threads, config.provider);
}

}  // namespace sherpa_onnx

#endif  // SHERPA_ONNX_CSRC_SESSION_H_
