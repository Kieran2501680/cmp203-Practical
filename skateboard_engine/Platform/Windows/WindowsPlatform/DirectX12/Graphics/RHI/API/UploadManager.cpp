#include "sktbdpch.h"
#include "UploadManager.h"
#include "Graphics/RHI/D3DGraphicsContext.h"
#include "Graphics/Resources/D3DCommandBuffer.h"

namespace Skateboard::D3D
{
	void UploadManager::UploadTaskData::PrepareCommandList(Skateboard::D3D::D3DGraphicsContext* context)
	{
		context->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&CommandAllocator));
		context->GetDevice()->CreateCommandList1(0,D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&CommandList));
		CommandList->Reset(CommandAllocator, nullptr);
	}

	void UploadManager::Init(D3D12MA::Allocator* alloc, const size_t UploadBufferSize)
	{
		D3D12_RESOURCE_DESC1 Desc{};
		Desc.Alignment = 0;
		Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		Desc.DepthOrArraySize = 1;
		Desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		Desc.Format = DXGI_FORMAT_UNKNOWN;
		Desc.Height = 1;
		Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		Desc.MipLevels = 1;
		Desc.SampleDesc.Count = 1;
		Desc.SampleDesc.Quality = 0;
		Desc.Width = ROUND_UP(UploadBufferSize, 64 * 1024);

		D3D12MA::ALLOCATION_DESC AllocDesc{};
		AllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

		D3D_CHECK_FAILURE(alloc->CreateResource3(
			&AllocDesc,
			&Desc,
			D3D12_BARRIER_LAYOUT_UNDEFINED,
			nullptr,
			0, nullptr,
			&p_UploadBuffer,
			IID_NULL,
			nullptr
		));

		        // We do not intend to read from this resource on the CPU. // we also never need to unmap this as we are are perpertually gonna be be wrting to it
		D3D_CHECK_FAILURE(p_UploadBuffer->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&p_UploadDataBegin)));

		VIRTUAL_BLOCK_DESC VB{};
		VB.Size = ROUND_UP(UploadBufferSize, 64 * 1024);

		CreateBlock(&VB, &p_SubAllocator);

		m_CompletionFence = Skateboard::ResourceFactory::CreateFence(FenceType_::FenceType_CPU_GPU, 0, L"UploadManagerFence");
		m_CompletionValue = 0;
	}

	CopyResult UploadManager::UploadBuffer(ID3D12Resource* Dst, uint32_t DstOffset, void* src, size_t size)
	{
			UploadTaskData TaskData;
			TaskData.PrepareCommandList(Skateboard::D3D::gD3DContext);

			UploadMutex.lock();

			auto completionValue = ++m_CompletionValue;
			TaskData.Receipt.CompletionFenceValue = completionValue;

			VIRTUAL_ALLOCATION_DESC desc{};
			desc.Size = size;
			desc.Alignment = 4;
			uint64_t SrcOffset;
			p_SubAllocator->Allocate(&desc, &TaskData.Receipt.allocation, &SrcOffset);

			ID3D12Resource* Src = p_UploadBuffer->GetResource();
			auto fence = static_cast<D3DFence*>(m_CompletionFence.get())->m_FenceObject.Get();

			TaskData.DST = Dst;
			TaskData.DstOfft = DstOffset;
			TaskData.SRC = p_UploadBuffer->GetResource();
			TaskData.SrcOfft = SrcOffset;

			std::future<void> future = std::async(std::launch::async, [&](UploadTaskData DATA)->void {

				auto cb = DATA.CommandList;

				D3D12_BUFFER_BARRIER startbarriers[] =
				{
					D3D12_BUFFER_BARRIER(D3D12_BARRIER_SYNC_ALL, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COMMON, D3D12_BARRIER_ACCESS_COPY_DEST, DATA.DST,0, UINT64_MAX)
				};

				D3D12_BARRIER_GROUP group(D3D12_BARRIER_TYPE_BUFFER, 1);
				group.pBufferBarriers = startbarriers;

				cb->Barrier(1, &group);

				cb->CopyBufferRegion(DATA.DST, DATA.DstOfft, DATA.SRC, DATA.SrcOfft, size);

				D3D12_BUFFER_BARRIER endBarriers[] =
				{
					D3D12_BUFFER_BARRIER(D3D12_BARRIER_SYNC_ALL, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_DEST,D3D12_BARRIER_ACCESS_COMMON , DATA.DST, 0,UINT64_MAX)
				};

				group.pBufferBarriers = endBarriers;

				cb->Barrier(1, &group);

				cb->Close();

				ID3D12CommandList* lists[] = { cb };

				Skateboard::D3D::gD3DContext->GetD3DCopyCommandQueue()->ExecuteCommandLists(1, lists);
				Skateboard::D3D::gD3DContext->GetD3DCopyCommandQueue()->Signal(fence, DATA.Receipt.CompletionFenceValue);

				//SKTBD_MSG_INFO("Executing ASYNC UPLOAD COMMAND LIST ")

				auto event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

				fence->SetEventOnCompletion(DATA.Receipt.CompletionFenceValue, event);

				WaitForSingleObject(event, INFINITE);
				CloseHandle(event);

				//SKTBD_MSG_INFO("COMPLETED ASYNC UPLOAD")

				UploadMutex.lock();

				p_SubAllocator->FreeAllocation(DATA.Receipt.allocation);
				//SKTBD_MSG_INFO("RELEASED ASYNC ALLOCATION")

				UploadMutex.unlock();

				DATA.CommandList->Release();
				DATA.CommandAllocator->Release();


			}, std::move(TaskData));

			UploadMutex.unlock();

			CopyResult result{};

			return{ completionValue , m_CompletionFence, std::move(future) };
	}

	CopyResult UploadManager::UploadBuffer(ID3D12Resource* Dst, uint32_t DstOffset, size_t size, const std::function<void(void*)>& Writer)
	{
		UploadTaskData TaskData{};
		TaskData.PrepareCommandList(Skateboard::D3D::gD3DContext);

		UploadMutex.lock();

		auto completionValue = ++m_CompletionValue;
		TaskData.Receipt.CompletionFenceValue = completionValue;

		VIRTUAL_ALLOCATION_DESC desc{};
		desc.Size = size;
		desc.Alignment = 4;
		uint64_t SrcOffset;
		p_SubAllocator->Allocate(&desc, &TaskData.Receipt.allocation, &SrcOffset);

		auto fence = static_cast<D3DFence*>(m_CompletionFence.get())->m_FenceObject.Get();

		TaskData.Size = size;
		TaskData.DST = Dst;
		TaskData.DstOfft = DstOffset;
		TaskData.SRC = p_UploadBuffer->GetResource();
		TaskData.SrcOfft = SrcOffset;

		std::future<void> future = std::async(std::launch::async, [&](UploadTaskData DATA, const std::function<void(void*)>& WriterFunctor) {

			SKTBD_MSG_INFO("Begin ASYNC UPLOAD")

			WriterFunctor(p_UploadDataBegin + DATA.SrcOfft);

			SKTBD_MSG_INFO("Copied Data ASYNC UPLOAD")

			auto cb = DATA.CommandList;

			D3D12_BUFFER_BARRIER startbarriers[] =
			{
				D3D12_BUFFER_BARRIER(D3D12_BARRIER_SYNC_ALL, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COMMON, D3D12_BARRIER_ACCESS_COPY_DEST, DATA.DST,0, UINT64_MAX)
			};

			D3D12_BARRIER_GROUP group(D3D12_BARRIER_TYPE_BUFFER, 1);
			group.pBufferBarriers = startbarriers;

			cb->Barrier(1, &group);

			cb->CopyBufferRegion(DATA.DST, DATA.DstOfft, DATA.SRC,  DATA.SrcOfft, DATA.Size);

			D3D12_BUFFER_BARRIER endBarriers[] =
			{
				D3D12_BUFFER_BARRIER(D3D12_BARRIER_SYNC_ALL, D3D12_BARRIER_SYNC_COPY, D3D12_BARRIER_ACCESS_COPY_DEST,D3D12_BARRIER_ACCESS_COMMON , DATA.DST, 0,UINT64_MAX)
			};

			group.pBufferBarriers = endBarriers;

			cb->Barrier(1, &group);

			cb->Close();

			ID3D12CommandList* lists[] = { cb };

			Skateboard::D3D::gD3DContext->GetD3DCopyCommandQueue()->ExecuteCommandLists(1, lists);
			Skateboard::D3D::gD3DContext->GetD3DCopyCommandQueue()->Signal(fence, DATA.Receipt.CompletionFenceValue);

			//SKTBD_MSG_INFO("Executing ASYNC UPLOAD COMMAND LIST ")

			auto event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

			fence->SetEventOnCompletion(DATA.Receipt.CompletionFenceValue, event);

			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);

			//SKTBD_MSG_INFO("COMPLETED ASYNC UPLOAD")

			UploadMutex.lock();

			p_SubAllocator->FreeAllocation(DATA.Receipt.allocation);
			//SKTBD_MSG_INFO("RELEASED ASYNC ALLOCATION")

			UploadMutex.unlock();

			DATA.CommandList->Release();
			DATA.CommandAllocator->Release();

			}, std::move(TaskData), Writer);

		UploadMutex.unlock();

		CopyResult result{};

		return{ completionValue , m_CompletionFence, std::move(future) };
	}
}
