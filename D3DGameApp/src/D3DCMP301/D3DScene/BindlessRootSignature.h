#pragma once

#include <d3dx12.h>

#include "Windows/WindowsPlatform/DirectX12/Graphics/RHI/D3DGraphicsContext.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DBuffer.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DView.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DPipeline.h"

struct BindlessRootSignature
{
	static void Prepare()
	{
		auto device = Skateboard::D3D::gD3DContext->GetDevice();

        //Create Root Signature for bindless input output textures
        {
            CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;

            CD3DX12_ROOT_PARAMETER params[] =
            {
                CD3DX12_ROOT_PARAMETER({.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV,.Descriptor = {0,0}, .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL, }),
                CD3DX12_ROOT_PARAMETER({.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,.Constants = {1,0,2}, .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL, })
            };

            rootSignatureDesc.Init(2, params, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;
            D3D_CHECK_FAILURE(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
            D3D_CHECK_FAILURE(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_Root)));
        }
		
	}

	static auto GetSignature()
	{
		return m_Root.Get();
	}

	static inline ComPtr<ID3D12RootSignature> m_Root;
};
