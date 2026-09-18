#pragma once

#include "Skateboard/Graphics/Resources/CommandBuffer.h"
#include "Skateboard/Graphics/RHI/GraphicsContext.h"

#include "Windows/WindowsPlatform/DirectX12/Graphics/D3D.h"
#include "Windows/WindowsPlatform/DirectX12/vendor/D3D12MemoryAllocator/include/D3D12MemAlloc.h"

#include "Skateboard/Memory/VirtualAllocator.h"


using namespace Skateboard::MemoryUtils;
using namespace Microsoft::WRL;

//class UploadManager;

namespace Skateboard::D3D
{
	class D3DGraphicsContext;

	class UploadManager
	{
		struct UploadReceipt
		{
			VirtualAllocation allocation;
			uint64_t CompletionFenceValue;
		};

		struct UploadTaskData
		{
			UploadReceipt Receipt;

			ID3D12GraphicsCommandList10* CommandList;
			ID3D12CommandAllocator* CommandAllocator;

			uint64_t Size;

			uint64_t SrcOfft;
			ID3D12Resource* SRC;

			uint64_t DstOfft;
			ID3D12Resource* DST;

			void PrepareCommandList(Skateboard::D3D::D3DGraphicsContext*);
		};

		std::mutex UploadMutex;

		Skateboard::FenceRef m_CompletionFence;
		uint64_t m_CompletionValue;

		D3D12MA::Allocation* p_UploadBuffer;
		UINT8* p_UploadDataBegin;

		BlockAllocator* p_SubAllocator;
		std::queue<UploadReceipt> v_PendingUploads;

		D3D12MA::Allocator* p_Allocator;
		ID3D12Device14* p_Device;

	public:
		void Init(D3D12MA::Allocator* , const size_t UploadBufferSize);

		CopyResult UploadBuffer(ID3D12Resource* Dst, uint32_t dstOffset, void* src, size_t size);
		CopyResult UploadBuffer(ID3D12Resource* Dst, uint32_t dstOffset, size_t size, const std::function<void(void*)>& Writer);
	};
}