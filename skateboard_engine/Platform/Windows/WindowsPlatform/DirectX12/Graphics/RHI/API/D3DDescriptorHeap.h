#pragma once
#include <memory>
#include <vector>

#include "Windows/WindowsPlatform/DirectX12/Graphics/D3D.h"

#include "Skateboard/SizedPtr.h"
#include "Skateboard/Graphics/RHI/GraphicsContext.h"
#include "Skateboard/Memory/VirtualAllocator.h"

#include "Skateboard/Log.h"

namespace
{
	constexpr const char* GET_NAME(D3D12_DESCRIPTOR_HEAP_TYPE TYPE) {
		switch (TYPE)
		{
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:	return	"D3D_CBV_SRV_UAV_HEAP";
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:	 	return	"D3D_Sampler_HEAP";
		case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:		 	return	"D3D_RTV_HEAP";
		case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:		 	return	"D3D_DSV_HEAP";
		case D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES:
		default:										return	"SOMETHING WENT WRONG HEAP";
		}
	};
}

namespace Skateboard::D3D
{
	template<D3D12_DESCRIPTOR_HEAP_TYPE HeapType>
	struct D3DDescriptorHandle
	{
		//friend class D3DDescriptorHeap;

		D3DDescriptorHandle operator+(unsigned int i)
		{
			D3DDescriptorHandle ret = *this;

			ret.m_Index+=i;
			ret.m_CPUHandle.ptr += s_HandleSize*i;
			ret.m_GPUHandle.ptr += s_HandleSize*i;

			return ret;
		}

		D3DDescriptorHandle operator-(unsigned int i)
		{
			D3DDescriptorHandle ret = *this;

			ret.m_Index -= i;
			ret.m_CPUHandle.ptr -= s_HandleSize * i;
			ret.m_GPUHandle.ptr -= s_HandleSize * i;

			return ret;
		}

		D3DDescriptorHandle& operator++() {
			
			++m_Index;
			m_CPUHandle.ptr += s_HandleSize;
			m_GPUHandle.ptr += s_HandleSize;
			
			return *this;
		}

		D3DDescriptorHandle operator++(int) {
			D3DDescriptorHandle temp = *this;
			
			m_Index++;
			m_CPUHandle.ptr += s_HandleSize;
			m_GPUHandle.ptr += s_HandleSize;
			
			return temp;
		}

		D3DDescriptorHandle& operator--() {

			--m_Index;
			m_CPUHandle.ptr -= s_HandleSize;
			m_GPUHandle.ptr -= s_HandleSize;

			return *this;
		}

		D3DDescriptorHandle operator--(int) {
			D3DDescriptorHandle temp = *this;

			--m_Index;
			m_CPUHandle.ptr -= s_HandleSize;
			m_GPUHandle.ptr -= s_HandleSize;

			return temp;
		}

		uint64_t GetCPUPointer() const { return m_CPUHandle.ptr; }
		uint64_t GetGPUPointer() const { return m_GPUHandle.ptr; }

		const D3D12_CPU_DESCRIPTOR_HANDLE& GetCPUHandle() const { return m_CPUHandle; }
		const D3D12_GPU_DESCRIPTOR_HANDLE& GetGPUHandle() const { return m_GPUHandle; }

		[[nodiscard("")]] uint32_t GetIndex() const { return m_Index; }

		static uint32_t GetHandleSize() { return s_HandleSize; }

		inline static uint32_t s_HandleSize = 0;

		D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;

		uint32_t m_Index;
		MemoryUtils::VirtualAllocation	m_Allocation;
	};

	template<D3D12_DESCRIPTOR_HEAP_TYPE HeapType>
	class D3DDescriptorHeap
	{
	public:
		D3DDescriptorHeap() : GPU_START_PTR(0), CPU_START_PTR(0), m_suballocator(nullptr){}
		D3DDescriptorHeap(const D3DDescriptorHeap&) = delete;
		auto operator=(const D3DDescriptorHeap&)->D3DDescriptorHeap & = delete;
		D3DDescriptorHeap(D3DDescriptorHeap&&) noexcept = delete;
		auto operator=(D3DDescriptorHeap&&) noexcept -> D3DDescriptorHeap & = delete;

		~D3DDescriptorHeap()
		{
		//	SKTBD_LOG_INFO( GET_NAME(HeapType) , L"Destroying descriptor heap")
		};

		void Create(const std::wstring& debugName, ID3D12Device* device, uint32_t capacity, bool shaderVisible)
		{

			ASSERT_SIMPLE((capacity && capacity < D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2), SKTBD_LOG_ERROR("D3DDescriptorHeap","Invalid capacity!"));

			if constexpr (HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_RTV || HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_DSV)
			{
				m_IsShaderVisible = false;
			}

			m_IsShaderVisible = shaderVisible;

			// Create a descriptor heap that can allocate up to maxCount descriptors
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = HeapType;
			desc.NumDescriptors = capacity;
			desc.Flags = (shaderVisible) ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			desc.NodeMask = 0;
			device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(m_Heap.ReleaseAndGetAddressOf()));
#ifndef SKTBD_SHIP
			m_Heap->SetName(debugName.c_str());
#endif

			//Define handles Static increment size;
			D3DDescriptorHandle<HeapType>::s_HandleSize = device->GetDescriptorHandleIncrementSize(desc.Type);

			if (m_IsShaderVisible)
			{
				GPU_START_PTR = m_Heap->GetGPUDescriptorHandleForHeapStart().ptr;
			}

