#pragma once

#include <algorithm>
#include <d3dx12.h>

#include "Vertex.h"
#include "LightingFunctionsHLSL/LSSharedStructs.hlsli"
#include "Windows/WindowsPlatform/DirectX12/Graphics/RHI/D3DGraphicsContext.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DBuffer.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DView.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DPipeline.h"

//NOTES CUBEMAP ORDER
//+X -X +Y -Y +Z -Z

ENUM_FLAG_OPERATORS(FrameFlags)

struct LightAndShadowRenderer
{
	struct Light_CPU
	{
		inline static uint32_t ShadowsMapSize = 1024;

		Light_CPU(LightType type, const std::wstring& name, bool create_shadowMaps = false)
		{

			m_Name = name;
			if (create_shadowMaps) CreateShadowmaps();
			//init m_data
			{
				switch (type)
					{
					case LightDirectional:
						m_data.Type = LightDirectional;
						m_data.LightDirection = float3(1, -1, 0);
						m_data.DiffuseColour = float4(0.75, 0.75, 0.75, 1);
						m_data.LightProjection = glm::orthoLH_ZO(-10.f, 10.f, -10.f, 10.f, 10.f, -10.f);
						m_data.LightView = glm::lookAtLH(float3(0.f,0.f,0.f), m_data.LightDirection, float3(0.f,1.f,0.f));
						break;
					case LightPoint:
						m_data.Type = LightPoint;
						m_data.DiffuseColour = float4(0.5, 0.5, 0.5, 1);
						m_data.LightPosition = float3(0, 0, 0);
						m_data.ConstantAttenuation = 0.05f;
						m_data.LinearAttenuation = 0.01f;
						m_data.SquareAttenuation = 0.001f;
						m_data.CutOffDistance = 100.f;
						break;
					case LightSpot:
						m_data.Type = LightSpot;
						m_data.LightDirection = float3(0, -1, 0);
						m_data.DiffuseColour = float4(0.5, 0.5, 0.5, 1);
						m_data.LightPosition = float3(0, 0, 0);
						m_data.ConstantAttenuation = 0.05f;
						m_data.LinearAttenuation = 0.01f;
						m_data.SquareAttenuation = 0.001f;
						m_data.CutOffDistance = 100.f;
						m_data.InnerCone = cos(radians(30.f));
						m_data.OuterCone = cos(radians(90.f));
						m_data.FalloffPower = 2;
						m_data.LightProjection = glm::perspectiveLH(glm::radians(90.f), 1.f, m_data.CutOffDistance, 0.01f);
						break;
					}
			}
		}

		void CreateShadowmaps()
		{

			auto device = Skateboard::D3D::gD3DContext->GetDevice();

			auto& SRV_HEAP = Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap();
			auto& DSV_HEAP = Skateboard::D3D::gD3DContext->GetDSVDescriptorHeap();

			//Create Depth  Render target
			CD3DX12_RESOURCE_DESC1 resourceDesc{};

			switch (GetLight().Type)
			{
			case LightDirectional:
				resourceDesc = CD3DX12_RESOURCE_DESC1::Tex2D(DXGI_FORMAT_D32_FLOAT, ShadowsMapSize, ShadowsMapSize, 1, 1);
				break;

			case LightSpot:
				resourceDesc = CD3DX12_RESOURCE_DESC1::Tex2D(DXGI_FORMAT_D32_FLOAT, ShadowsMapSize, ShadowsMapSize, 1, 1);
				break;

			case LightPoint:
				resourceDesc = CD3DX12_RESOURCE_DESC1::Tex2D(DXGI_FORMAT_D32_FLOAT, ShadowsMapSize / 4, ShadowsMapSize / 4, 6, 1);
				break;

			}

			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			auto allocDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_DEFAULT };

			auto allocator = Skateboard::D3D::gD3DContext->GetMemoryAllocator();

			D3D12_CLEAR_VALUE clearValue{ .Format = DXGI_FORMAT_D32_FLOAT, .DepthStencil = D3D12_DEPTH_STENCIL_VALUE{0.f,0} };

			allocator->CreateResource3(
				&allocDesc,
				&resourceDesc,
				D3D12_BARRIER_LAYOUT::D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,
				&clearValue,
				0,
				NULL,
				&m_DepthRenderTarget,
				IID_NULL,
				nullptr
			);

