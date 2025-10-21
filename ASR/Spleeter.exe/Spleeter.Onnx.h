// offline-source-separation.h
//
// Copyright (c)  2025  Xiaomi Corporation

#ifndef SHERPA_ONNX_CSRC_OFFLINE_SOURCE_SEPARATION_H_
#define SHERPA_ONNX_CSRC_OFFLINE_SOURCE_SEPARATION_H_

#include <stdio.h>
#include <chrono> 
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <thread>
#include <utility>
#include <cassert>
#include <ostream>
#include <algorithm>
#include <functional>
#include <numeric>
#include <sstream>

namespace sherpa_onnx {

	struct OfflineSourceSeparationModelConfig {
		std::string vocals;
		std::string accompaniment;
		int32_t num_threads = std::thread::hardware_concurrency();
	};

	struct MultiChannelSamples {
		std::vector<std::vector<float>> data;
	};

	//输入
	struct OfflineSourceSeparationInput {
		MultiChannelSamples samples;
		int32_t sample_rate;
	};

	//输出
	struct OfflineSourceSeparationOutput {
		std::vector<MultiChannelSamples> stems;
		int32_t sample_rate;
	};

	class OfflineSourceSeparationImpl;

	//分离器
	class OfflineSourceSeparation {
	public:
		virtual ~OfflineSourceSeparation();

		OfflineSourceSeparation(const OfflineSourceSeparationModelConfig& config);

		//处理
		OfflineSourceSeparationOutput Process(
			const OfflineSourceSeparationInput& input) const;

		int32_t GetOutputSampleRate() const;

		// e.g., it is 2 for 2stems from spleeter
		int32_t GetNumberOfStems() const;

	private:
		//impl 处理
		std::unique_ptr<OfflineSourceSeparationImpl> impl_;
	};

}  // namespace sherpa_onnx

#endif  // SHERPA_ONNX_CSRC_OFFLINE_SOURCE_SEPARATION_H_
