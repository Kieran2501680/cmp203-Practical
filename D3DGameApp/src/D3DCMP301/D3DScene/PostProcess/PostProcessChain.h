#pragma once

#include <d3dx12.h>

#include "D3DCMP301/D3DScene/D3DResource.h"

enum class HandleType : uint8_t
{
    Resource,
    ResourceState,
    RTV,
	SRV,
	UAV,
    Tuple,
};

using TextureResource = std::tuple<Resource, TextureResourceState, Skateboard::D3D::RTV_HANDLE, Skateboard::D3D::SHADER_VIEW_HANDLE, Skateboard::D3D::SHADER_VIEW_HANDLE>;


class Effect
{
public:
	virtual ~Effect() {};
	virtual void Prepare(ID3D12RootSignature* Signature) = 0;
	virtual void Apply(ID3D12GraphicsCommandList10* List, TextureResource& Input, TextureResource& Output) = 0;
};

class PostProcessChain
{
public:

    static void CreateOuputTextures(uint width, uint height)
    {
        m_OutDimensions = { width,height };

        //create output textures
        {
            for (auto idx = 0; auto& [allocation, State, RTV, UAV, SRV] : m_IntermediateBuffers)
            {
                //Create Render target
                {
					//free old resources if they exist, deferred release to avoid resource being in use by gpu after full release;
                    if(allocation.Get())
                    {
                        Skateboard::D3D::gD3DContext->DeferredRelease(allocation.Detach());
						Skateboard::D3D::gD3DContext->GetRTVDescriptorHeap().DeferredFree(RTV);
						Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().DeferredFree(UAV);
						Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().DeferredFree(SRV);
                    }

                    auto resourceDesc = CD3DX12_RESOURCE_DESC1::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1);
                    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

                    auto allocDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_DEFAULT };

                    auto allocator = Skateboard::D3D::gD3DContext->GetMemoryAllocator();

                    D3D12_CLEAR_VALUE ClearColor{ .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .Color = {0.1,0.1,0.1,1.0} };

                    //use older versions of this function if you are not using enhanced barriers
                    allocator->CreateResource3(
                        &allocDesc,
                        &resourceDesc,
                        D3D12_BARRIER_LAYOUT_RENDER_TARGET,
                        &ClearColor,
                        0,
                        NULL,
                        allocation.ReleaseAndGetAddressOf(),
                        IID_NULL,
                        nullptr
                    );

                    State = { .Sync = D3D12_BARRIER_SYNC_RENDER_TARGET, .Access = D3D12_BARRIER_ACCESS_RENDER_TARGET, .Layout = D3D12_BARRIER_LAYOUT_RENDER_TARGET };

                    allocation->GetResource()->SetName((std::wstring(L"PostProcessChain Intermediate Texture") + std::to_wstring(idx++)).c_str());

                    RTV = Skateboard::D3D::gD3DContext->GetRTVDescriptorHeap().Allocate();
                    UAV = Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().Allocate();
                    SRV = Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().Allocate();

                    auto device = Skateboard::D3D::gD3DContext->GetDevice();

                    //create default views for the render target resource
                    device->CreateShaderResourceView(allocation->GetResource(), nullptr, SRV.GetCPUHandle());
                    device->CreateUnorderedAccessView(allocation->GetResource(), nullptr, nullptr, UAV.GetCPUHandle());
                    device->CreateRenderTargetView(allocation->GetResource(), nullptr, RTV.GetCPUHandle());
                }

            }
        }
    }

	static void Prepare()
	{
		auto m_device = Skateboard::D3D::gD3DContext->GetDevice();

        CreateOuputTextures(Skateboard::D3D::gD3DContext->GetClientWidth(), Skateboard::D3D::gD3DContext->GetClientHeight());
	}

    template<HandleType T = HandleType::Tuple>
    static auto& GetInput()
    {
        if constexpr (T == HandleType::Tuple)
            return m_IntermediateBuffers.Get();
        else 
            return std::get<static_cast<uint8_t>(T)>(m_IntermediateBuffers.Get());
    }

    template<HandleType T = HandleType::Tuple>
    static auto& GetOutput()
    {
        if constexpr (T == HandleType::Tuple) 
            return m_IntermediateBuffers.GetNext();
		else
			return  std::get<static_cast<uint8_t>(T)>(m_IntermediateBuffers.GetNext());
    }

    static void PrepareForPostProcessing( ID3D12GraphicsCommandList10* list)
    {
        ID3D12Resource* res[] = {GetInput<HandleType::Resource>()->GetResource()};

        Transition::BarrierToSRV(res, GetInput<HandleType::ResourceState>(), list);
    }

    static void SetScissorAndViewport(ID3D12GraphicsCommandList10* list)
    {
        D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)GetOutput<HandleType::Resource>()->GetResource()->GetDesc().Width,(float)GetOutput<HandleType::Resource>()->GetResource()->GetDesc().Height , 0.0f, 1.0f};
        D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>((float)GetOutput<HandleType::Resource>()->GetResource()->GetDesc().Width), static_cast<LONG>((float)GetOutput<HandleType::Resource>()->GetResource()->GetDesc().Height) };

        list->RSSetViewports(1, &viewport);
        list->RSSetScissorRects(1, &scissorRect);
    }

    static void Flip() { ++m_IntermediateBuffers; }

    static uint2 GetDimensions() { return m_OutDimensions; };

private:
	inline static Skateboard::RingArray<TextureResource, 2> m_IntermediateBuffers;

    inline static uint2 m_OutDimensions;
};
