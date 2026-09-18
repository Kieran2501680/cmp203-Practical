#pragma once

#include "Skateboard/Mathematics.h"
#include "Skateboard/Graphics/Resources/CommonResources.h"
#include "Skateboard/Graphics/RHI/GraphicsSettingsDefines.h"
#include "Skateboard/Time/TimeManager.h"

#include "sktbdpch.h"

namespace Skateboard
{
	struct CopyResult
	{
		uint64_t m_CompletionValue;
		FenceRef m_CompletionFence;
		std::future<void> m_CompletionFuture;
	};

	using ShaderIdentifier = uint64_t;

	struct ShaderTableRecord
	{
		ShaderIdentifier m_identifier;
		uint64_t m_userData;
	};

	class RenderCommand;
	class ResourceFactory;

	//Submit structure for the graphics and compute work
	struct GraphicsSubmitInfo
	{
		CommandBuffer** CommandBuffers;
		uint32_t BufferCount;
	};

	struct ComputeSubmitInfo
	{
		ComputeCommandBuffer** CommandBuffers;
		uint32_t BufferCount;
	};

	enum GraphicsContextInitFlags_ : uint8_t
	{
		GraphicsContextInitFlags_None = 0,
		GraphicsContextInitFlags_EnableMSAABackBuffer = 1,
		GraphicsContextInitFlags_ForceSoftwareRendering = 1 << 1,
		GraphicsContextInitFlags_InitFullScreen = 1 << 2,
		GraphicsConetxtIntiFlags_InitWithPlatformResolution = 1 << 3,
		GraphicsContextInitFlags_DoNotClearBackBufferOnBeginFrame = 1 << 4,
		GraphicsContextInitFlags_Headless = 1 << 5,
	};
	ENUM_FLAG_OPERATORS(GraphicsContextInitFlags_);

	enum FrameTimeWaitMode :uint8_t
	{
		None = 0,
		VSync = 1,
		Sleep = 1 << 2,
		Spin = 1 << 3
	};

	struct GraphicsContextDescription
	{ 
		static GraphicsContextDescription Default()
		{
			GraphicsContextDescription ret;
			 
			ret.StagingCopyBufferSize = 2 * 1024 * 1024; //16MB
			ret.BufferedFrameCount = 3;
			ret.BackBufferResolution = {0,0};
			ret.DefaultZNear = 0;
			ret.DefaultZFar = 1;
			ret.DefaultClearColour = float4(0.1f, 0.1f, 0.1f, 1.f);
			ret.DefaultBackBufferFormat = DataFormat_R8G8B8A8_UNORM_SRGB;
			ret.Flags = GraphicsConetxtIntiFlags_InitWithPlatformResolution;
			ret.FrameSyncWaitMode = FrameTimeWaitMode::None;
			ret.FrameTimeTargetMS = 16.67f; //60fps

			return ret;
		}

		uint32_t StagingCopyBufferSize;
		uint32_t BufferedFrameCount;
		uint2 BackBufferResolution;
		float DefaultZNear;
		float DefaultZFar;
		float4 DefaultClearColour;
		DataFormat_ DefaultBackBufferFormat;
		GraphicsContextInitFlags_ Flags; 
		float FrameTimeTargetMS;
		FrameTimeWaitMode FrameSyncWaitMode;
	};

	class GraphicsContext
	{
		friend class RenderCommand;
		friend class ResourceFactory;
		friend class Application;

	public:
		GraphicsContext()
		:	m_ClientWidth(0),
			m_ClientHeight(0),
			m_BackBufferFormat(DataFormat_UNKNOWN),
			m_BackBufferViewport(),
			m_BackBufferScissor(),
			m_numberOfFrameResources(0),
			m_FrameTimeTargetMS(0.f),
			b_bClearBackBuffer(false),
			b_IsFullScreen(false),
			b_Headless(false)
		{
			ASSERT_SIMPLE(!Context);
			Context = this;
		}

		virtual ~GraphicsContext() {
			Context = nullptr;
		}

