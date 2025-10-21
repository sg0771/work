/**
 * Copyright (c)  2022  Xiaomi Corporation (authors: Fangjun Kuang)
 *
 * See LICENSE for clarification regarding multiple authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <cctype>  // std::tolower

#include "../portaudio/portaudio.h"  // NOLINT
#include "./display.h"
#include "./microphone.h"
#include "./recognizer.h"

#pragma comment(lib, "portaudio.lib")
#pragma comment(lib,"libncnn.lib")
#pragma comment(lib,"kaldi-native-fbank.lib")
#pragma comment(lib,"kissfft.lib")
#pragma comment(lib,"sherpa-ncnn-core.lib")

bool stop = false;

static int32_t RecordCallback(const void *input_buffer,
                              void * /*output_buffer*/,
                              unsigned long frames_per_buffer,  // NOLINT
                              const PaStreamCallbackTimeInfo * /*time_info*/,
                              PaStreamCallbackFlags /*status_flags*/,
                              void *user_data) {
  auto s = reinterpret_cast<sherpa_ncnn::Stream *>(user_data);

  s->AcceptWaveform(16000, reinterpret_cast<const float *>(input_buffer),
                    frames_per_buffer);

  return stop ? paComplete : paContinue;
}

static void Handler(int32_t sig) {
  stop = true;
  fprintf(stderr, "\nCaught Ctrl + C. Exiting...\n");
};

int32_t main(int32_t argc, char *argv[]) {
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);
  signal(SIGINT, Handler);

  sherpa_ncnn::RecognizerConfig config;
  // std::string strDir = "D:\\sherpa-ncnn-test\\streaming-zipformer\\";
  // std::string strDir = "D:\\sherpa-ncnn-test\\int8\\";
  std::string strDir =  "D:\\sherpa-ncnn-test\\streaming-zipformer-small-bilingual\\";

  config.model_config.tokens = strDir + "tokens.txt ";
  config.model_config.encoder_param = strDir + "encoder_jit_trace-pnnx.ncnn.param";
  config.model_config.encoder_bin = strDir + "encoder_jit_trace-pnnx.ncnn.bin";
  config.model_config.decoder_param = strDir + "decoder_jit_trace-pnnx.ncnn.param";
  config.model_config.decoder_bin =  strDir + "decoder_jit_trace-pnnx.ncnn.bin";
  config.model_config.joiner_param = strDir + "joiner_jit_trace-pnnx.ncnn.param";
  config.model_config.joiner_bin = strDir + "joiner_jit_trace-pnnx.ncnn.bin";
  
  int32_t num_threads = 4;
  config.model_config.encoder_opt.num_threads = num_threads;
  config.model_config.decoder_opt.num_threads = num_threads;
  config.model_config.joiner_opt.num_threads = num_threads;

  const float expected_sampling_rate = 16000;
  if (argc == 10) {
    std::string method = argv[9];
    if (method == "greedy_search" || method == "modified_beam_search") {
      config.decoder_config.method = method;
    }
  }

  //if (argc >= 11) {
  //  config.hotwords_file = argv[10];
  //}

  //if (argc == 12) {
  //  config.hotwords_score = atof(argv[11]);
  //}

  config.enable_endpoint = true;

  config.endpoint_config.rule1.min_trailing_silence = 2.4;
  config.endpoint_config.rule2.min_trailing_silence = 1.2;
  config.endpoint_config.rule3.min_utterance_length = 300;

  config.feat_config.sampling_rate = expected_sampling_rate;
  config.feat_config.feature_dim = 80;

  fprintf(stderr, "%s\n", config.ToString().c_str());

  sherpa_ncnn::Recognizer recognizer(config);
  auto s = recognizer.CreateStream();

  sherpa_ncnn::Microphone mic;

  PaDeviceIndex num_devices = Pa_GetDeviceCount();
  fprintf(stderr, "Num devices: %d\n", num_devices);

  PaStreamParameters param;

  param.device = Pa_GetDefaultInputDevice();
  if (param.device == paNoDevice) {
    fprintf(stderr, "No default input device found\n");
    exit(EXIT_FAILURE);
  }
  fprintf(stderr, "Use default device: %d\n", param.device);

  const PaDeviceInfo *info = Pa_GetDeviceInfo(param.device);
  fprintf(stderr, "  Name: %s\n", info->name);
  fprintf(stderr, "  Max input channels: %d\n", info->maxInputChannels);

  param.channelCount = 1;
  param.sampleFormat = paFloat32;

  param.suggestedLatency = info->defaultLowInputLatency;
  param.hostApiSpecificStreamInfo = nullptr;
  const float sample_rate = 16000;

  PaStream *stream;
  PaError err =
      Pa_OpenStream(&stream, &param, nullptr, /* &outputParameters, */
                    sample_rate,
                    0,          // frames per buffer
                    paClipOff,  // we won't output out of range samples
                                // so don't bother clipping them
                    RecordCallback, s.get());
  if (err != paNoError) {
    fprintf(stderr, "portaudio error: %s\n", Pa_GetErrorText(err));
    exit(EXIT_FAILURE);
  }

  err = Pa_StartStream(stream);
  fprintf(stderr, "Started\n");

  if (err != paNoError) {
    fprintf(stderr, "portaudio error: %s\n", Pa_GetErrorText(err));
    exit(EXIT_FAILURE);
  }

  std::string last_text;
  int32_t segment_index = 0;
  sherpa_ncnn::Display display;
  while (!stop) {
    while (recognizer.IsReady(s.get())) {
      recognizer.DecodeStream(s.get());
    }

    bool is_endpoint = recognizer.IsEndpoint(s.get());

    if (is_endpoint) {
      s->Finalize();
    }
    auto text = recognizer.GetResult(s.get()).text;

    if (!text.empty() && last_text != text) {
      last_text = text;

      std::transform(text.begin(), text.end(), text.begin(),
                     [](auto c) { return std::tolower(c); });

      display.Print(segment_index, text);
    }

    if (is_endpoint) {
      if (!text.empty()) {
        ++segment_index;
      }

      recognizer.Reset(s.get());
    }

    Pa_Sleep(20);  // sleep for 20ms
  }

  err = Pa_CloseStream(stream);
  if (err != paNoError) {
    fprintf(stderr, "portaudio error: %s\n", Pa_GetErrorText(err));
    exit(EXIT_FAILURE);
  }

  return 0;
}
