#pragma once

#include "sktbdpch.h"

#include "Skateboard/Graphics/RHI/GraphicsContext.h"
#include "Skateboard/Platform.h"

#include "API/D3DDescriptorHeap.h"
#include "API/UploadManager.h"

#include "D3DRenderCommand.h"
#include "D3DResourceFactory.h"

#ifndef SKTBD_DESKTOP_PLATFORM_MIN_BUFFER_ALIGNMENT
#define SKTBD_DESKTOP_PLATFORM_MIN_BUFFER_ALIGNMENT (64*1024)
#endif // !SKTBD_DESKTOP_PLATFORM_MIN_BUFFER_ALIGNMENT

#define D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED
#define D3D12MA_OPTIONS16_SUPPORTED 1

#include "Windows/WindowsPlatform/DirectX12/Graphics/D3DTypes.h"
#include "Windows/WindowsPlatform/DirectX12/vendor/D3D12MemoryAllocator/include/D3D12MemAlloc.h"

using namespace Microsoft::WRL;

#define D3D_DEVICE_REMOVED_EXTENDED_DATA_ENABLE_FLAG 0b1

namespace Skateboard::D3D
{
	//easy access d3dContext
	extern D3DGraphicsContext* gD3DContext;

	class D3DGraphicsContext final : public GraphicsContext
	{
		friend class UploadManager;
		friend class D3DRenderCommand;
		friend class D3DResourceFactory;
		//friend class D3DDescriptorHeap;
		friend class D3DDebugTools;

	public:
		//Flags
		uint32_t m_Flags = D3D_DEVICE_REMOVED_EXTENDED_DATA_ENABLE_FLAG;

		// Let's not make things confusing and initialise everything in the constructor
		D3DGraphicsContext(HWND window);
		// And release all in destructor
		virtual ~D3DGraphicsContext() final override;

	protected:
		RenderCommand* GetAPI() final override { return &D3D_API; };
		ResourceFactory* GetResourceFactory() final override { return &D3D_RESOURCE_FACTORY; };

		//---------------------------------------OVERRIDES

		virtual SKTBDR Init(const GraphicsContextDescription& desc, std::unique_ptr<TimeManager> timer) override;

		// Public functions to resize the buffers according to the new dimensions stored in lParam based on the size description in wParam
		void Resize_(uint32_t clientWidth, uint32_t clientHeight, bool Fullscreen) final override;
		//void OnResized_() final override;

		virtual void Update_() final override;

		virtual void BeginFrame_() final override;
		virtual void EndFrame_() final override;

		virtual void WaitUntilIdle_() final override;

		virtual bool IsRaytracingSupported_() final override { return m_HasDXR; }
		virtual bool AreWorkGraphsSupported_() final override { return false; }
		virtual bool IsUnifiedMemoryArchitecture_() final override { return p_MemoryAllocator->IsUMA(); }

		virtual CopyResult CopyDataToBuffer_(Buffer* dest, off_t offset, size_t size, void* src)final override;
		virtual CopyResult CopyDataToBuffer_(Buffer* dest, off_t offset, size_t size, std::function<void(void*)> WriterFunct) final override;

		void SubmitCompute_(const ComputeSubmitInfo& submit) override;
		void SubmitGraphics_(const GraphicsSubmitInfo& submit) override;

		void GraphicsSignalFence_(Fence* fence, uint64_t value) override;
		void ComputeSignalFence_(Fence* fence, uint64_t value) override;

		void GraphicsWaitFence_(Fence* fence, uint64_t value) override;
		void ComputeWaitFence_(Fence* fence, uint64_t value) override;

		uint32_t GetCurrentFrameResourceIndex_() override { return m_frameIndex; };

		// Inherited via GraphicsContext
		ShaderIdentifier GetShaderIdentifier(const Pipeline* Pipeline, const std::wstring& ShaderName) override;
		CopyResult WriteTopLevelASInstanceDataToBuffer_(Buffer* dest, off_t offset, size_t num, TLASInstanceData* data) override;
		RaytracingASSizeInfo QueryAccelerationStructureSizeReq_(const AccelerationStructureDesc& AS_Desc, SKTBD_ACCELERATION_STRUCT_BUILD_FLAGS Flags) override;
		ShaderTable BuildShaderTable_(const BufferRef& Target, off_t offset, ComputePipeline* pso, const RaytracingPipelineDesc& m_desc, ShaderTableGroup Group) override;

		//---------------------------------------------------END OVERRIDES

	

	public:
		IDXGISwapChain* GetSwapChain() const { return m_SwapChain.Get(); }

		DXGI_FORMAT GetDXGIBackBufferFormat() const { return SkateboardBufferFormatToD3D(m_BackBufferFormat); }

		D3DDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV> GetImGuiDescriptorHandle() const { return m_ImGuiHandle; }

		ID3D12GraphicsCommandList10* GetD3DDefaultFrameCommandList() const;
		ID3D12CommandAllocator* GetD3DDefaultFrameCommandAllocator() const;

		ID3D12Device14* GetDevice() const { return m_Device.Get(); }
		DXGI_ADAPTER_DESC1 GetAdapterDesc() const { return AdapterDesc; }
		HANDLE GetDeviceRemovedEventHANDLE() const { return m_DeviceRemovedEvent; };
		auto GetDxcUtils() const { return m_Utils.Get(); }

		ID3D12CommandQueue* GetD3DGraphicsCommandQueue() const { return m_GraphicsCommandQueue.Get(); }
		ID3D12CommandQueue* GetD3DComputeCommandQueue() const { return m_ComputeCommandQueue.Get(); }

		ID3D12CommandQueue* GetD3DCopyCommandQueue() const { return m_CopyCommandQueue.Get(); }

