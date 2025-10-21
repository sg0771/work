// offline-lm-config.h
//
// Copyright (c)  2023  Xiaomi Corporation
#ifndef SHERPA_ONNX_CSRC_OFFLINE_LM_CONFIG_H_
#define SHERPA_ONNX_CSRC_OFFLINE_LM_CONFIG_H_

#include <string>

#include "parse-options.h"

namespace sherpa_onnx {

struct OfflineLMConfig {
  // path to the onnx model
  std::string model;

  // LM scale
  float scale = 0.5;
  int32_t lm_num_threads = std::thread::hardware_concurrency();
  std::string lm_provider = "cpu";

  OfflineLMConfig() = default;

  OfflineLMConfig(const std::string &model, float scale, int32_t lm_num_threads,
                  const std::string &lm_provider)
      : model(model),
        scale(scale),
        lm_num_threads(lm_num_threads),
        lm_provider(lm_provider) {}

  void Register(ParseOptions *po);
  bool Validate() const;

  std::string ToString() const;
};

}  // namespace sherpa_onnx

#endif  // SHERPA_ONNX_CSRC_OFFLINE_LM_CONFIG_H_
