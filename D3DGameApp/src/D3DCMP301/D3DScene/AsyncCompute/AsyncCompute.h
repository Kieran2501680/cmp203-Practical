#pragma once

#include <d3dx12.h>
#include "Windows/WindowsPlatform/DirectX12/Graphics/RHI/D3DGraphicsContext.h"

struct AsyncCompute
{
	static void  Prepare()
	{
		auto device = Skateboard::D3D::gD3DContext->GetDevice();

		device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(m_AsyncAllocator.ReleaseAndGetAddressOf()));
		device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_LIST_FLAGS::D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(m_AsyncList.ReleaseAndGetAddressOf()));

		device->CreateFence(0, D3D12_FENCE_FLAGS::D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_AsyncFence.ReleaseAndGetAddressOf()));

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			D3D_CHECK_FAILURE(HRESULT_FROM_WIN32(GetLastError()));
		}
	}

	static auto GetAsyncList() -> ID3D12GraphicsCommandList10*
	{
		return m_AsyncList.Get();
	}

	static void Begin()
	{
		if (m_AsyncFence->GetCompletedValue() < m_AsyncFenceValue)
		{
			SKTBD_LOG_ERROR("AsyncCompute", "NotDoneYet")
		}

		m_AsyncAllocator->Reset();
		m_AsyncList->Reset(m_AsyncAllocator.Get(), nullptr);
	}

	static void Wait()
	{
		if(m_AsyncFence->GetCompletedValue() >= m_AsyncFenceValue)
		{
			SKTBD_LOG_TRACE("AsyncCompute","Wait returned immediately as fence is complete")
			return;
		}

		D3D_CHECK_FAILURE(m_AsyncFence->SetEventOnCompletion(m_AsyncFenceValue, m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}

	static void InsertWaitIntoGraphicsQueue()
	{
		Skateboard::D3D::gD3DContext->GetD3DGraphicsCommandQueue()->Wait(m_AsyncFence.Get(), m_AsyncFenceValue);
	}

	static void End()
	{
		m_AsyncList->Close();

		ID3D12CommandList* list[] = {m_AsyncList.Get()};

		Skateboard::D3D::gD3DContext->GetD3DComputeCommandQueue()->ExecuteCommandLists(1, list);
		D3D_CHECK_FAILURE(Skateboard::D3D::gD3DContext->GetD3DComputeCommandQueue()->Signal(m_AsyncFence.Get(), ++m_AsyncFenceValue));
	}

	inline static ComPtr<ID3D12GraphicsCommandList10> m_AsyncList;
	inline static ComPtr<ID3D12CommandAllocator> m_AsyncAllocator;
	inline static ComPtr<ID3D12Fence> m_AsyncFence;
	inline static uint64_t m_AsyncFenceValue;
	inline static HANDLE m_fenceEvent;
};