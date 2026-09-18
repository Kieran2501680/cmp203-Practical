#pragma once

#include "D3DCMP301/D3DScene/BindlessRootSignature.h"
#include "D3DCMP301/D3DScene/D3DResource.h"
#include "D3DCMP301/D3DScene/LightsAndShadows/LightAndShadow.h"
#include "D3DCMP301/D3DScene/LightsAndShadows/Vertex.h"

struct DeferredPass
{
	static void Prepare()
	{
		auto device = Skateboard::D3D::gD3DContext->GetDevice();
		//create textured lit render state
		{
			ComPtr<IDxcBlobEncoding> vertexShader;
			ComPtr<IDxcBlobEncoding> pixelShader;

			Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(L"GBufferVS", vertexShader.GetAddressOf());
			Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(L"GBufferPS", pixelShader.GetAddressOf());

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
			psoDesc.NumRenderTargets = NumberOfRenderTargets;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
			psoDesc.RTVFormats[1] = DXGI_FORMAT_R32G32B32A32_FLOAT;
			psoDesc.SampleDesc.Count = 1;
			D3D_CHECK_FAILURE(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_GeometryPipeline)));

			m_GeometryPipeline->SetName(L"ColourPipline");

		}

		//create gbuffer resolve compute pipeline
		{
			D3D12_COMPUTE_PIPELINE_STATE_DESC Desc{};
			ComPtr<IDxcBlobEncoding> computeShader;

			ASSERT_SIMPLE(Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(L"GBufferResolveCS", computeShader.GetAddressOf()));

			Desc.CS = { computeShader->GetBufferPointer(), computeShader->GetBufferSize() };
			Desc.pRootSignature = BindlessRootSignature::GetSignature();

			D3D_CHECK_FAILURE(device->CreateComputePipelineState(&Desc, IID_PPV_ARGS(&m_ResolvePipline)));
		}

		CreateTextures(GraphicsContext::GetClientWidth(), GraphicsContext::GetClientHeight());
	}

	static void CreateTextures(uint32_t Width, uint32_t Height)
	{
		//create render Targets and depth buffer
		{
			auto Allocator = Skateboard::D3D::gD3DContext->GetMemoryAllocator();
			auto& RTVAllocator = Skateboard::D3D::gD3DContext->GetRTVDescriptorHeap();
			auto& DSVAllocator = Skateboard::D3D::gD3DContext->GetDSVDescriptorHeap();
			auto& SRVAllocator = Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap();

			auto device = Skateboard::D3D::gD3DContext->GetDevice();

			if(Albedo)
			{
				D3D::gD3DContext->DeferredRelease(Albedo.Detach());
				D3D::gD3DContext->DeferredRelease(Normal.Detach());
				D3D::gD3DContext->DeferredRelease(Depth.Detach());

				RTVAllocator.DeferredFree(RTVs);
				DSVAllocator.DeferredFree(DSV);
				SRVAllocator.DeferredFree(SRVs);
			}

			auto ResourceDesc = CD3DX12_RESOURCE_DESC1::Tex2D(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, Width, Height, 1, 1);
			ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			auto AllocDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_DEFAULT };

			D3D12_CLEAR_VALUE RTVClear{};
			RTVClear.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			RTVClear.Color[0]= 0.f; 
			RTVClear.Color[1]= 0.f; 
			RTVClear.Color[2]= 0.f;
			RTVClear.Color[3]= 0.f; 


			Allocator->CreateResource3(&AllocDesc, &ResourceDesc, D3D12_BARRIER_LAYOUT_RENDER_TARGET, &RTVClear, 0, nullptr, Albedo.GetAddressOf(), IID_NULL, nullptr);
			Allocator->CreateResource3(&AllocDesc, &ResourceDesc, D3D12_BARRIER_LAYOUT_RENDER_TARGET, &RTVClear, 0, nullptr, Normal.GetAddressOf(), IID_NULL, nullptr);

			RTVs = RTVAllocator.Allocate(NumberOfRenderTargets);

			device->CreateRenderTargetView(Albedo.Get()->GetResource(), nullptr, RTVs.GetCPUHandle());
			device->CreateRenderTargetView(Normal.Get()->GetResource(), nullptr, (RTVs+1).GetCPUHandle());

			ResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
			ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			D3D12_CLEAR_VALUE DSVClear{};
			DSVClear.Format = DXGI_FORMAT_D32_FLOAT;
			DSVClear.DepthStencil.Depth = 0.0f;
			Allocator->CreateResource3(&AllocDesc, &ResourceDesc, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE, &DSVClear, 0, nullptr, Depth.GetAddressOf(), IID_NULL, nullptr);

			DSV = DSVAllocator.Allocate();
			device->CreateDepthStencilView(Depth.Get()->GetResource(), nullptr, DSV.GetCPUHandle());

			SRVs = SRVAllocator.Allocate(NumberOfRenderTargets+1);

			device->CreateShaderResourceView(Albedo.Get()->GetResource(), nullptr, SRVs.GetCPUHandle());
			device->CreateShaderResourceView(Normal.Get()->GetResource(), nullptr, (SRVs + 1).GetCPUHandle());

			auto DepthSRVDesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT,1,0);

			device->CreateShaderResourceView(Depth.Get()->GetResource(), &DepthSRVDesc, (SRVs + 2).GetCPUHandle());

			Albedo->GetResource()->SetName(L"GBuffer-Albedo");
			Normal->GetResource()->SetName(L"GBuffer-Normal");
			Depth->GetResource()->SetName(L"GBuffer-Depth");

			RTVState.Access = D3D12_BARRIER_ACCESS_RENDER_TARGET;
			RTVState.Layout = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
			RTVState.Sync = D3D12_BARRIER_SYNC_RENDER_TARGET;

			DSVState.Access = D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
			DSVState.Layout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
			DSVState.Sync = D3D12_BARRIER_SYNC_DEPTH_STENCIL;
		}
	}

	static void Begin(ID3D12GraphicsCommandList10* list)
	{
		ID3D12Resource* RTVresources[] = {
			Albedo.Get()->GetResource(),
			Normal.Get()->GetResource(),
		};

		//Transition the render targets to be shader readable
		Transition::BarrierToRTV(RTVresources, RTVState, list, 2);

		ID3D12Resource* DSVresources[] = {
			Depth.Get()->GetResource()
		};

		Transition::BarrierToDSV_WRITE(DSVresources, DSVState, list, 1);

		float4 clear = { 0,0,0,0 };

		list->ClearRenderTargetView(RTVs.GetCPUHandle(), (float*)&clear, 0, nullptr);
		list->ClearRenderTargetView((RTVs+1).GetCPUHandle(), (float*)&clear, 0, nullptr);
		list->ClearDepthStencilView(DSV.GetCPUHandle(),D3D12_CLEAR_FLAG_DEPTH, 0, 0,0, nullptr);

		list->SetPipelineState(m_GeometryPipeline.Get());
		list->OMSetRenderTargets(NumberOfRenderTargets, &RTVs.GetCPUHandle(), true, &DSV.GetCPUHandle());
	}

	static void End(ID3D12GraphicsCommandList10* list)
	{
		ID3D12Resource* RTVresources[] = {
			Albedo.Get()->GetResource(),
			Normal.Get()->GetResource(),
		};

		ID3D12Resource* DSVresources[] = {
			Depth.Get()->GetResource()
		};

		//Transition the render targets to be shader readable
		Transition::BarrierToSRV(RTVresources, RTVState, list, 2);
		Transition::BarrierToSRV(DSVresources, DSVState, list, 1);
	}

	static void ResolveGBuffer(ID3D12GraphicsCommandList10* list, const uint32_t OutputUAV_IDX)
	{
		list->SetComputeRootSignature(BindlessRootSignature::GetSignature());
		list->SetPipelineState(m_ResolvePipline.Get());

		uint2 src = { SRVs.GetIndex() , OutputUAV_IDX };

		LightAndShadowRenderer::SetComputeFrameData(list);

		list->SetComputeRoot32BitConstants(1,2, &src, 0);

		list->Dispatch(
			(uint32_t)ceil(GraphicsContext::GetClientWidth() / 16.0f),
			(uint32_t)ceil(GraphicsContext::GetClientHeight() / 16.0f),
			1);
	}

	static constexpr uint32_t NumberOfRenderTargets = 2;

	inline static Resource Albedo;
	inline static Resource Normal;
	inline static Resource Depth;

	inline static TextureResourceState RTVState;
	inline static TextureResourceState DSVState;

	inline static D3D::SHADER_VIEW_HANDLE SRVs; //2 handles for albedo and normal

	inline static D3D::RTV_HANDLE RTVs; //2 handles for albedo and normal
	inline static D3D::DSV_HANDLE DSV;  //1 handle for depth

	inline static ComPtr<ID3D12PipelineState> m_GeometryPipeline;
	inline static ComPtr<ID3D12PipelineState> m_ResolvePipline;
};