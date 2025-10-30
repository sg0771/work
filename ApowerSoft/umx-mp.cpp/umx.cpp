#include "dsp.hpp"
#include "inference.hpp"
#include "lstm.hpp"
#include "model.hpp"
#include "wiener.hpp"
#include <Eigen/Core>
#include <Eigen/Dense>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <unsupported/Eigen/FFT>
#include <vector>
#include <fstream>

using namespace umxcpp;

// forward declarations
static std::vector<Eigen::MatrixXf>
shift_inference(struct umxcpp::umx_model& model, Eigen::MatrixXf& full_audio);

static std::vector<Eigen::MatrixXf>
split_inference(struct umxcpp::umx_model& model, Eigen::MatrixXf& full_audio);

struct WavHeader_Read {
    uint32_t chunkId/*[4]*/;          // "RIFF"
    uint32_t chunkSize;       // �ļ��ܴ�С - 8
    uint32_t format/*[4]*/;           // "WAVE"
    uint32_t subchunk1Id/*[4]*/;      // "fmt "
    uint32_t subchunk1Size;   // ͨ��Ϊ16
    uint16_t audioFormat;     // ����1 (PCM)
    uint16_t numChannels;     // ������
    uint32_t sampleRate;      // ������
    uint32_t byteRate;        // ÿ�������ֽ���
    uint16_t blockAlign;      // ÿ������֡���ֽ���
    uint16_t bitsPerSample;   // ÿ��������λ��
    //LIST Ԫ����
    //uint32_t subchunk2Id/*[4]*/;      // "data"
    //uint32_t subchunk2Size;   // ��Ƶ���ݴ�С
};

struct WavHeader_Write {
    uint32_t chunkId/*[4]*/;          // "RIFF"
    uint32_t chunkSize;       // �ļ��ܴ�С - 8
    uint32_t format/*[4]*/;           // "WAVE"
    uint32_t subchunk1Id/*[4]*/;      // "fmt "
    uint32_t subchunk1Size;   // ͨ��Ϊ16
    uint16_t audioFormat;     // ����1 (PCM)
    uint16_t numChannels;     // ������
    uint32_t sampleRate;      // ������
    uint32_t byteRate;        // ÿ�������ֽ���
    uint16_t blockAlign;      // ÿ������֡���ֽ���
    uint16_t bitsPerSample;   // ÿ��������λ��
    uint32_t subchunk2Id/*[4]*/;      // "data"
    uint32_t subchunk2Size;   // ��Ƶ���ݴ�С
};

/// Write the audio data to a WAV file
static void write_audio_file(const Eigen::MatrixXf& waveform, const WavHeader_Write& header,
    std::wstring filename, short* pSample) {
    std::ofstream outputFile(filename, std::ios::binary);//������������ļ�
    if (!outputFile.is_open()) {
        std::wcerr << L"Failed to create file." << filename << std::endl;
        return;
    }
    outputFile.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader_Write)); //д��WAVͷ
    int nSize = 44100 * 4;//1s����
    int nIndex = 0;
    int nSample = waveform.cols();

    int bRun = 1;
    while (bRun)
    {
        // Write the audio data
        int nWrite = 0;
        for (nWrite = 0; nWrite < 44100; nWrite++) {
            if (nWrite + nIndex >= nSample) {
                bRun = 0;//���һ����
                break;
            }
            pSample[2 * nWrite + 0] = static_cast<short>(waveform(0, nWrite + nIndex) * INT16_MAX);     // left channel
            pSample[2 * nWrite + 1] = static_cast<short>(waveform(1, nWrite + nIndex) * INT16_MAX); // right channel
        }
        if (nWrite <= 0)
            break;
        outputFile.write(reinterpret_cast<const char*>(pSample), nWrite * 4);
        nIndex += nWrite;
    }
    outputFile.close();
}


