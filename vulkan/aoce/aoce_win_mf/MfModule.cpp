#include "MfModule.hpp"

#include <mfapi.h>

#include <aoce/AoceManager.hpp>
#include <aoce_win/WinHelp.hpp>

#include "videodevice/MFVideoManager.hpp"
#include <aoce/module/ModuleManager.hpp>
#pragma comment(lib,"mf.lib")
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfuuid.lib")
namespace aoce {
namespace win {
namespace mf {
    
MfModule::MfModule(/* args */) {}

MfModule::~MfModule() {}

bool MfModule::loadModule() {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {        
        hr = MFStartup(MF_VERSION);
    }
    logHResult(hr, "coInitialize");
    AoceManager::Get().addVideoManager(CameraType::win_mf,
                                       new MFVideoManager());
    return SUCCEEDED(hr);
}

void MfModule::unloadModule() {
    AoceManager::Get().removeVideoManager(CameraType::win_mf);
    HRESULT hr = MFShutdown();
    CoUninitialize();
}

ADD_MODULE(MfModule, aoce_win_mf)

}  // namespace mf
}  // namespace win
}  // namespace aoce
