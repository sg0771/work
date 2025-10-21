// sherpa-onnx-offline-source-separation.cc
//
// Copyright (c)  2025  Xiaomi Corporation


#pragma comment(lib, "wxonnx.lib")
#include "Spleeter.Onnx.h"
#include "wave-reader.h"
#include "wave-writer.h"

static bool FileExist(const std::string& path) {
    try {
        if (std::filesystem::exists(path)) {
            return true;
        }
    }
    catch (...) {
        return false;
    }
    return false;
}

static bool TestOutFile(std::string filename) {
    if (filename.length() == 0)
        return false;
    std::ofstream outputFile(filename, std::ios::binary);//���Դ�������ļ�
    if (!outputFile.is_open()) {
        std::cerr << "TestOutFile Failed to create file." << filename << std::endl;
        std::error_code ec;
        auto status = std::filesystem::status(filename, ec);
        if (!ec) {
            if (!std::filesystem::exists(status)) {
                std::cerr << "����: �ļ�������" << std::endl;
            }
            else if (!std::filesystem::is_regular_file(status)) {
                std::cerr << "����: Ŀ�겻���ļ�" << std::endl;
            }
        }
        else {
            std::cerr << "Filesystem����: " << ec.message() << std::endl;
        }
        return false;
    }
    outputFile.close();
    std::filesystem::remove(filename); //ɾ�������ļ�
    return true;
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

  //�����������ļ�·��
  if (!FileExist(input_wave)) {
    std::cout << "Please provide input file as -i xxx.wav\n" <<std::endl;
    exit(EXIT_FAILURE);
  }
  //�������ģ��·��
  if (!FileExist(vocal_model)) {
      std::cout << "Please provide vocal model file as -v vocal.onnx" << std::endl;
      exit(EXIT_FAILURE);
  }
  //������ģ��·��
  if (!FileExist(accp_model)) {
      std::cout << "Please provide accp model file as -a accp.onnx" << std::endl;
      exit(EXIT_FAILURE);
  }

  bool hasExport = false;
  if (!output_vocals_wave.empty()) {
	  if (!TestOutFile(output_vocals_wave)) {
          std::cout << "Please provide vocal output file as -v vocal.wav" << std::endl;
          output_vocals_wave = "";
      }
      else {
          hasExport = true;
      }
  }

  if (!output_accompaniment_wave.empty()) {
      if (!TestOutFile(output_accompaniment_wave)) {
          std::cout << "Please provide accp output file as -v accp.wav" << std::endl;
          output_accompaniment_wave = "";
      }
      else {
          hasExport = true;
      }
  }

  if (!hasExport) {
      std::cout << "Please provide output file -o or -oa " << std::endl;
    exit(EXIT_FAILURE);
  }

  //��ȡ������Ƶ�ļ�
  bool is_ok = false;
  sherpa_onnx::OfflineSourceSeparationInput input;
  input.samples.data = sherpa_onnx::ReadWaveMultiChannel(input_wave, &input.sample_rate, &is_ok);
  if (!is_ok) {
    fprintf(stderr, "Failed to read '%s'\n", input_wave.c_str());
    return -1;
  }

  fprintf(stderr, "Started\n");

  //model ����vocal accp ģ��·��
  sherpa_onnx::OfflineSourceSeparationModelConfig config;
  config.vocals = vocal_model;
  config.accompaniment = accp_model;
  config.num_threads = std::thread::hardware_concurrency();
  //����ģ��
  sherpa_onnx::OfflineSourceSeparation sp(config);

  const auto begin = std::chrono::steady_clock::now();
  auto output = sp.Process(input);//����
  const auto end = std::chrono::steady_clock::now();

  float elapsed_seconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - begin)
          .count() /
      1000.;

  //���������Ƶ
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

  //���������Ƶ
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
  fprintf(stderr, "num threads: %d\n", config.num_threads);
  fprintf(stderr, "Elapsed seconds: %.3f s\n", elapsed_seconds);
  float rtf =  duration/ elapsed_seconds;
  fprintf(stderr, "Real time factor (RTF): %.3f / %.3f = %.3f\n",
          elapsed_seconds, duration, rtf);

  return 0;
}