		D3D_SHADER_VIEW_HEAP &GetSRVDescriptorHeap() { return m_SRVDescriptorHeap; }
		D3D_RTV_HEAP &GetRTVDescriptorHeap() { return m_RTVDescriptorHeap; }
		D3D_DSV_HEAP &GetDSVDescriptorHeap() { return m_DSVDescriptorHeap; }
		D3D_SAMPLER_HEAP &GetSamplerDescriptorHeap() { return m_SamplerHeap; }

		_NODISCARD ID3D12Resource* GetCurrentD3DBackBuffer() { return m_SwapChainBuffers[m_frameIndex].Get(); }
		_NODISCARD const ID3D12Resource* GetCurrentD3DBackBuffer() const { return m_SwapChainBuffers[m_frameIndex].Get(); }

		_NODISCARD ID3D12Fence* GetFence() { return m_Fence.Get(); }
		_NODISCARD const ID3D12Fence* GetFence() const { return m_Fence.Get(); }
		
		// We need to be able to access the descriptors stored in the respective heaps of our buffer
		const D3D12_CPU_DESCRIPTOR_HANDLE* GetD3DCurrentBackBufferRTVHandle() const;
		const D3D12_CPU_DESCRIPTOR_HANDLE* GetD3DDepthStencilHandle() const;

		void SetDeferredReleasesFlag();

		void DeferredRelease(IUnknown* resource);

		void ProcessDeferrals();

		D3D12MA::Allocator* GetMemoryAllocator() const { return p_MemoryAllocator; }

	private:
		bool CheckDeviceRemovedStatus() const;

		void CreateDevice();
		void CreateFenceAndEventHandle();
		void CreateDescriptorSizes();
		void Check4xMSAAQualitySupport();
		void CreateCommandQueueAndCommandList();
		void CreateSwapChain();

		void CreateRenderTargetViews();
		void CreateDepthStencilBuffer();

		void CreateDescriptorHeaps();

		void CreateUploadManager();

		void MoveToNextFrame();

	private:
		// Window settings, mostly in regards with resizing
		bool	m_Vsync;								// Note: Fullscreen in this framework is handled by the platform API

		// MSAA support (not used in this project, but could be enabled)
		bool m_MSAAEnable;
		UINT m_MSAAQuality;

		// Working environment
		HWND m_MainWindow;

		//DXC GetDxcUtils for shader reflection logic
		Microsoft::WRL::ComPtr<IDxcUtils> m_Utils;

		//upload manager for uploading buffers
		UploadManager m_UploadManager;

		// COM Objects
		// GetDevice and DXGI Factory
		DXGI_ADAPTER_DESC1 AdapterDesc;
		Microsoft::WRL::ComPtr<ID3D12Device14>				m_Device;						// The device is the display adapter, like the graphics card
		HANDLE m_DeviceRemovedEvent;


		DWORD												m_MessageCallbackCookie;
		ComPtr<ID3D12InfoQueue1>							m_InfoQueue;

		Microsoft::WRL::ComPtr<IDXGIFactory7>				m_DXGIInterface;				// The interface to generate any DXGI objects (version 4 provides more functionalities, such as EnumWarpAdapters)

		//Memory Allocator // INITIALISED IN THE CREATE DEVICE
		D3D12MA::Allocator*									p_MemoryAllocator;

		// Fence
		Microsoft::WRL::ComPtr<ID3D12Fence>					m_Fence;						// The fence object to synchronise the CPU/GPU

		UINT m_frameIndex;
		HANDLE m_fenceEvent;
	
		std::array<uint64_t, GRAPHICS_SETTINGS_NUMFRAMERESOURCES> a_FenceValues;			// Identify a fence point in time. Everytime we mark a new fence point, increment this integer

		// Command Objects
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_GraphicsCommandQueue;					// The command queue we will use for this application to submit commands to the GPU
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_ComputeCommandQueue;					// The command queue we will use for this application to submit commands to the GPU
		Microsoft::WRL::ComPtr<ID3D12CommandQueue>			m_CopyCommandQueue;					// The command queue we will use for this application to submit commands to the GPU

		// GetSwapChain
		Microsoft::WRL::ComPtr<IDXGISwapChain4>				m_SwapChain;

		ComPtr<ID3D12Resource>								m_SwapChainBuffers[GRAPHICS_SETTINGS_NUMFRAMERESOURCES];	// The back buffers to use in the swapchain

		// Descriptor sizes
		UINT m_RTVDescriptorSize;
		UINT m_DSVDescriptorSize;
		UINT m_CBVSRVUAVDescriptorSize;

		// Bools
		bool	m_HasDXR;			// A bool that will be checked on init to initialise raytracing components
		bool	m_HasWorkGraphs;
		bool	m_ClientResized;

		std::vector<IUnknown*> m_DeferredReleases[GRAPHICS_SETTINGS_NUMFRAMERESOURCES]{};
		UINT32 m_DeferredFlags[GRAPHICS_SETTINGS_NUMFRAMERESOURCES];
		std::mutex m_DeferredMutex;

		D3D_SHADER_VIEW_HEAP m_RWSRVDescriptorHeap;		// Read-Write
		D3D_SHADER_VIEW_HEAP m_SRVDescriptorHeap;			// Write only as shader visible

		D3D_RTV_HEAP m_RTVDescriptorHeap;
		D3D_DSV_HEAP m_DSVDescriptorHeap;

		D3D_SAMPLER_HEAP m_SamplerHeap;

		//Imgui
		SHADER_VIEW_HANDLE m_ImGuiHandle;

		//RenderAPI
		D3DRenderCommand D3D_API;
		D3DResourceFactory D3D_RESOURCE_FACTORY;

	};
}