			CPU_START_PTR = m_Heap->GetCPUDescriptorHandleForHeapStart().ptr;

			MemoryUtils::VIRTUAL_BLOCK_DESC aDesc = {};
			aDesc.Size = desc.NumDescriptors;
			Skateboard::MemoryUtils::CreateBlock(&aDesc, &m_suballocator);
		}

		D3DDescriptorHandle<HeapType> Allocate(uint32_t count = 1u)
		{
			// Protects the heap from race conditions
			std::lock_guard lock(m_HeapMutex);

			if (count == 0u)
			{
			//	SKTBD_MSG_WARN("Tried to allocate 0 descriptors in a descriptor heap. Returning an invalid handle");
				return { {SIZE_T_MAX}, {UINT64_MAX} };
			}

			MemoryUtils::VirtualAllocation allocation;

			Skateboard::MemoryUtils::VIRTUAL_ALLOCATION_DESC allocDesc = {};
			allocDesc.Flags = MemoryUtils::VIRTUAL_ALLOCATION_FLAG_STRATEGY_MIN_TIME;
			allocDesc.Size = count;

			uint64_t offset;

			m_suballocator->Allocate(&allocDesc, &allocation, &offset);

			D3DDescriptorHandle<HeapType> ret;

			ret.m_Allocation = allocation;
			ret.m_Index = static_cast<uint32_t>(offset);
			ret.m_CPUHandle = { CPU_START_PTR + offset * D3DDescriptorHandle<HeapType>::s_HandleSize };

			if (m_IsShaderVisible)
			{
				ret.m_GPUHandle = { GPU_START_PTR + offset * D3DDescriptorHandle<HeapType>::s_HandleSize };
			}
			else
			{
				ret.m_GPUHandle = { UINT64_MAX };
			}

			//SKTBD_LOG_TRACE(GET_NAME(HeapType),"allocated {} descriptors", count)

			return ret;
		};

		void DeferredFree(D3DDescriptorHandle<HeapType>& handle)
		{
			if (handle.m_Allocation.AllocHandle == 0)
			{
				//SKTBD_LOG_WARN(GET_NAME(HeapType), "Tried to free an invalid descriptor handle. Ignoring.");
				return;
			}

			// Protects the heap from race conditions
			std::lock_guard lock(m_HeapMutex);

			MemoryUtils::VIRTUAL_ALLOCATION_INFO info{};

			m_suballocator->GetAllocationInfo(handle.m_Allocation, &info);

			//SKTBD_LOG_TRACE(GET_NAME(HeapType), "freed {} descriptors", info.Size);

			// Defer freeing any resources until the next frame has started. 
			m_DeferredAvailableIndices[m_FrameIndex].push_back(handle.m_Allocation);

			//Inform the internal D3D engine, descriptors need to be free.
			handle = {};
		}

		void ProcessDeferredFree(uint64_t frameResourceIndex)
		{
			// Protects the heap from race conditions
			std::lock_guard lock(m_HeapMutex);

			// Ensure we haven't supplied an invalid frame index.
			SKTBD_LOG_ASSERT(frameResourceIndex < GRAPHICS_SETTINGS_NUMFRAMERESOURCES, GET_NAME(HeapType), "Invalid frame index!");

			std::vector<MemoryUtils::VirtualAllocation>& indices{ m_DeferredAvailableIndices[frameResourceIndex] };

			for (auto allocation : indices)
			{
				m_suballocator->FreeAllocation(allocation);
			}

			indices.clear();
		}

		void Release()
		{
			m_suballocator->Release();
			m_suballocator = nullptr;

			m_Heap->Release();

			// Push the heap back into the deferred resources vector.
			// Again, it is imperative at this point in the shutdown process, all resources have been cleaned.
			//gD3DContext->DeferredRelease(m_Heap.Get());
		}

		[[nodiscard("")]] ID3D12DescriptorHeap* const GetHeap() const { return m_Heap.Get(); }
		[[nodiscard("")]] static constexpr  D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() { return HeapType; }
		[[nodiscard("")]] uint32_t GetHeapSize() const { return m_HeapSize; }
		[[nodiscard("")]] uint32_t GetDescriptorIncrementSize() { return D3DDescriptorHandle<HeapType>::s_HandleSize; }

		void SetCurrentFrameIndex(uint8_t CurrentFrame)
		{
			m_FrameIndex = CurrentFrame;
		}

	private:
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;

		//uint32_t m_DescriptorIncrementSize;	// Avoid continuous query

		SIZE_T GPU_START_PTR;
		SIZE_T CPU_START_PTR;

		uint32_t m_HeapSize{ 0u };
		uint32_t m_HeapCapacity{ 0u };
		uint8_t m_FrameIndex {0u};

		std::vector <MemoryUtils::VirtualAllocation> m_DeferredAvailableIndices[GRAPHICS_SETTINGS_NUMFRAMERESOURCES]{};
		std::mutex m_HeapMutex;

		bool m_IsShaderVisible{ true };

		MemoryUtils::BlockAllocator* m_suballocator;
	};

	typedef D3DDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV> D3D_SHADER_VIEW_HEAP;
	typedef D3DDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>		  D3D_RTV_HEAP;
	typedef D3DDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>		  D3D_DSV_HEAP;
	typedef D3DDescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>	  D3D_SAMPLER_HEAP;

	typedef D3DDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV> SHADER_VIEW_HANDLE;
	typedef D3DDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>			RTV_HANDLE;
	typedef D3DDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>			DSV_HANDLE;
	typedef D3DDescriptorHandle<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>		SAMPLER_HANDLE;

}