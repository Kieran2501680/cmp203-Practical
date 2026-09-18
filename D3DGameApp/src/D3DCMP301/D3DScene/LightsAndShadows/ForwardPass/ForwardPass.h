#pragma once
#include "D3DCMP301/D3DScene/LightsAndShadows/LightAndShadow.h"
#include "D3DCMP301/D3DScene/LightsAndShadows/Vertex.h"

struct ForwardPass
{
	static void Prepare()
	{
		auto device = Skateboard::D3D::gD3DContext->GetDevice();
		//create textured lit render state
		{
			ComPtr<IDxcBlobEncoding> vertexShader;
			ComPtr<IDxcBlobEncoding> pixelShader;

			Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(L"TextureLitShadowVS.hlsl", vertexShader.GetAddressOf());
			Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(L"TextureLitShadowPS.hlsl", pixelShader.GetAddressOf());

			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = Vertex::GetD3DLayout();
			psoDesc.pRootSignature = BindlessRootSignature::GetSignature();
			psoDesc.VS = D3D12_SHADER_BYTECODE(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
			psoDesc.PS = D3D12_SHADER_BYTECODE(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState.DepthEnable = true;
			psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
			psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			psoDesc.DepthStencilState.FrontFace = psoDesc.DepthStencilState.BackFace;
			psoDesc.DepthStencilState.StencilEnable = false;
			psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			D3D_CHECK_FAILURE(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_GeometryPipeline)));

			m_GeometryPipeline->SetName(L"ColourPipline");
		}
	}

	static void Begin(ID3D12GraphicsCommandList10* list, const D3D12_CPU_DESCRIPTOR_HANDLE* ColourRenderTarget, const D3D12_CPU_DESCRIPTOR_HANDLE* DepthRenderTarget)
	{
		list->SetPipelineState(m_GeometryPipeline.Get());
		list->OMSetRenderTargets(1, ColourRenderTarget, false, DepthRenderTarget);
	}

	inline static ComPtr<ID3D12PipelineState> m_GeometryPipeline;
};
