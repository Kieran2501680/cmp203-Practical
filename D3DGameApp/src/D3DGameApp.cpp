
#include <Skateboard.h>
#include "Skateboard/EntryPoint.h"

#include "Windows/WindowsPlatform/DirectX12/Graphics/RHI/D3DGraphicsContext.h"
#include "nvapi-main/nvapi.h"
#include "nvapi-main/nvapi_interface.h"

#include "DefaultGameLayer.h"

#define SKTBD_LOG_COMPONENT "D3DGameApp"

static void OnDeviceRemoved(PVOID context, BOOLEAN)
{
    ID3D12Device5* removedDevice = (ID3D12Device5*)context;

    NvAPI_D3D12_FlushRaytracingValidationMessages(removedDevice);
} 

static void __stdcall myValidationMessageCallback(void* pUserData, NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY severity, const char* messageCode, const char* message, const char* messageDetails)
{
    Skateboard::LogSeverity Severity;
    switch (severity)
    { 
    case NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY_ERROR: Severity = Skateboard::LogSeverity::Error; break;
    case NVAPI_D3D12_RAYTRACING_VALIDATION_MESSAGE_SEVERITY_WARNING: Severity = Skateboard::LogSeverity::Warn; break;
    }
    SKTBD_LOG(Severity,"NvAPI" ,"Ray Tracing Validation message: {0}: [{1}] {2} \n {3}", messageCode, message, messageDetails);
}

class D3DGameApp : public Skateboard::Application
{ 
public:
	D3DGameApp(const Skateboard::PlatformDescription& desc) : Application(desc)
	{
        ID3D12Device5* device = Skateboard::D3D::gD3DContext->GetDevice();

		auto adapterinfo = Skateboard::D3D::gD3DContext->GetAdapterDesc();

#ifndef SKTBD_SHIP
        if (adapterinfo.VendorId == 0x10DE) // NVDA
        {
            SKTBD_MSG_INFO("Initializing NVIDIA API for Further Debugging")
            NvAPI_Initialize();

            NvAPI_D3D12_EnableRaytracingValidation(device, NVAPI_D3D12_RAYTRACING_VALIDATION_FLAG_NONE);

            static void* nvapiValidationCallbackHandle;

            NvAPI_D3D12_RegisterRaytracingValidationMessageCallback(device, &myValidationMessageCallback, nullptr, &nvapiValidationCallbackHandle);

            HANDLE waitHandle;
            RegisterWaitForSingleObject(
                &waitHandle,
                Skateboard::D3D::gD3DContext->GetDeviceRemovedEventHANDLE(),
                OnDeviceRemoved,
                device,     // Pass the device as our context
                INFINITE,    // No timeout
                0 // No flags
            );
        }
#endif 

		PushLayer(new DefaultGameLayer());
	}
};

Skateboard::Application* Skateboard::CreateApplication(int argc, char** argv)
{
    PlatformDescription description{};

    description.GraphicsContextInitDesc = GraphicsContextDescription::Default();
    description.Title = L"D3D Examples using Skateboard Engine Backend";
    description.WindowSize = { 1280, 720 };
    description.Flags = PlatformDescriptionFlags_None;
    description.GraphicsContextInitDesc.FrameSyncWaitMode = FrameTimeWaitMode::VSync;
    //description.GraphicsContextInitDesc.FrameSyncWaitMode = FrameTimeWaitMode::Sleep;
    //description.GraphicsContextInitDesc.FrameSyncWaitMode = FrameTimeWaitMode::Spin;
    description.GraphicsContextInitDesc.FrameTimeTargetMS = 16.67f;

	return new D3DGameApp(description);
}