			m_DepthRenderTarget->GetResource()->SetName((m_Name + L"_DSRT").c_str());

			//pointlight needs a special case for the views;
			CD3DX12_SHADER_RESOURCE_VIEW_DESC SRVPointdesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::TexCube(DXGI_FORMAT_R32_FLOAT);//create default views for the render target resource
			CD3DX12_SHADER_RESOURCE_VIEW_DESC SRVDirSpotdesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::Tex2D(DXGI_FORMAT_R32_FLOAT);//create default views for the render target resource

			D3D12_DEPTH_STENCIL_VIEW_DESC DSVdesc{};
			DSVdesc.Format = DXGI_FORMAT_D32_FLOAT;
			DSVdesc.Flags = D3D12_DSV_FLAG_NONE;
			DSVdesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
			DSVdesc.Texture2DArray.ArraySize = 6;
			DSVdesc.Texture2DArray.FirstArraySlice = 0;
			DSVdesc.Texture2DArray.MipSlice = 0;

			m_SRVHandle = SRV_HEAP.Allocate();
			m_DSVHandle = DSV_HEAP.Allocate();

			switch (GetLight().Type)
			{
			case LightDirectional:
				//create default views for the render target resource
				device->CreateShaderResourceView(m_DepthRenderTarget->GetResource(), &SRVDirSpotdesc, m_SRVHandle.GetCPUHandle());
				device->CreateDepthStencilView(m_DepthRenderTarget->GetResource(), nullptr, m_DSVHandle.GetCPUHandle());
				break;
			case LightSpot:
				//create default views for the render target resource
				device->CreateShaderResourceView(m_DepthRenderTarget->GetResource(), &SRVDirSpotdesc, m_SRVHandle.GetCPUHandle());
				device->CreateDepthStencilView(m_DepthRenderTarget->GetResource(), nullptr, m_DSVHandle.GetCPUHandle());
				break;
			case LightPoint:
				//= D3D12_DEPTH_STENCIL_VIEW_DESC(DXGI_FORMAT_R32_FLOAT, D3D12_DSV_DIMENSION_TEXTURE2DARRAY, );//create default views for the render target resource
				device->CreateShaderResourceView(m_DepthRenderTarget->GetResource(), &SRVPointdesc, m_SRVHandle.GetCPUHandle());
				device->CreateDepthStencilView(m_DepthRenderTarget->GetResource(), &DSVdesc, m_DSVHandle.GetCPUHandle());
			}

			m_data.ShadowMapID = m_SRVHandle.GetIndex();
			m_data.ShadowMapSamplerID = m_DefaultShadowSampler->GetSamplerIndex();

			b_ShadowsReady = true;
		}
			
		void ReleaseShadowMaps()
		{
			if(m_DepthRenderTarget)
			{
				auto& SRV_HEAP = Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap();
				auto& DSV_HEAP = Skateboard::D3D::gD3DContext->GetDSVDescriptorHeap();

				Skateboard::D3D::gD3DContext->DeferredRelease(m_DepthRenderTarget);
				m_DepthRenderTarget = nullptr;
				SRV_HEAP.DeferredFree(m_SRVHandle);
				DSV_HEAP.DeferredFree(m_DSVHandle);
			}

			m_data.ShadowMapID = AssetManager::GetDefaultTexture(TextureDimension_Texture2D)->GetViewIndex();
			m_data.ShadowMapSamplerID = m_DefaultShadowSampler->GetSamplerIndex();

			b_ShadowsReady = false;
		}

		auto& GetLight() const { return m_data; }
		auto& GetLight() { return m_data; }
		auto& GetLightAlloc() const { return m_DepthRenderTarget; }
		auto& GetLightDSV()  { return m_DSVHandle; }
		auto& GetLightDSV() const { return m_DSVHandle; }
		auto& GetLightSRV()  { return m_SRVHandle; }
		auto& GetLightSRV() const { return m_SRVHandle; }
		auto& GetName() const { return m_Name; }

	protected:
		Light m_data;

		std::wstring m_Name;
		D3D12MA::Allocation* m_DepthRenderTarget;
		Skateboard::D3D::DSV_HANDLE m_DSVHandle;
		Skateboard::D3D::SHADER_VIEW_HANDLE m_SRVHandle;

