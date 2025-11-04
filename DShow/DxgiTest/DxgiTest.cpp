// DxgiTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_6.h>
#include <d3d11.h>
#include <atlbase.h>
#include <Windows.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")


static void HrToBuffer(HRESULT hr, WCHAR* buffer, int length) {
	memset(buffer, 0, length * sizeof(WCHAR));
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		buffer, length, nullptr);
}

class DXGI_Info {
public:
	CComPtr<IDXGIFactory1>m_pFactory1 = nullptr;

	DXGI_Info() {
		HRESULT hr = S_OK;
		hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)(&m_pFactory1));
		if (FAILED(hr)) {
			wprintf(L"%ws CreateDXGIFactory1 Error", __FUNCTIONW__);
			m_pFactory1 = nullptr;
			return;
		}
	}

	int ListOutput1() {

		std::wcout << __FUNCTIONW__ << std::endl;
		// 参数定义  
		int nDxgiIndex = -1;
		int iGpuIndex = 0; // 显卡的数量 
		while (TRUE) {
			CComPtr<IDXGIAdapter1>pAdapter = nullptr;
			HRESULT hr = m_pFactory1->EnumAdapters1(iGpuIndex, &pAdapter); //枚举显卡
			if (FAILED(hr)) {
				break;
			}

			// Driver types supported
			const UINT NumDriverTypes = 4;
			D3D_DRIVER_TYPE DriverTypes[4] = {
				D3D_DRIVER_TYPE_UNKNOWN,
				D3D_DRIVER_TYPE_HARDWARE,
				D3D_DRIVER_TYPE_WARP,
				D3D_DRIVER_TYPE_REFERENCE,
			};

			UINT NumFeatureLevels = 4;
			D3D_FEATURE_LEVEL FeatureLevels[4] = {
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1,
				D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_1
			};

			CComPtr<ID3D11Device>m_pDev = nullptr;
			CComPtr<ID3D11DeviceContext>m_pContext = nullptr;
			D3D_FEATURE_LEVEL FeatureLevel;
				//获取D3D11 设备
			for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex) {
				hr = D3D11CreateDevice(pAdapter, DriverTypes[DriverTypeIndex], nullptr,
						0, FeatureLevels, NumFeatureLevels,
						D3D11_SDK_VERSION, &m_pDev, &FeatureLevel, &m_pContext);
				if (SUCCEEDED(hr)) {
					wprintf(L"%ws DriverTypeIndex = %d \r\n", __FUNCTIONW__, (int)DriverTypeIndex);
					break;
				}
			}
			if (m_pDev == nullptr) {
				wprintf(L"%ws D3D11CreateDevice Error \r\n", __FUNCTIONW__);
				break;
			}
			// 获取信息  
			DXGI_ADAPTER_DESC desc;
			hr = pAdapter->GetDesc(&desc);
			/* ignore Microsoft's 'basic' renderer' */
			if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c) {
				iGpuIndex++;
				continue;
			}
			UINT iOutputIndex = 0;
			while (true) { //枚举显卡上对应的显示器
				CComPtr<IDXGIOutput>Output = nullptr;
				hr = pAdapter->EnumOutputs(iOutputIndex, &Output);// Get output
				if (SUCCEEDED(hr)) {
					//OBS 代码上还有 IDXGIOutput5 IDXGIOutput6
					//其中 IDXGIOutput6是HDR相关的
					//而且 IDXGIOutput5 IDXGIOutput6 只能在Win10 及以上系统上使用
					CComPtr<IDXGIOutput1>Output1 = nullptr;
					hr = Output->QueryInterface(__uuidof(Output1), reinterpret_cast<void**>(&Output1));// QI for Output 1
					if (SUCCEEDED(hr)) {
						CComPtr <IDXGIOutputDuplication>pDuplication = nullptr;
						hr = Output1->DuplicateOutput(m_pDev, &pDuplication);//桌面采集对象
						if (SUCCEEDED(hr)) {
							nDxgiIndex = iGpuIndex;
							wprintf(L"%ws [%d] [%ws] [%d] Support DXGI\r\n",
								__FUNCTIONW__, (int)iGpuIndex, desc.Description, iOutputIndex);
						}
						else {
							WCHAR BufferError[256];
							HrToBuffer(hr, BufferError,256);
							wprintf(L"%ws [%d] [%ws] [%d] Not Support DXGI ERROR=%ws \r\n",
								__FUNCTIONW__, (int)iGpuIndex, desc.Description, iOutputIndex, BufferError);
						}
					}
				}
				else {
					//没有获得有效设备
					WCHAR BufferError[256];
					HrToBuffer(hr, BufferError, 256);
					wprintf(L"%ws [%d] [%ws] [%d] EnumOutputs ERROR=%ws\r\n",
						__FUNCTIONW__, (int)iGpuIndex, desc.Description, iOutputIndex, BufferError);
					break;
				}
				iOutputIndex++;
			}
			iGpuIndex++;
		}
		return nDxgiIndex;
	}

};


int main()
{
    setlocale(LC_CTYPE, "");

	DXGI_Info gpu;
	gpu.ListOutput1();
	system("pause");
	return 0;
}