static bool Exist(const std::wstring& path) {
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

static bool TestOutFile(std::wstring filename) {
    if (filename.length() == 0)
        return false;
    std::ofstream outputFile(filename, std::ios::binary);//���Դ�������ļ�
    if (!outputFile.is_open()) {
        std::wcerr << L"TestOutFile Failed to create file." << filename << std::endl;
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

int wmain(int argc, const wchar_t** argv)
{

    // init parallelism for eigen
#ifdef _OPENMP
    Eigen::initParallel();
#endif

    std::wstring model_file = L"";//ģ���ļ�
    std::wstring input_file = L"";//�����ļ�

    std::wstring output_drum_file = L"";//����ĵ� -d --drum  //���0
    std::wstring output_bass_file = L"";//�����˹ -b --bass  //���1
    std::wstring output_accp_file = L"";//�������  -a --accp //���3
    std::wstring output_vocal_file = L"";//������� -v --vocal   //���3

    for (int i = 1; i < argc; i += 2) {
        const wchar_t* strParam = argv[i];
        const wchar_t* strValue = argv[i + 1];
        if (wcsicmp(strParam, L"-m") == 0 || wcsicmp(strParam, L"--model") == 0) {
            model_file = strValue;
        }
        if (wcsicmp(strParam, L"-i") == 0 || wcsicmp(strParam, L"--input") == 0) {
            input_file = strValue;
        }
        if (wcsicmp(strParam, L"-v") == 0 || wcsicmp(strParam, L"--vocal") == 0) {
            output_vocal_file = strValue;
        }
        if (wcsicmp(strParam, L"-a") == 0 || wcsicmp(strParam, L"--accp") == 0) {
            output_accp_file = strValue;
        }
        if (wcsicmp(strParam, L"-d") == 0 || wcsicmp(strParam, L"--drum") == 0) {
            output_drum_file = strValue;
        }
        if (wcsicmp(strParam, L"-b") == 0 || wcsicmp(strParam, L"--bass") == 0) {
            output_bass_file = strValue;
        }
    }
    if (!Exist(model_file)) {
        std::wcout << L"No model_file " << model_file << std::endl;
        return -1;
    }
    if (!Exist(input_file)) {
        std::wcout << L"No input_file " << input_file << std::endl;
        return -1;
    }

    bool bHasOutput = false; //�Ƿ�������ļ�
    if (!TestOutFile(output_drum_file)) {
        output_drum_file = L"";
    }
    else {
        bHasOutput = true; //������ļ�
    }
    if (!TestOutFile(output_bass_file)) {
        output_bass_file = L"";
    }
    else {
        bHasOutput = true; //������ļ�
    }
    if (!TestOutFile(output_accp_file)) {
        output_accp_file = L"";
    }
    else {
        bHasOutput = true; //������ļ�
    }
    if (!TestOutFile(output_vocal_file)) {
        output_vocal_file = L"";
    }
    else {
        bHasOutput = true; //������ļ�
    }

    if (!bHasOutput) {
        std::cout << "not output_file error" << std::endl;
        return -1;
    }

    auto start = std::chrono::high_resolution_clock::now();

    std::cout << "umx.cpp Main driver program" << std::endl;

    //��ȡ�ļ�
    WavHeader_Read headerRead;    //��ȡWAVͷ
    WavHeader_Write headerWrite;    //���WAVͷ���Ƴ� LIST INFO
    uint32_t subchunk2Size = 0; //��Ƶ���ݴ�С


    std::ifstream inputFile(input_file, std::ios::binary); //�������ļ�
    if (!inputFile.is_open()) {
        std::wcerr << L"Failed to open input file." << input_file << std::endl;
        return -1;
    }

    inputFile.read(reinterpret_cast<char*>(&headerRead), sizeof(WavHeader_Read)); //��ȡWAV    
    //����ļ�ͷ
    memcpy(&headerWrite, &headerRead, sizeof(WavHeader_Read));
    headerWrite.subchunk2Id = 0x61746164; // "data"
    headerWrite.subchunk2Size = subchunk2Size;//��Ƶ���ݴ�С

    if (headerRead.chunkId != 0x46464952 || headerRead.format != 0x45564157) { //����ļ�ͷ
        std::cerr << "Invalid WAV file." << std::endl;
        return -1;
    }
    if (headerRead.subchunk1Size != 16 ||
        headerRead.audioFormat != 1 ||
        headerRead.sampleRate != 44100 ||
        headerRead.numChannels != 2) {
        std::cerr << "Is Not 44100 2Channel S16 PCM format." << std::endl;
        return -1;
    }

    while (true) {
        uint32_t temp1;
        uint32_t temp2;
        inputFile.read(reinterpret_cast<char*>(&temp1), sizeof(uint32_t));
        if (temp1 == 0x5453494C) { // "LIST"
            inputFile.read(reinterpret_cast<char*>(&temp2), sizeof(uint32_t));
            // ������ǰ�ӿ�
            inputFile.seekg(temp2, std::ios::cur);
        }
        else if (temp1 == 0x61746164) { // "data"
            inputFile.read(reinterpret_cast<char*>(&subchunk2Size), sizeof(uint32_t));
            break;
        }
        std::streamsize bytesRead = inputFile.gcount();
        if (bytesRead != 4) {
            std::cerr << "File EOF." << std::endl;
            return -1;
        }
    }

    headerWrite.chunkSize = subchunk2Size + 36;
    headerWrite.subchunk2Size = subchunk2Size;//����Ƶ���ݴ�С
    int nSample = subchunk2Size / 4; //������
    double duration = nSample / 44100; //��Ƶʱ��
    std::cout << "nSample " << nSample << " ��Ƶʱ��: " << duration << "s" << std::endl;

    Eigen::MatrixXf audioSource(2, nSample);
    // ��ȡ��Ƶ����
    short* pSample = new short[44100 * 2];
    int nSize = 44100 * 4;//1s����
    int nIndex = 0;
    while (true) {
        inputFile.read(reinterpret_cast<char*>(pSample), nSize);
        std::streamsize bytesRead = inputFile.gcount();
        if (bytesRead <= 0)
            break;
        int nRead = bytesRead / 4;
        for (size_t i = 0; i < nRead; i++)
        {
            audioSource(0, nIndex + i) = pSample[2 * i] / static_cast<float>(INT16_MAX);     // left channel
            audioSource(1, nIndex + i) = pSample[2 * i + 1] / static_cast<float>(INT16_MAX);; // right channel
        }
        nIndex += nRead;
    }
    inputFile.close();

    // initialize a struct umx_model
    struct umx_model model
    {
    };

    auto ret = load_umx_model(model_file, &model);
    std::cout << "umx_model_load returned " << (ret ? "true" : "false")
        << std::endl;
    if (!ret)
    {
        std::cerr << "Error loading model" << std::endl;
        exit(1);
    }

    std::vector<Eigen::MatrixXf> target_waveforms =
        shift_inference(model, audioSource); // shift inference

    // ������ʱ
    auto end = std::chrono::high_resolution_clock::now();

    // �������ʱ�䣨���룩
    auto nProgTime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    //��demucs.cpp ˳����ȫһ��
    //��˹
    if (output_bass_file.length() > 0)
    {
        Eigen::MatrixXf audio_target = target_waveforms[0];
        write_audio_file(audio_target, headerWrite, output_bass_file, pSample);//д���ļ�
    }
    //�ĵ�
    if (output_drum_file.length() > 0)
    {
        Eigen::MatrixXf audio_target = target_waveforms[1];
        write_audio_file(audio_target, headerWrite, output_drum_file, pSample);//д���ļ�
    }
    //����
    if (output_accp_file.length() > 0)
    {
        Eigen::MatrixXf audio_target = target_waveforms[2];
        write_audio_file(audio_target, headerWrite, output_accp_file, pSample);//д���ļ�
    }
    //����
    if (output_vocal_file.length() > 0)
    {
        Eigen::MatrixXf audio_target = target_waveforms[3];
        write_audio_file(audio_target, headerWrite, output_vocal_file, pSample);//д���ļ�
    }
    delete[]pSample;
    std::cout << "����ִ��ʱ��: " << nProgTime / 1000.0 << "s" << std::endl;
    return 0;
}

static std::vector<Eigen::MatrixXf>
shift_inference(struct umxcpp::umx_model& model, Eigen::MatrixXf& full_audio)
{
    int length = full_audio.cols();

    // first, apply shifts for time invariance
    // we simply only support shift=1, the demucs default
    // shifts (int): if > 0, will shift in time `mix` by a random amount between
    // 0 and 0.5 sec
    //     and apply the oppositve shift to the output. This is repeated
    //     `shifts` time and all predictions are averaged. This effectively
    //     makes the model time equivariant and improves SDR by up to 0.2
    //     points.
    int max_shift =
        (int)(umxcpp::MAX_SHIFT_SECS * umxcpp::SUPPORTED_SAMPLE_RATE);

    int offset = rand() % max_shift;

    // populate padded_full_audio with full_audio starting from
    // max_shift to max_shift + full_audio.cols()
    // incorporate random offset at the same time
    Eigen::MatrixXf shifted_audio =
        Eigen::MatrixXf::Zero(2, length + max_shift - offset);
    shifted_audio.block(0, offset, 2, length) = full_audio;

    std::vector<Eigen::MatrixXf> waveform_outputs =
        split_inference(model, shifted_audio);

    // trim the output to the original length
    // waveform_outputs = waveform_outputs[..., max_shift:max_shift + length]
    std::vector<Eigen::MatrixXf> trimmed_waveform_outputs;

    trimmed_waveform_outputs.push_back(Eigen::MatrixXf(2, length));
    trimmed_waveform_outputs.push_back(Eigen::MatrixXf(2, length));
    trimmed_waveform_outputs.push_back(Eigen::MatrixXf(2, length));
    trimmed_waveform_outputs.push_back(Eigen::MatrixXf(2, length));

    for (int i = 0; i < 4; ++i)
    {
        trimmed_waveform_outputs[i].setZero();
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < length; ++k)
            {
                trimmed_waveform_outputs[i](j, k) =
                    waveform_outputs[i](j, k + offset);
            }
        }
    }

    return trimmed_waveform_outputs;
}

static std::vector<Eigen::MatrixXf>
split_inference(struct umxcpp::umx_model& model, Eigen::MatrixXf& full_audio)
{
    // calculate segment in samples
    int segment_samples = (int)(umxcpp::SEGMENT_LEN_SECS * umxcpp::SUPPORTED_SAMPLE_RATE); // 60 seconds data

    // let's create reusable buffers - LATER
    // struct umxcpp::stft_buffers stft_buf(buffers.segment_samples);
    struct umxcpp::stft_buffers reusable_stft_buf(segment_samples);

    int nb_stft_frames_segment = (segment_samples / umxcpp::FFT_HOP_SIZE + 1);

    int lstm_hidden_size = model.hidden_size / 2;

    std::array<struct umxcpp::lstm_data, 4> streaming_lstm_data = {
        umxcpp::create_lstm_data(lstm_hidden_size, nb_stft_frames_segment),
        umxcpp::create_lstm_data(lstm_hidden_size, nb_stft_frames_segment),
        umxcpp::create_lstm_data(lstm_hidden_size, nb_stft_frames_segment),
        umxcpp::create_lstm_data(lstm_hidden_size, nb_stft_frames_segment) };


    int stride_samples = (int)((1 - umxcpp::OVERLAP) * segment_samples);

    int length = full_audio.cols();

    // create an output tensor of zeros for four source waveforms
    std::vector<Eigen::MatrixXf> out;
    out.push_back(Eigen::MatrixXf(2, length));
    out.push_back(Eigen::MatrixXf(2, length));
    out.push_back(Eigen::MatrixXf(2, length));
    out.push_back(Eigen::MatrixXf(2, length));

    for (int i = 0; i < 4; ++i)
    {
        out[i].setZero();
    }

    // create weight tensor
    Eigen::VectorXf weight(segment_samples);
    Eigen::VectorXf sum_weight(length);
    for (int i = 0; i < segment_samples / 2; ++i)
    {
        weight(i) = i + 1;
        weight(segment_samples - i - 1) = i + 1;
        sum_weight(i) = 0.0f;
    }
    weight /= weight.maxCoeff();
    weight = weight.array().pow(umxcpp::TRANSITION_POWER);

    // for loop from 0 to length with stride stride_samples
    for (int offset = 0; offset < length; offset += stride_samples)
    {
        // create a chunk of the padded_full_audio
        int chunk_end = std::min(segment_samples, length - offset);
        Eigen::MatrixXf chunk = full_audio.block(0, offset, 2, chunk_end);
        int chunk_length = chunk.cols();

        std::cout << "2., apply model w/ split, rate: " << (offset / (double)length) * 100.0
            << "%, chunk shape: (" << chunk.rows() << ", " << chunk.cols()
            << ")" << std::endl;

        // REPLACE THIS WITH UMX INFERENCE!
        std::vector<Eigen::MatrixXf> chunk_out =
            umx_inference(model, chunk, reusable_stft_buf, streaming_lstm_data);

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 2; ++j)
            {
                for (int k = 0; k < segment_samples; ++k)
                {
                    if (offset + k >= length)
                    {
                        break;
                    }
                    out[i](j, offset + k) +=
                        weight(k % chunk_length) * chunk_out[i](j, k);
                }
            }
        }

        for (int k = 0; k < segment_samples; ++k) {
            if (offset + k >= length) {
                break;
            }
            sum_weight(offset + k) += weight(k % chunk_length);
        }
    }

    assert(sum_weight.minCoeff() > 0);

    //��·����Ĺ�һ��
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < length; ++k)
            {
                out[i](j, k) /= sum_weight[k];
            }
        }
    }

    // now copy the appropriate segment of the output
    // into the output tensor same shape as the input
    std::vector<Eigen::MatrixXf> waveform_outputs;
    waveform_outputs.push_back(Eigen::MatrixXf(2, length));
    waveform_outputs.push_back(Eigen::MatrixXf(2, length));
    waveform_outputs.push_back(Eigen::MatrixXf(2, length));
    waveform_outputs.push_back(Eigen::MatrixXf(2, length));

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 2; ++j)
        {
            for (int k = 0; k < length; ++k)
            {
                waveform_outputs[i](j, k) = out[i](j, k);
            }
        }
    }
    return waveform_outputs;
}
