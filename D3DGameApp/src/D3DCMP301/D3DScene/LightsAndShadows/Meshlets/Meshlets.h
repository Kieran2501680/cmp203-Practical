#pragma once

#include <d3dx12.h>

#include "D3DCMP301/D3DScene/BindlessRootSignature.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/RHI/D3DGraphicsContext.h"
#include "meshoptimizer/meshoptimizer.h"

struct MeshInfo
{
	std::wstring filename;
	std::string tag;
};

struct Meshlets
{
	static std::vector<Mesh*> PreprocessMeshesIntoMeshlets(std::vector<MeshInfo> Meshes_to_Process, Skateboard::BufferLayout Layout)
	{
		std::vector<Mesh*> processed;
		MeshletCounts.resize(Meshes_to_Process.size());

		for (auto& mesh : Meshes_to_Process)
		{
			//load the mesh
			auto meshdata = Skateboard::AssetManager::LoadModelCPU(mesh.filename.c_str(), Layout);

			for (auto idx = 0; auto& i : meshdata)
			{
				const size_t max_vertices = 64;
				const size_t max_triangles = 126; // note: in v0.25 or prior, max_triangles needs to be divisible by 4
				const float cone_weight = 0.0f;

				size_t max_meshlets = meshopt_buildMeshletsBound(i.IndexCount, max_vertices, max_triangles);
				std::vector<meshopt_Meshlet> meshlets(max_meshlets);
				std::vector<unsigned int> meshlet_vertices(i.IndexCount);
				std::vector<unsigned char> meshlet_triangles(i.IndexCount);

				auto meshletcount = 0;

				if (i.IndexLayout == Skateboard::bit16)
				{
					meshletcount = meshopt_buildMeshlets<uint16_t>(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), (uint16_t*)i.IndexData.data(),
						i.IndexCount, (float*)i.VertexData.data(), i.VertexCount, Layout.GetStride(), max_vertices, max_triangles, cone_weight);
				}
				else if (i.IndexLayout == Skateboard::bit32)
				{
					meshletcount = meshopt_buildMeshlets<uint32_t>(meshlets.data(), meshlet_vertices.data(), meshlet_triangles.data(), (uint32_t*)i.IndexData.data(),
						i.IndexCount, (float*)i.VertexData.data(), i.VertexCount, Layout.GetStride(), max_vertices, max_triangles, cone_weight);
				}

				const meshopt_Meshlet& last = meshlets[meshletcount - 1];

				meshlet_vertices.resize(last.vertex_offset + last.vertex_count);
				meshlet_triangles.resize(last.triangle_offset + last.triangle_count * 3);
				meshlets.resize(meshletcount);

				std::vector<PrimitiveData> processed_primitive(meshletcount);

				auto name = mesh.tag + "_meshlets";

				SKTBD_LOG_INFO("meshlets", "Printing meshlets of {}", name)

				for (auto idx = 0; idx<meshletcount;idx++)
				{
					auto& meshlet = meshlets[idx];

					PrimitiveData out{};
					out.IndexCount = meshlet.triangle_count * 3;
					out.VertexCount = meshlet.vertex_count;
					out.IndexLayout = Skateboard::bit16;
					out.VertexLayout = Layout;
					out.IndexData.resize(out.IndexCount * sizeof(uint16_t));
					out.VertexData.resize((uint64_t)meshlet.vertex_count * Layout.GetStride());
					out.BoundingBox = i.BoundingBox;
					out.Name = name + std::to_string(idx);

					for(uint indice = 0;  indice < meshlet.triangle_count*3; indice++)
					{
						out.GetIndexData<uint16_t>()[indice] = meshlet_triangles[meshlet.triangle_offset + indice];
					}

					for (uint vertice = 0; vertice < meshlet.vertex_count; vertice++)
					{
						auto translatedVertex = meshlet_vertices[meshlet.vertex_offset + vertice];
						out.GetVertexData<Vertex>()[vertice] = i.GetVertexData<Vertex>()[translatedVertex];
					}

					processed_primitive[idx] = std::move(out);
				}

				//save meshlets to gpu
				processed.push_back(Skateboard::AssetManager::CreateModelFromBuffers(name, processed_primitive));
			}

			//save the original mesh to gpu
			Skateboard::AssetManager::CreateModelFromBuffers(mesh.tag, meshdata);

		}
		return processed;
	}

	static void Prepare()
	{
		CD3DX12_STATE_OBJECT_DESC SODesc(D3D12_STATE_OBJECT_TYPE_EXECUTABLE);

		// Optional flag to allow state object additions
		auto pConfig = SODesc.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
		pConfig->SetFlags(D3D12_STATE_OBJECT_FLAG_ALLOW_STATE_OBJECT_ADDITIONS);

		auto pGenericProgram = SODesc.CreateSubobject<CD3DX12_GENERIC_PROGRAM_SUBOBJECT>();

		//signature
		auto pRootSig = SODesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
		pRootSig->SetRootSignature(BindlessRootSignature::GetSignature());

		auto CreateShaderSubobject = [&](const std::wstring filename, IDxcBlobEncoding* blob)->bool
			{
				if (Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(filename.c_str(), &blob))
				{
					auto pS = SODesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
					CD3DX12_SHADER_BYTECODE bcS(blob->GetBufferPointer(), blob->GetBufferSize());
					pS->SetDXILLibrary(&bcS);
					pS->DefineExport(filename.c_str(), L"*");
					pGenericProgram->AddExport(filename.c_str());
					return true;
				}
				return false;
			};

		std::array<IDxcBlobEncoding*, 3> Blobs = {}; //store blobs as a temp array as we need the for lifetime

		//ASSERT_SIMPLE(CreateShaderSubobject(L"AmplificationShader", Blobs[0]))
		ASSERT_SIMPLE(CreateShaderSubobject(L"MeshShader", Blobs[1])		 )
		ASSERT_SIMPLE(CreateShaderSubobject(L"GBufferPS", Blobs[2])			 )
		

		auto pRast = SODesc.CreateSubobject<CD3DX12_RASTERIZER_SUBOBJECT>();
		pGenericProgram->AddSubobject(*pRast);

		auto pPrimitiveTopology = SODesc.CreateSubobject<CD3DX12_PRIMITIVE_TOPOLOGY_SUBOBJECT>();
		pPrimitiveTopology->SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);

		pGenericProgram->AddSubobject(*pPrimitiveTopology);

		auto pRTFormats = SODesc.CreateSubobject<CD3DX12_RENDER_TARGET_FORMATS_SUBOBJECT>();
		pRTFormats->SetNumRenderTargets(2);

		for (int renderTarget = 0; renderTarget < 2; renderTarget++)
		{
			pRTFormats->SetRenderTargetFormat(renderTarget, DXGI_FORMAT_R32G32B32A32_FLOAT);
		}

		pGenericProgram->AddSubobject(*pRTFormats);

		
		//auto pBlend = SODesc.CreateSubobject<CD3DX12_BLEND_SUBOBJECT>();

		//	 Describe a blend state
		//D3D12_BLEND_DESC blendDesc = {};
		//pBlend->SetAlphaToCoverageEnable(false);											// Specifies whether to use alpha-to-coverage as a multisampling technique when setting a pixel to a render target
		//pBlend->SetIndependentBlendEnable(false);									// Specifies whether to enable independent blending in simultaneous render targets (FALSE only uses RenderTarget[0])#

		//for (int renderTarget = 0; renderTarget < desc.RenderTargetCount; renderTarget++)
		//	{
		//		auto& config = desc.Blend.RTBlendConfigs[renderTarget];

		//		D3D12_RENDER_TARGET_BLEND_DESC rtdesc;
		//		rtdesc.BlendEnable = config.BlendEnable;
		//		rtdesc.BlendOp = (D3D12_BLEND_OP)config.BlendOp;
		//		rtdesc.BlendOpAlpha = (D3D12_BLEND_OP)config.BlendOpAlpha;
		//		rtdesc.DestBlend = (D3D12_BLEND)config.DestBlend;
		//		rtdesc.DestBlendAlpha = (D3D12_BLEND)config.DestBlendAlpha;
		//		rtdesc.LogicOp = (D3D12_LOGIC_OP)config.LogicOp;
		//		rtdesc.LogicOpEnable = config.LogicOpEnable;
		//		rtdesc.RenderTargetWriteMask = config.RenderTargetWriteMask;
		//		rtdesc.SrcBlend = (D3D12_BLEND)config.SrcBlend;
		//		rtdesc.SrcBlendAlpha = (D3D12_BLEND)config.SrcBlendAlpha;

		//		pBlend->SetRenderTarget(renderTarget, rtdesc);
		//	}

		//	pGenericProgram->AddSubobject(*pBlend);
		

		
		//DEPTH STENCIL SUBOBJECT 2 CAUSES ERRORS IN PIX 2507.11 version as of 17/09/2025 USING stencil object 1 for the time being;
		/*
			//auto pDepth = SODesc.CreateSubobject<CD3DX12_DEPTH_STENCIL2_SUBOBJECT>();

			//pDepth->SetDepthEnable(DSconfig.DepthEnable);									// Specify whether to enable depth testing
			//pDepth->SetDepthWriteMask (DSconfig.DepthWriteAll ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO);		// Enable or disable writing to sections or all of the depth testing buffer
			//pDepth->SetDepthBoundsTestEnable(DSconfig.DepthBoundsTestEnable);
			//pDepth->SetDepthFunc((D3D12_COMPARISON_FUNC)DSconfig.DepthFunc);				// Function to compare new depth data to existing depth data
			//
			//pDepth->SetStencilEnable(DSconfig.StencilEnable);								// Spicify whether to enable stencil testing
			//

			//D3D12_DEPTH_STENCILOP_DESC1 FrontDesc;

			//FrontDesc.StencilDepthFailOp = (D3D12_STENCIL_OP)DSconfig.FrontFace.StencilDepthFailOp;
			//FrontDesc.StencilFailOp = (D3D12_STENCIL_OP)DSconfig.FrontFace.StencilFailOp;
			//FrontDesc.StencilFunc = (D3D12_COMPARISON_FUNC)DSconfig.FrontFace.StencilFunc;
			//FrontDesc.StencilPassOp = (D3D12_STENCIL_OP)DSconfig.FrontFace.StencilPassOp;
			//FrontDesc.StencilReadMask = DSconfig.FrontFace.StencilReadMask;							// Identify a portion of the depth-stencil buffer for reading stencil data
			//FrontDesc.StencilWriteMask = DSconfig.FrontFace.StencilWriteMask;						// Identify a portion of the depth-stencil buffer for writing stencil data

			//pDepth->SetFrontFace(FrontDesc);

			//D3D12_DEPTH_STENCILOP_DESC1 BackDesc;
			//BackDesc.StencilDepthFailOp = (D3D12_STENCIL_OP)DSconfig.BackFace.StencilDepthFailOp;
			//BackDesc.StencilFailOp = (D3D12_STENCIL_OP)DSconfig.BackFace.StencilFailOp;
			//BackDesc.StencilFunc = (D3D12_COMPARISON_FUNC)DSconfig.BackFace.StencilFunc;
			//BackDesc.StencilPassOp = (D3D12_STENCIL_OP)DSconfig.BackFace.StencilPassOp;
			//BackDesc.StencilReadMask = DSconfig.BackFace.StencilReadMask;							// Identify a portion of the depth-stencil buffer for reading stencil data
			//BackDesc.StencilWriteMask = DSconfig.BackFace.StencilWriteMask;						// Identify a portion of the depth-stencil buffer for writing stencil data

			//pDepth->SetBackFace(BackDesc);
		*/

		auto pDepth = SODesc.CreateSubobject<CD3DX12_DEPTH_STENCIL1_SUBOBJECT>();

		pDepth->SetDepthEnable(true);									// Specify whether to enable depth testing
		pDepth->SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK_ALL);		// Enable or disable writing to sections or all of the depth testing buffer
		pDepth->SetDepthFunc(D3D12_COMPARISON_FUNC_GREATER);				// Function to compare new depth data to existing depth data

		pDepth->SetStencilEnable(false);								// Spicify whether to enable stencil testing

		auto pDSFormat = SODesc.CreateSubobject<CD3DX12_DEPTH_STENCIL_FORMAT_SUBOBJECT>();
		pDSFormat->SetDepthStencilFormat(DXGI_FORMAT_D32_FLOAT);

		pGenericProgram->AddSubobject(*pDepth);
		pGenericProgram->AddSubobject(*pDSFormat);

		pGenericProgram->SetProgramName(L"MeshletPipeline");

		D3D_CHECK_FAILURE(Skateboard::D3D::gD3DContext->GetDevice()->CreateStateObject(SODesc, IID_PPV_ARGS(&MeshletPipelineState)));

		ComPtr<ID3D12StateObjectProperties1> pSOProperties;
		D3D_CHECK_FAILURE(MeshletPipelineState->QueryInterface(IID_PPV_ARGS(&pSOProperties)));
		Program = { D3D12_PROGRAM_TYPE_GENERIC_PIPELINE, pSOProperties->GetProgramIdentifier(L"MeshletPipeline") };

		//cleaning
		for (auto& Blob : Blobs)
		{
			if (Blob)Blob->Release();
		}
	};

	static void DrawMeshlets(ID3D12GraphicsCommandList10* list, uint32_t MeshToDraw, uint32_t InstanceDataToUse)
	{
		list->SetProgram(&Program);
		auto meshletCount = MeshletCounts[MeshToDraw];
		auto meshletOffset = 0u;

		for (uint32_t i = 0; i < MeshToDraw; i++)
		{
			meshletOffset += MeshletCounts[i];
		}

		uint2 SRC = { InstanceDataToUse, meshletOffset };

		list->SetGraphicsRoot32BitConstants(1, 2, &SRC, 0);
		list->DispatchMesh(25, 1, 1);
	}

	inline static std::vector<uint32_t> MeshletCounts;

	inline static ComPtr<ID3D12Resource> MeshletDataBuffer;

	inline static ComPtr<ID3D12StateObject> MeshletPipelineState;
	inline static D3D12_SET_PROGRAM_DESC Program;
};