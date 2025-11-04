#include"json.hpp"
#include <iostream>
#include <codecvt>

static std::wstring UTF8toUnicode(const std::string& strUTF8) {
    std::wstring_convert< std::codecvt_utf8<wchar_t>> wcv;
    return wcv.from_bytes(strUTF8);
}

static std::string  UnicodeToUTF8(const std::wstring& wstrUnicode) {
    std::wstring_convert< std::codecvt_utf8<wchar_t> > wcv;
    return wcv.to_bytes(wstrUnicode);
}

int main(){   
    //初始化JSON
    nlohmann::json jin;
    jin["happy"] = true;
    jin["pi"] = 3.14;
    jin["data"] = 419;
    jin["key"] = "987";//字符串
    const wchar_t* wsz = L"中国";
    std::string s1 = UnicodeToUTF8(wsz);
    jin["name"] = s1.c_str();//会变成数组！！
    std::string strJson = jin.dump();//JSON 保存为字符，可以写入文本
    const char* szJson = strJson.c_str();
    std::cout << szJson << std::endl;

    //解析字符串为JSON
    nlohmann::json  j2 = nlohmann::json::parse(szJson);
    std::string s2 = j2["name"];
    std::wstring ws = UTF8toUnicode(s2);//字符串转为UNICODE
    return 0;
}