		virtual SKTBDR Init(const GraphicsContextDescription& desc, std::unique_ptr<TimeManager> GraphicsContextTimer)
		{
			m_numberOfFrameResources = desc.BufferedFrameCount;
			b_Headless = desc.Flags & GraphicsContextInitFlags_Headless;

			m_GraphicsContextTimer = std::move(GraphicsContextTimer);
			m_FrameTimeTargetMS = desc.FrameTimeTargetMS;
			m_FrameTimeWaitMode = desc.FrameSyncWaitMode;

			if (!b_Headless)
			{
				b_IsFullScreen = desc.Flags & GraphicsContextInitFlags_InitFullScreen;
				m_BackBufferFormat = desc.DefaultBackBufferFormat;
				m_ClearColour = desc.DefaultClearColour;
				m_ClientHeight = desc.BackBufferResolution.y;
				m_ClientWidth = desc.BackBufferResolution.x;
				m_NewHeight = m_ClientHeight;
				m_NewWidth = m_ClientWidth;
				m_BackBufferViewport.MinZ = desc.DefaultZNear;
				m_BackBufferViewport.MaxZ = desc.DefaultZFar;
				b_bClearBackBuffer = !(desc.Flags & GraphicsContextInitFlags_DoNotClearBackBufferOnBeginFrame);
			}

			return OKEYDOKEY;
		}

		/// <summary>
		/// Static Interface Of Skateboard Engine Graphics Context
		/// </summary> 

		static Viewport GetBackBufferViewport() { return  Context->m_BackBufferViewport; };
		static Rect     GetBackBufferScissor()  { return  Context->m_BackBufferScissor; };

		static void Resize(uint32_t clientWidth, uint32_t clientHeight, bool fullscreen) { Context->Resize_(clientWidth, clientHeight, fullscreen);}

		static float GetClientAspectRatio()	{ return static_cast<float>(Context->m_ClientWidth) / Context->m_ClientHeight; }

		static void WaitUntilIdle() { Context->WaitUntilIdle_(); }

		static void Update() { Context->Update_(); }

		static void BeginFrame() { Context->BeginFrame_(); }
		static void EndFrame() { Context->EndFrame_(); }

		static bool IsRaytracingSupported() { return Context->IsRaytracingSupported_(); }
		static bool AreWorkGraphsSupported() { return Context->AreWorkGraphsSupported_(); }
		static bool IsUnifiedMemoryArchitecture() { return Context->IsUnifiedMemoryArchitecture_(); }

		static void SubmitGraphics(const GraphicsSubmitInfo& submit) { Context->SubmitGraphics_(submit);}
		static void SubmitCompute(const ComputeSubmitInfo& submit)   { Context->SubmitCompute_(submit); }

		static void GraphicsSignalFence(Fence* fence, uint64_t value){ Context->GraphicsSignalFence_(fence, value); }
		static void ComputeSignalFence(Fence* fence, uint64_t value) { Context->ComputeSignalFence_(fence, value); }

		static void GraphicsWaitFence(Fence* fence, uint64_t value)  { Context->GraphicsWaitFence_(fence, value); }
		static void ComputeWaitFence(Fence* fence, uint64_t value)   { Context->ComputeWaitFence_(fence, value); }

		static CopyResult CopyDataToBuffer(Buffer* dest, off_t offset, size_t size, void* src) { return Context->CopyDataToBuffer_(dest, offset, size, src); };
		static CopyResult CopyDataToBuffer(Buffer* dest, off_t offset, size_t size, std::function<void(void*)> WriterFunct) { return Context->CopyDataToBuffer_(dest, offset, size, WriterFunct); }

		//Raytracing
		static CopyResult WriteTopLevelASInstanceDataToBuffer(Buffer* dest, off_t offset, size_t num, TLASInstanceData* data) { return Context->WriteTopLevelASInstanceDataToBuffer_(dest, offset, num, data); }
		static RaytracingASSizeInfo QueryAccelerationStructureSizeReq(const AccelerationStructureDesc& AS_Desc, SKTBD_ACCELERATION_STRUCT_BUILD_FLAGS Flags) { return Context->QueryAccelerationStructureSizeReq_(AS_Desc, Flags); };
		static ShaderTable BuildShaderTable(const BufferRef& Target, off_t offset, ComputePipeline* pso, const RaytracingPipelineDesc& m_desc, ShaderTableGroup Group) { return Context->BuildShaderTable_(Target, offset, pso, m_desc, Group); };

		//Access to Variables

		static uint32_t GetClientWidth()  { return Context->m_ClientWidth; }
		static uint32_t GetClientHeight() { return Context->m_ClientHeight; }
		static DataFormat_ GetBackBufferDataFormat() { return Context->m_BackBufferFormat; }
		static bool    IsFullscreen()     { return Context->b_IsFullScreen; }

		static uint64_t GetCurrentFrameResourceIndex() { return Context->GetCurrentFrameResourceIndex_(); }
		static GraphicsCommandBuffer* GetDefaultCommandBuffer() { return Context->m_DefaultGraphicsCB.Get().get(); }

		static RenderTargetView* GetCurrentBackBufferRTV() { return Context->m_SwapChainRTVs.Get().get(); }

