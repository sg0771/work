#include "../demucs/demucs.h"
#pragma comment(lib, "demucs.onnx.lib")
#include <string>
#include <Windows.h>

static std::wstring GBToUTF16(std::string strASNI) {
    int len = ::MultiByteToWideChar(CP_ACP, 0, strASNI.c_str(), -1, NULL, 0);
    if (len <= 0)
        return L"";
    std::wstring wstr(len, 0);
    ::MultiByteToWideChar(CP_ACP, 0, strASNI.c_str(), -1, &wstr[0], len);
    return wstr;
}

int main(int argc, const char **argv)
{
	std::string model_file = "";//模型
	std::string input_file = "";//输入文件

    std::string output_drum_file = "";//输出鼓点 -d --drum  
    std::string output_bass_file = "";//输出贝斯 -b --bass  
    std::string output_accp_file = "";//输出伴奏  -a --accp 
    std::string output_vocal_file = "";//输出人声 -v --vocal  

    for (int i = 1; i < argc; i += 2) {
        const char* strParam = argv[i];
        const char* strValue = argv[i + 1];
        if (stricmp(strParam, "-m") == 0 || stricmp(strParam, "--model") == 0) {
            model_file = strValue;
        }
        if (stricmp(strParam, "-i") == 0 || stricmp(strParam, "--input") == 0) {
            input_file = strValue;
        }  
        if (stricmp(strParam, "-v") == 0 || stricmp(strParam, "--vocal") == 0) {
            output_vocal_file = strValue;
        }
        if (stricmp(strParam, "-a") == 0 || stricmp(strParam, "--accp") == 0) {
            output_accp_file = strValue;
        }
        if (stricmp(strParam, "-d") == 0 || stricmp(strParam, "--drum") == 0) {
            output_drum_file = strValue;
        }
        if (stricmp(strParam, "-b") == 0 || stricmp(strParam, "--bass") == 0) {
            output_bass_file = strValue;
        }
    }
    std::wstring wstr_model_file = GBToUTF16(model_file);
    std::wstring wstr_input_file = GBToUTF16(input_file);
    std::wstring wstr_output_drum_file = GBToUTF16(output_drum_file);
    std::wstring wstr_output_bass_file = GBToUTF16(output_bass_file);
    std::wstring wstr_output_accp_file = GBToUTF16(output_accp_file);
    std::wstring wstr_output_vocal_file = GBToUTF16(output_vocal_file);
    WXDemucsProcess(wstr_input_file.c_str(),
        wstr_model_file.c_str(),
        wstr_output_drum_file.c_str(),
        wstr_output_bass_file.c_str(),
        wstr_output_accp_file.c_str(),
        wstr_output_vocal_file.c_str());

    return 0;
}
