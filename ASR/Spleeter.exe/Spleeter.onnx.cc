// sherpa-onnx-offline-source-separation.cc
//
// Copyright (c)  2025  Xiaomi Corporation
#include <stdio.h>

#include <chrono>  // NOLINT
#include <string>

#pragma comment(lib, "wxonnx.lib")
#include "offline-source-separation.h"
#include "wave-reader.h"
#include "wave-writer.h"

std::string trimQuotes(const std::string& str) {
    if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.size() - 2);
    }
    return str;
}

int main(int32_t argc, char *argv[]) {

  std::string input_wave;
  std::string output_vocals_wave;
  std::string output_accompaniment_wave;
  std::string vocal_model;
  std::string accp_model;


  for (int i = 1; i < argc; i += 2) {
      const char* strParam = argv[i];
      const char* strValue = argv[i + 1];
      if (stricmp(strParam, "-i") == 0 || stricmp(strParam, "--input") == 0) {
          input_wave = strValue;
      }
      if (stricmp(strParam, "-v") == 0 || stricmp(strParam, "--vocal") == 0) {
          vocal_model = strValue;
      }
      if (stricmp(strParam, "-a") == 0 || stricmp(strParam, "--accp") == 0) {
          accp_model = strValue;
      }
      if (stricmp(strParam, "-o") == 0 || stricmp(strParam, "--output") == 0) {
          output_vocals_wave = strValue;
      }
      if (stricmp(strParam, "-oa") == 0 || stricmp(strParam, "--oaccp") == 0) {
          output_accompaniment_wave = strValue;
      }
  }
  input_wave = trimQuotes(input_wave);
  vocal_model = trimQuotes(vocal_model);
  accp_model = trimQuotes(accp_model);
  output_vocals_wave = trimQuotes(output_vocals_wave);
  output_accompaniment_wave = trimQuotes(output_accompaniment_wave);

  if (input_wave.empty()) {
    fprintf(stderr, "Please provide input file as -i xxx.wav\n");
    exit(EXIT_FAILURE);
  }
  if (vocal_model.empty() && accp_model.empty()) {
      fprintf(stderr, "Please provide model file -v and -a \n");
      exit(EXIT_FAILURE);
  }

  if (output_vocals_wave.empty() && output_accompaniment_wave.empty()) {
    fprintf(stderr, "Please provide output file -o or -oa \n");
    exit(EXIT_FAILURE);
  }
  sherpa_onnx::OfflineSourceSeparationConfig config;//model 包含vocal accp 模型路径
  config.model.spleeter.vocals = vocal_model;
  config.model.spleeter.accompaniment = accp_model;

  if (!config.Validate()) { //检查模型是否存在
    fprintf(stderr, "Errors in config!\n");
    exit(EXIT_FAILURE);
  }

  bool is_ok = false;
  sherpa_onnx::OfflineSourceSeparationInput input;
  input.samples.data = sherpa_onnx::ReadWaveMultiChannel(input_wave, &input.sample_rate, &is_ok);
  if (!is_ok) {
    fprintf(stderr, "Failed to read '%s'\n", input_wave.c_str());
    return -1;
  }

  fprintf(stderr, "Started\n");

  sherpa_onnx::OfflineSourceSeparation sp(config);

  const auto begin = std::chrono::steady_clock::now();
  auto output = sp.Process(input);//推理
  const auto end = std::chrono::steady_clock::now();

  float elapsed_seconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count() /
      1000.;

  //输出人声音频
  if (!output_vocals_wave.empty()) {
      is_ok = sherpa_onnx::WriteWave(
          output_vocals_wave, output.sample_rate, output.stems[0].data[0].data(),
          output.stems[0].data[1].data(), output.stems[0].data[0].size());

      if (!is_ok) {
          fprintf(stderr, "Failed to write to '%s'\n", output_vocals_wave.c_str());
          exit(EXIT_FAILURE);
      }
      fprintf(stderr, "Saved to write to '%s'\n", output_vocals_wave.c_str());
  }

  //输出伴奏音频
  if (!output_accompaniment_wave.empty()) {
      is_ok = sherpa_onnx::WriteWave(output_accompaniment_wave, output.sample_rate,
          output.stems[1].data[0].data(),
          output.stems[1].data[1].data(),
          output.stems[1].data[0].size());

      if (!is_ok) {
          fprintf(stderr, "Failed to write to '%s'\n",
              output_accompaniment_wave.c_str());
          exit(EXIT_FAILURE);
      }
      fprintf(stderr, "Saved to write to '%s'\n", output_accompaniment_wave.c_str());
  };


  fprintf(stderr, "Done\n");

  float duration = input.samples.data[0].size() / static_cast<float>(input.sample_rate);
  fprintf(stderr, "num threads: %d\n", config.model.num_threads);
  fprintf(stderr, "Elapsed seconds: %.3f s\n", elapsed_seconds);
  float rtf =  duration/ elapsed_seconds;
  fprintf(stderr, "Real time factor (RTF): %.3f / %.3f = %.3f\n",
          elapsed_seconds, duration, rtf);

  return 0;
}
