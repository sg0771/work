//
// Created by DefTruth on 2021/9/20.
//

#include <Windows.h>
#include <WinUser.h>
#include "resource.h"

#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <memory>
#include <MNN/Interpreter.hpp>
#include <MNN/ImageProcess.hpp>
#include <libyuv.h>

#pragma comment(lib,"mnn.lib")
//#pragma comment(lib,"WX_Image.lib")
// form DLL ---------------
class  MNN_Handler
{
public:
    MNN::Interpreter* mnn_interpreter = nullptr;
    MNN::Session* mnn_session = nullptr;
    MNN::CV::ImageProcess* mnn_pretreat = nullptr; // init at runtime
    MNN::Tensor* mnn_tensor = nullptr;

    const float mean_vals[3] = { 0.f, 0.f, 0.f }; // BGR
    const float norm_vals[3] = { 1.f / 255.f, 1.f / 255.f, 1.f / 255.f };
    unsigned int num_threads = 4; // initialize at runtime.

    int inpHeight = 0; //scale size
    int inpWidth = 0;
    std::shared_ptr<uint8_t> m_rsC4;//192x192  BGRA 8UC4 
    std::shared_ptr<uint8_t> m_rsC3;//192x192 BGR  8UC3
    std::shared_ptr<uint8_t> m_seg;//192x192
    bool m_bOpen = false;
public:
    static std::shared_ptr<MNN_Handler> s_pInst;
    void InitMnn() {
        HINSTANCE hDLL = GetModuleHandle(L"Humanseg.dll");
        HRSRC hResource = FindResource(hDLL, MAKEINTRESOURCE(IDR_MNN1), L"MNN");
        if (!hResource) return;
        DWORD ResourceSize = SizeofResource(hDLL, hResource);
        if (ResourceSize == 0) return;
        const void* pResourceData = LoadResource(hDLL, hResource);
        if (!pResourceData) return;

        // 1. init interpreter
        mnn_interpreter = MNN::Interpreter::createFromBuffer(pResourceData, ResourceSize);
        
        if (mnn_interpreter == nullptr)
            return;

        // 2. init schedule_config
        MNN::ScheduleConfig schedule_config;
        schedule_config.numThread = (int)num_threads;
        MNN::BackendConfig backend_config;
        backend_config.precision = MNN::BackendConfig::Precision_High; // default Precision_High
        schedule_config.backendConfig = &backend_config;

        schedule_config.type =  MNN_FORWARD_VULKAN;

        // 3. create session
        mnn_session = mnn_interpreter->createSession(schedule_config);
        if (mnn_session == nullptr)
            return;

        // 4. init input tensor
        mnn_tensor = mnn_interpreter->getSessionInput(mnn_session, "x"); //form paddlseg onnx
        if (mnn_tensor == nullptr)
            return;

        // 5. init input dims
        inpHeight = mnn_tensor->height();
        inpWidth = mnn_tensor->width();
        mnn_interpreter->resizeTensor(mnn_tensor, { 1, 3, inpHeight, inpWidth });//���������Size

        m_rsC4 = std::shared_ptr<uint8_t>(new uint8_t[this->inpHeight * this->inpWidth * 4]);
        m_rsC3 = std::shared_ptr<uint8_t>(new uint8_t[this->inpHeight * this->inpWidth * 3]);
        m_seg = std::shared_ptr<uint8_t>(new uint8_t[this->inpHeight * this->inpWidth]);
        // resize session
        mnn_interpreter->resizeSession(mnn_session);
        // init 0.
        mnn_pretreat = MNN::CV::ImageProcess::create(MNN::CV::BGR, MNN::CV::BGR, mean_vals, 3, norm_vals, 3);
        m_bOpen = true;
    }
public:
    bool IsOpen() {
        return m_bOpen;
    }
    static MNN_Handler* GetInst()
    {
        if (s_pInst == nullptr) {
            s_pInst = std::shared_ptr<MNN_Handler>(new MNN_Handler);
            s_pInst->InitMnn();
        }
        return s_pInst.get();
    }

    virtual ~MNN_Handler()
    {
        mnn_interpreter->releaseModel();
        if (mnn_session)
            mnn_interpreter->releaseSession(mnn_session);
    }
};
std::shared_ptr<MNN_Handler> MNN_Handler::s_pInst = nullptr;


class Humanseg
{
    int m_iWidth = 0;
    int m_iHeight = 0;
    std::shared_ptr<uint8_t> m_segDst;//m_iWidth x m_iHeight
public:
     bool Open(int width,
        int height) {
         if (!MNN_Handler::GetInst()->IsOpen()) {
             return false;
         }
        m_iWidth = width,
        m_iHeight = height,
        m_segDst = std::shared_ptr<uint8_t>(new uint8_t[this->m_iWidth * this->m_iHeight]);
        return true;
    }
    void Close(){

    }
    
public:

    void Detect(uint8_t* pBuf, int pitch)
    {
        libyuv::ARGBScale(pBuf, pitch,
            m_iWidth, m_iHeight,
            MNN_Handler::GetInst()->m_rsC4.get(), MNN_Handler::GetInst()->inpWidth * 4,
            MNN_Handler::GetInst()->inpWidth, MNN_Handler::GetInst()->inpHeight,
            libyuv::kFilterLinear);//Resize To 192x192 RGBA

        libyuv::ARGBToRGB24(
            MNN_Handler::GetInst()->m_rsC4.get(), MNN_Handler::GetInst()->inpWidth * 4,
            MNN_Handler::GetInst()->m_rsC3.get(), MNN_Handler::GetInst()->inpWidth * 3,
            MNN_Handler::GetInst()->inpWidth, MNN_Handler::GetInst()->inpHeight
        ); // Convert BGR

        // 1. make input tensor
        MNN_Handler::GetInst()->mnn_pretreat->convert(MNN_Handler::GetInst()->m_rsC3.get(), MNN_Handler::GetInst()->inpWidth, MNN_Handler::GetInst()->inpHeight, MNN_Handler::GetInst()->inpWidth * 3, MNN_Handler::GetInst()->mnn_tensor);

        // 2. inference & run session
        MNN_Handler::GetInst()->mnn_interpreter->runSession(MNN_Handler::GetInst()->mnn_session);

        auto output_tensors = MNN_Handler::GetInst()->mnn_interpreter->getSessionOutputAll(MNN_Handler::GetInst()->mnn_session);

        // 3. generate matting
        auto device_fgr_ptr = output_tensors.at("tf.identity");
        MNN::Tensor host_fgr_tensor(device_fgr_ptr, device_fgr_ptr->getDimensionType());  // NCHW
        device_fgr_ptr->copyToHostTensor(&host_fgr_tensor);

        float* fgr_ptr = host_fgr_tensor.host<float>();
        const unsigned int channel_step = MNN_Handler::GetInst()->inpHeight * MNN_Handler::GetInst()->inpWidth;

        float* mask_ptr = fgr_ptr;// fast assign & channel transpose(CHW->HWC).

        for (size_t i = 0; i < MNN_Handler::GetInst()->inpHeight * MNN_Handler::GetInst()->inpWidth; i++) {
            float pix = mask_ptr[i * 2  + 1];
            MNN_Handler::GetInst()->m_seg.get()[i] = uint8_t(pix * 255.0f);
        }
        libyuv::ScalePlane(
            MNN_Handler::GetInst()->m_seg.get(), MNN_Handler::GetInst()->inpWidth, MNN_Handler::GetInst()->inpWidth, MNN_Handler::GetInst()->inpHeight,
            m_segDst.get(), m_iWidth, m_iWidth, m_iHeight,
            libyuv::kFilterLinear
        );

        for (size_t h = 0; h < m_iHeight; h++) {
            for (size_t w = 0; w < m_iWidth; w++) {
                int gray = m_segDst.get()[h * m_iWidth + w] * 2;
                if (gray > 255)
                    gray = 255;
                //if (gray > 50) {
                //    pBuf[h * pitch + w * 4 + 3] = 255;
                //}
                //else {
                //    pBuf[h * pitch + w * 4 + 0] = 0;
                //    pBuf[h * pitch + w * 4 + 1] = 0;
                //    pBuf[h * pitch + w * 4 + 2] = 0;
                //    pBuf[h * pitch + w * 4 + 3] = 0;
                //}
                pBuf[h * pitch + w * 4 + 3] = gray;
            }
        }
    }

};


//-------------------------------------------------------------------------------------------------------
EXTERN_C __declspec(dllexport) void*   HumansegCreate(int width, int height) {
    Humanseg* obj = new Humanseg;
    bool bOpen = obj->Open(width, height);
    if (!bOpen) {
        delete obj;
        return nullptr;
    }
    return obj;
}

EXTERN_C __declspec(dllexport) int     HumansegDetect(void* ptr, uint8_t* pData, int pitch) {
    if (ptr) {
        Humanseg* obj = (Humanseg*)ptr;
        obj->Detect(pData,pitch);
        return 1;
    }
    return 0;
}

EXTERN_C __declspec(dllexport) void    HumansegDestroy(void* ptr) {
    if (ptr) {
        Humanseg* obj = (Humanseg*)ptr;
        delete obj;
    }
}


EXTERN_C MNN_PUBLIC  int MNNSupportVulkan();


EXTERN_C __declspec(dllexport) int WXSupportVulkan() {
    return MNNSupportVulkan();
}