		static DepthStencilView* GetDefaultDepthBuffer() { return Context->m_DefaultDSV.get(); }

		static void SetBackBufferClearColour(float4 nClearColour) { Context->m_ClearColour = nClearColour; }
		static void SetClearBackBuffer(bool ClearBackBuffer) { Context->b_bClearBackBuffer = ClearBackBuffer; }

		/// <summary>
		/// Virtual Interface of Skateboard Engine Graphics Context
		/// </summary>
	protected:
		virtual void Resize_(uint32_t clientWidth, uint32_t clientHeight, bool Fullscreen) {}

		virtual void WaitUntilIdle_() {}

		virtual void Update_() {};

		virtual void BeginFrame_() {}
		virtual void EndFrame_() {}

		virtual bool IsRaytracingSupported_() = 0;
		virtual bool AreWorkGraphsSupported_() = 0;
		virtual bool IsUnifiedMemoryArchitecture_() = 0;

		virtual void SetFrameTimeWaitMode_(FrameTimeWaitMode waitMode) { m_FrameTimeWaitMode = waitMode; };
		virtual void SetFrameTimeTarget_(float target) { m_FrameTimeTargetMS = target; };

		//Work submission and synchronisation
		virtual void SubmitCompute_(const ComputeSubmitInfo& submit) = 0;
		virtual void SubmitGraphics_(const GraphicsSubmitInfo& submit) = 0; 

		virtual void GraphicsSignalFence_(Fence* fence, uint64_t value) = 0;
		virtual void ComputeSignalFence_(Fence* fence, uint64_t value) = 0;

		virtual void GraphicsWaitFence_(Fence* fence, uint64_t value) = 0;
		virtual void ComputeWaitFence_(Fence* fence, uint64_t value) = 0;

		//Data Shuffling
		virtual CopyResult CopyDataToBuffer_(Buffer* dest, off_t offset, size_t size, void* src) = 0;
		virtual CopyResult CopyDataToBuffer_(Buffer* dest, off_t offset, size_t size, std::function<void(void*)> WriterFunct) = 0;

		//Raytracing Data
		virtual CopyResult WriteTopLevelASInstanceDataToBuffer_(Buffer* dest, off_t offset, size_t num, TLASInstanceData* data) = 0;

		virtual uint32_t GetCurrentFrameResourceIndex_() = 0;

		virtual ShaderIdentifier GetShaderIdentifier(const Pipeline* Pipeline, const std::wstring& ShaderName) = 0;

		virtual ShaderTable BuildShaderTable_(const BufferRef& Target, off_t offset, ComputePipeline* pso, const RaytracingPipelineDesc& m_desc, ShaderTableGroup Group) = 0;
		virtual RaytracingASSizeInfo QueryAccelerationStructureSizeReq_(const AccelerationStructureDesc& AS_Desc, SKTBD_ACCELERATION_STRUCT_BUILD_FLAGS Flags) = 0;

	protected:
		virtual RenderCommand* GetAPI() = 0;
		virtual ResourceFactory* GetResourceFactory() = 0;

	public:
		//context is accessible directly from this class
		static inline GraphicsContext* Context = nullptr;
	protected:
		uint32_t m_ClientWidth, m_ClientHeight;		// Width and height of the client area (does not include the top bar and menus, this is the drawable surface)
		uint32_t m_NewWidth, m_NewHeight;		// New Width and height of the client area used temporarily when window is resized

		float4 m_ClearColour;

		RingArray<GraphicsCommandBufferRef> m_DefaultGraphicsCB;

		//Default Render Targets
		RingArray<RenderTargetViewRef, GRAPHICS_SETTINGS_NUMFRAMERESOURCES>	m_SwapChainRTVs;			// only views can be accessed

		DataFormat_ m_BackBufferFormat;

		Viewport m_BackBufferViewport;					// The viewport to which the 3D world will be rendered onto
		Rect	 m_BackBufferScissor;					// Pixels outside of this rectangle are culled (not rasterized onto the back buffer)

		//TODO: [deprecate this]
		//Default DSV
		TextureBufferRef										m_DepthStencilBuffer;						// The depth/stencil buffer
		DepthStencilViewRef										m_DefaultDSV;

		std::unique_ptr<TimeManager> m_GraphicsContextTimer;

		uint32_t m_numberOfFrameResources;
		float m_FrameTimeTargetMS;
		FrameTimeWaitMode m_FrameTimeWaitMode;

		bool b_bClearBackBuffer;
		bool b_IsFullScreen;
		bool b_Headless;
	};
}
