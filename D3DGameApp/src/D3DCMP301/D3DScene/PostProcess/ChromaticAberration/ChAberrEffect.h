#pragma once

#include "ChromaticAberration.hlsl"
#include "D3DCMP301/D3DScene/PostProcess/PostProcessChain.h"

class ChromaticAberrationEffect : public Effect
{
public:
	void Prepare(ID3D12RootSignature* Signature) override
	{
		auto m_device = Skateboard::D3D::gD3DContext->GetDevice();

        //Chromatic aberration setup
        {
            //Create aberration shader
            {
                ComPtr<IDxcBlobEncoding> Blob;

                SKTBD_LOG_ASSERT(Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(L"ChromaticAberration", Blob.ReleaseAndGetAddressOf()), "PostProcessChain", "Failed to load chromatic aberration shader");

                D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
                desc.CS = { Blob->GetBufferPointer(), Blob->GetBufferSize() };
                desc.pRootSignature = Signature;
                desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

                D3D_CHECK_FAILURE(m_device->CreateComputePipelineState(&desc, IID_PPV_ARGS(&ComputeShaderState)));
            }

            //frame data buffer
            {
                auto resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer({ sizeof(ChromAbParams),0 });

                auto allocator = Skateboard::D3D::gD3DContext->GetMemoryAllocator();

                auto allocDesc = (allocator->IsGPUUploadHeapSupported()) ? D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_GPU_UPLOAD } : D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_UPLOAD };

                for (uint count = 0; auto& [allocation, pointer_mapping] : Params)
                {
                    //create the resource
                    allocator->CreateResource3(
                        &allocDesc,
                        &resourceDesc,
                        D3D12_BARRIER_LAYOUT_UNDEFINED,
                        0,
                        0,
                        NULL,
                        &allocation,
                        IID_NULL,
                        nullptr
                    );

                    //map the resource
                    allocation->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&pointer_mapping));

                    pointer_mapping->UVoffset = float2{ 0.05 ,0.05 };
                    pointer_mapping->SamplerIDX = 0;

                    allocation->GetResource()->SetName(std::wstring(L"ChromAbParamsBuffer" + std::to_wstring(count++)).c_str());
                }
            }

        }
	}

	void Apply(ID3D12GraphicsCommandList10* List, TextureResource& Input, TextureResource& Output) override
	{
		List->SetPipelineState(ComputeShaderState.Get());

		ID3D12Resource* res[] = { std::get<static_cast<uint8_t>(HandleType::Resource)>(Output)->GetResource() };

		//transition relevant resources to the relevant states
		Transition::BarrierToUAV(res, std::get<static_cast<uint8_t>(HandleType::ResourceState)>(Output), List);

		uint32_t InOut[]{ std::get<static_cast<uint8_t>(HandleType::SRV)>(Input).GetIndex(),  std::get<static_cast<uint8_t>(HandleType::UAV)>(Output).GetIndex() };
		List->SetComputeRootConstantBufferView(0, Params.Get().first->GetResource()->GetGPUVirtualAddress());
		List->SetComputeRoot32BitConstants(1, 2, InOut, 0);

		List->Dispatch(std::get<static_cast<uint8_t>(HandleType::Resource)>(Output)->GetResource()->GetDesc().Width / 32, std::get<static_cast<uint8_t>(HandleType::Resource)>(Output)->GetResource()->GetDesc().Height / 32, 1);

        res[0] = std::get<static_cast<uint8_t>(HandleType::Resource)>(Input)->GetResource();

        Transition::BarrierToSRV(res, std::get<static_cast<uint8_t>(HandleType::ResourceState)>(Input), List);
	}

protected:
    Skateboard::RingArray<std::pair<ComPtr<D3D12MA::Allocation>, ChromAbParams*>> Params;
	ComPtr<ID3D12PipelineState> ComputeShaderState;
};