		bool b_ShadowsReady = false;
	};

	static void Prepare()
	{
		auto device = Skateboard::D3D::gD3DContext->GetDevice();

		//create Default sampler
		{
			Skateboard::SamplerDesc desc;
			desc = SamplerDesc::InitAsDefaultTextureSampler();

			m_DefaultTextureSampler = ResourceFactory::CreateSampler(desc, L"TextureSampler");

			desc = SamplerDesc::InitAsDefaultShadowSampler();

			m_DefaultShadowSampler = ResourceFactory::CreateSampler(desc, L"ShadowSampler");

		}

		//create instance data
		{
			auto resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer({ 64 * 1024,0 });
			auto allocator = Skateboard::D3D::gD3DContext->GetMemoryAllocator();

			auto allocDesc = (allocator->IsGPUUploadHeapSupported()) ? D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_GPU_UPLOAD } : D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_UPLOAD };

			for (auto count = 0; auto& [allocation, SRV, pointer_mapping] : m_InstanceBuffer)
			{
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

				allocation->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&pointer_mapping));

				InstanceData DefaultData{};

				DefaultData.m_WorldTransform = glm::identity<matrix>();
				DefaultData.AlbedoTextureID = Skateboard::AssetManager::GetDefaultTexture(TextureDimension_Texture2D)->GetViewIndex();
				DefaultData.SamplerID = m_DefaultTextureSampler->GetSamplerIndex();

				memcpy(pointer_mapping, &DefaultData, sizeof(InstanceData));

				allocation->GetResource()->SetName(std::wstring(L"InstanceDataBuffer" + std::to_wstring(count++)).c_str());

				SRV = Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().Allocate(1);
				auto viewdesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer((64 * 1024) / sizeof(InstanceData), sizeof(InstanceData));
				device->CreateShaderResourceView(allocation->GetResource(), &viewdesc, SRV.GetCPUHandle());
			}
		}

		//create light data
		{
			auto resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer({ 64 * 1024,0 });

			auto allocator = Skateboard::D3D::gD3DContext->GetMemoryAllocator();

			auto allocDesc = (allocator->IsGPUUploadHeapSupported()) ? D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_GPU_UPLOAD } : D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_UPLOAD };

			for (auto count = 0; auto& [allocation, SRV, pointer_mapping] : m_LightsBuffer)
			{
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

				allocation->GetResource()->Map(0, nullptr, reinterpret_cast<void**>(&pointer_mapping));

				allocation->GetResource()->SetName(std::wstring(L"LightDataBuffer" + std::to_wstring(count++)).c_str());

				SRV = Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().Allocate(1);
				auto viewdesc = CD3DX12_SHADER_RESOURCE_VIEW_DESC::StructuredBuffer((64 * 1024) / sizeof(Light), sizeof(Light));
				device->CreateShaderResourceView(allocation->GetResource(), &viewdesc, SRV.GetCPUHandle());
			}
		}

		//create frame Buffers
		{
			auto resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer({ sizeof(FrameData),0 });

			auto allocator = Skateboard::D3D::gD3DContext->GetMemoryAllocator();

			auto allocDesc = (allocator->IsGPUUploadHeapSupported()) ? D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_GPU_UPLOAD } : D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_UPLOAD };

			for (uint count = 0; auto& [allocation, pointer_mapping] : m_FrameBuffer)
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

				allocation->GetResource()->SetName(std::wstring(L"FrameDataBuffer" + std::to_wstring(count++)).c_str());
			}
		}

		//create depth only render state
		{
			auto device = Skateboard::D3D::gD3DContext->GetDevice();

			ComPtr<IDxcBlobEncoding> vertexShader;
			ComPtr<IDxcBlobEncoding> pixelShader;

			Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(L"DepthOnlyVS.hlsl", vertexShader.GetAddressOf());
			Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(L"DepthOnlyPS.hlsl", pixelShader.GetAddressOf());

			// Describe and create the graphics pipeline state object (PSO).
			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = Vertex::GetD3DLayout();
			psoDesc.pRootSignature = BindlessRootSignature::GetSignature();
			psoDesc.VS = D3D12_SHADER_BYTECODE(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
			psoDesc.PS = D3D12_SHADER_BYTECODE(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState.DepthEnable = true;
			psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			psoDesc.DepthStencilState.StencilEnable = false;
			psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			psoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			psoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			psoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_GREATER;
			psoDesc.DepthStencilState.FrontFace = psoDesc.DepthStencilState.BackFace;
			psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			psoDesc.NumRenderTargets = 0;
			psoDesc.SampleDesc.Count = 1;
			D3D_CHECK_FAILURE(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_DepthPipeline)));

			m_DepthPipeline->SetName(L"DepthPipline");
		}

		m_Lights.emplace_back(LightDirectional, L"DirectionalLight", false);

		m_Frame = {};
		m_Frame.InstanceDataSRV_IDX = std::get<D3D::SHADER_VIEW_HANDLE>(m_InstanceBuffer.Get()).GetIndex();
		m_Frame.LightBufferSRV_IDX = std::get<D3D::SHADER_VIEW_HANDLE>(m_LightsBuffer.Get()).GetIndex();
	}

	static const std::vector<Light_CPU>& GetLights() { return m_Lights; }
	static std::vector<Light_CPU>& GetEditableLights() { bLightsDirty = true; return m_Lights; }

	static void SetCameraData(const matrix& view, const matrix& projection, const float3& position)
	{
		m_Frame.ViewMatrix = view;
		m_Frame.ProjectionMatrix = projection;
		m_Frame.InvProjection = glm::inverse(projection);
		m_Frame.InvView = glm::inverse(view);
		m_Frame.CameraPosition = position;
	}

	static void SetGraphicsFrameData(ID3D12GraphicsCommandList10* list)
	{
		list->SetGraphicsRootConstantBufferView(0, m_FrameBuffer.Get().first->GetResource()->GetGPUVirtualAddress());
	}

	static void SetComputeFrameData(ID3D12GraphicsCommandList10* list)
	{
		list->SetComputeRootConstantBufferView(0, m_FrameBuffer.Get().first->GetResource()->GetGPUVirtualAddress());
	}

	static void SetAmbientLight(const float4& ambient)
	{
		m_Frame.AmbientLight = ambient;
	}

	static void SetRaytracingSceneIDX(const uint32_t& AS_Handle)
	{
		m_Frame.RaytracingSceneSRV_IDX = AS_Handle;
	}

	static void SetGeometryInfoAndStorageBufferIDX(const uint32_t& geometryBufferHandle, const uint32_t& VertexStorageBuffer)
	{
		m_Frame.GeometryBufferSRV_IDX = geometryBufferHandle;
		m_Frame.VertexIndexBufferSRV_IDX = VertexStorageBuffer;
	}

	static uint32_t PushInstance(const InstanceData& instance)
	{
		++m_CurrentInstance;
		std::get<InstanceData*>(m_InstanceBuffer.Get())[m_CurrentInstance] = instance;
		return m_CurrentInstance;
	}

	static void UpdateLightsAndFrameData()
	{
		if (bLightsDirty)
		{
			m_LightsBuffer.IncrementCounter();

			int index = 0;
			for (auto& light : m_Lights)
			{
				std::get<Light*>(m_LightsBuffer.Get())[index] = light.GetLight();
				++index;
			}

			m_Frame.LightCount = m_Lights.size();
			m_Frame.LightBufferSRV_IDX = std::get<D3D::SHADER_VIEW_HANDLE>(m_LightsBuffer.Get()).GetIndex();

			bLightsDirty = false;
		}

		
		m_FrameBuffer.IncrementCounter();
		++m_Frame.FrameIndex;

		memcpy(m_FrameBuffer.Get().second, &m_Frame, sizeof(FrameData));
	}

	static void ResetInstanceBuffer()
	{
		++m_InstanceBuffer;
		m_CurrentInstance = 0;
		m_Frame.InstanceDataSRV_IDX = std::get<D3D::SHADER_VIEW_HANDLE>(m_InstanceBuffer.Get()).GetIndex();
	}

	static void BeginDepthPass(ID3D12GraphicsCommandList10* list)
	{
		if (!m_Lights.empty())
		{
			std::vector<CD3DX12_TEXTURE_BARRIER> barriers(m_Lights.size());

			auto range = D3D12_BARRIER_SUBRESOURCE_RANGE{};
			range.IndexOrFirstMipLevel = 0xffffffff; //means all subresources

			std::transform(m_Lights.begin(), m_Lights.end(), barriers.begin(), [range, list](Light_CPU& light) {
				return CD3DX12_TEXTURE_BARRIER
				(
					D3D12_BARRIER_SYNC_DRAW, D3D12_BARRIER_SYNC_DRAW,
					D3D12_BARRIER_ACCESS_SHADER_RESOURCE, D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE,
					D3D12_BARRIER_LAYOUT_SHADER_RESOURCE, D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE,
					light.GetLightAlloc()->GetResource(),
					range,
					D3D12_TEXTURE_BARRIER_FLAG_NONE
				);
				});

			CD3DX12_BARRIER_GROUP barrierGroup(barriers.size(), barriers.data());

			list->Barrier(1, &barrierGroup);
		}

		list->SetPipelineState(m_DepthPipeline.Get());
	}

	static void RecordCommandPerLight(ID3D12GraphicsCommandList10* List, std::function<void(ID3D12GraphicsCommandList10* list, const Light_CPU* light, uint32_t InstanceCountMultiplier,  uint32_t Execution_idx)> DrawCalls)
	{
		D3D12_VIEWPORT viewport = { 0,0,1024,1024,0,1.f };

		List->RSSetViewports(1, &viewport);

		D3D12_RECT scissor = { 0, 0, 1024, 1024 };

		List->RSSetScissorRects(1, &scissor);

		auto lightIdx = 0;
		for(auto& light : m_Lights)
		{
			//set the light index so we know which light we are rendering for in the shader and which projection and view to use
			List->SetGraphicsRoot32BitConstant(1, lightIdx, 1);

			//clear and set the depth target
			List->ClearDepthStencilView(
				light.GetLightDSV().GetCPUHandle(),								// DSV to the resource we want to clear
				D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,		// Flags indicating which part of the depth/stencil buffer to clear (here both)
				GRAPHICS_DEPTH_DEFAULT_CLEAR_COLOUR,					// Defines the value to clear the depth buffer
				GRAPHICS_STENCIL_DEFAULT_CLEAR_COLOUR,					// Defines the value to clear the stencil buffer
				0,														// The number of items in the pRects array (next parameter)
				nullptr													// An array of D3D12_RECTs that identify rectangle regions on the render target to clear. When nullptr, the entire render target is cleared
			);

			List->OMSetRenderTargets(0, nullptr, false, &light.GetLightDSV().GetCPUHandle());

			if (light.GetLight().Type == LightPoint)
			{
				//gotta draw that cube map
				DrawCalls(List, &light, 6, lightIdx);
			}
			else
			{
				//spot and directional only need a s ingle map light needs to draw once
				DrawCalls(List, &light, 1, lightIdx);
			}
			++lightIdx;
		}
	}

	static void PrepareShadowMapsForReading(ID3D12GraphicsCommandList10* list)
	{
		if (!m_Lights.empty())
		{
			std::vector<CD3DX12_TEXTURE_BARRIER> barriers(m_Lights.size());

			auto range = D3D12_BARRIER_SUBRESOURCE_RANGE{};
			range.IndexOrFirstMipLevel = 0xffffffff; //means all subresources

			std::transform(m_Lights.begin(), m_Lights.end(), barriers.begin(), [range](Light_CPU& light) {
				return CD3DX12_TEXTURE_BARRIER
				(
					D3D12_BARRIER_SYNC_DRAW, D3D12_BARRIER_SYNC_DRAW,
					D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE, D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
					D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE, D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,
					light.GetLightAlloc()->GetResource(),
					range,
					D3D12_TEXTURE_BARRIER_FLAG_NONE
				);
				});

			CD3DX12_BARRIER_GROUP barrierGroup(barriers.size(), barriers.data());

			list->Barrier(1, &barrierGroup);
		}
	}

	static void RenderImGui()
	{
		ImGui::Separator();

		//if (ImGui::Button("Add Light"))
		//{
		//	//Lights.push_back(CMP203::Light());
		//	m_LightsNeedUpdating = true;
		//}

		auto index = 0;
		auto iterator = m_Lights.begin();

		while (iterator != m_Lights.end())
		{
			using namespace ImGui;
			auto& light = *iterator;
			Text("Light %i", index);

			auto id = "##" + std::to_string(index);
			auto Name = [id](std::string name) -> std::string
				{
					return (name + id);
				};

			if (Button(Name("DeleteLight").c_str()))
			{
				iterator = m_Lights.erase(iterator);
				bLightsDirty = true;
				continue;
			}

			++index;
			++iterator;

			if (Combo(Name("Type").c_str(), (int*)&light.GetLight().Type, "Directional\0Point\0Spot\0")) bLightsDirty = true;
			if (InputFloat3(Name("Diffuse").c_str(), (float*)&light.GetLight().DiffuseColour)) bLightsDirty = true;
			if (InputFloat(Name("ConstantAttenuation").c_str(), &light.GetLight().ConstantAttenuation)) bLightsDirty = true;
			if (InputFloat(Name("LinearAttenuation").c_str(), &light.GetLight().LinearAttenuation)) bLightsDirty = true;
			if (InputFloat(Name("SquareAttenuation").c_str(), &light.GetLight().SquareAttenuation)) bLightsDirty = true;
			if (InputFloat(Name("CutOffDistance").c_str(), &light.GetLight().CutOffDistance)) bLightsDirty = true;
			if (InputFloat3(Name("Position").c_str(), (float*)&light.GetLight().LightPosition)) bLightsDirty = true;
			if (InputFloat(Name("InnerCone").c_str(), &light.GetLight().InnerCone)) bLightsDirty = true;
			if (InputFloat3(Name("Direction").c_str(), (float*)&light.GetLight().LightDirection)) bLightsDirty = true;
			if (InputFloat(Name("OuterCone").c_str(), &light.GetLight().OuterCone)) bLightsDirty = true;
			if (InputFloat(Name("FalloffPower").c_str(), &light.GetLight().FalloffPower)) bLightsDirty = true;
			if (InputFloat(Name("FalloffPower").c_str(), &light.GetLight().FalloffPower)) bLightsDirty = true;

			Separator();
		}
	}

	static void Shutdown()
	{
		for (auto& [allocation, pointer_mapping] : m_FrameBuffer)
		{
			if (allocation)
			{
				allocation->GetResource()->Unmap(0, nullptr);
				allocation->Release();
				allocation = nullptr;
				pointer_mapping = nullptr;
			}
		}
		for (auto& [allocation,SRV, pointer_mapping] : m_LightsBuffer)
		{
			if (allocation)
			{
				Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().DeferredFree(SRV);
				allocation->GetResource()->Unmap(0, nullptr);
				allocation->Release();
				allocation = nullptr;
				pointer_mapping = nullptr;
			}
		}
		for (auto& [allocation,SRV, pointer_mapping] : m_InstanceBuffer)
		{
			if (allocation)
			{
				Skateboard::D3D::gD3DContext->GetSRVDescriptorHeap().DeferredFree(SRV);
				allocation->GetResource()->Unmap(0, nullptr);
				allocation->Release();
				allocation = nullptr;
				pointer_mapping = nullptr;
			}
		}
		m_Lights.clear();

		m_DefaultShadowSampler.reset();
		m_DefaultTextureSampler.reset();
	}

	inline static void SetFrameFlags(FrameFlags flags)
	{
		m_Frame.Flags = flags;
	}

	inline static bool bLightsDirty = true;
	inline static uint32_t m_CurrentInstance = 0;

	inline static ComPtr<ID3D12PipelineState> m_DepthPipeline;

	inline static SamplerRef m_DefaultTextureSampler;
	inline static SamplerRef m_DefaultShadowSampler;

	inline static std::vector<Light_CPU> m_Lights;
	inline static FrameData m_Frame{};

	uint static GetLightCount() { return m_Lights.size(); }
	static auto& GetLightBufferSRV() { return std::get<D3D::SHADER_VIEW_HANDLE>(m_LightsBuffer.Get()); }

	static auto& GetInstanceBufferSRV() { return std::get<D3D::SHADER_VIEW_HANDLE>(m_InstanceBuffer.Get()); }

	//need to buffer those as they are per frame resources
	inline static RingArray<std::pair<D3D12MA::Allocation*, FrameData*>> m_FrameBuffer;
	inline static RingArray<std::tuple<D3D12MA::Allocation*, D3D::SHADER_VIEW_HANDLE,  Light*>> m_LightsBuffer;
	inline static RingArray<std::tuple<D3D12MA::Allocation*, D3D::SHADER_VIEW_HANDLE,  InstanceData*>> m_InstanceBuffer;
};
