#pragma once

#include <d3dx12.h>

#include "Windows/WindowsPlatform/DirectX12/Graphics/RHI/D3DGraphicsContext.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DBuffer.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DView.h"
#include "Windows/WindowsPlatform/DirectX12/Graphics/Resources/D3DPipeline.h"

struct Triangle
{
    struct Vertex
    {
        float3 position;
        float4 color;
    };

	static void Prepare()
	{
		auto device = Skateboard::D3D::gD3DContext->GetDevice();
		auto allocator = Skateboard::D3D::gD3DContext->GetMemoryAllocator();

        // Create an empty root signature.
        {
            D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
            rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
            rootSignatureDesc.NumParameters = 0;
            rootSignatureDesc.pParameters = nullptr;
            rootSignatureDesc.NumStaticSamplers = 0;
            rootSignatureDesc.pStaticSamplers = nullptr;


            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;
            D3D_CHECK_FAILURE(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
            D3D_CHECK_FAILURE(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
        }

        // Create the pipeline state, which includes compiling and loading shaders.
        {
            ComPtr<IDxcBlobEncoding> vertexShader;
            ComPtr<IDxcBlobEncoding> pixelShader;

            Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(L"VS.hlsl", vertexShader.GetAddressOf());
            Skateboard::D3D::ShaderUtils::ValidatePathNLoadBlob(L"PS.hlsl", pixelShader.GetAddressOf());


            // Define the vertex input layout.
            D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
            {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            // Describe and create the graphics pipeline state object (PSO).
            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
            psoDesc.pRootSignature = m_rootSignature.Get();
            psoDesc.VS = D3D12_SHADER_BYTECODE(vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
            psoDesc.PS = D3D12_SHADER_BYTECODE(pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());
            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthEnable = FALSE;
            psoDesc.DepthStencilState.StencilEnable = FALSE;
            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleDesc.Count = 1;
            D3D_CHECK_FAILURE(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_DrawObject)));
        }

        // Create the vertex buffer.
        {
            auto m_aspectRatio = Skateboard::D3D::gD3DContext->GetClientAspectRatio();

            // Define the geometry for a triangle.
            Vertex triangleVertices[] =
            {
                { { 0.0f, 0.25f * m_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
                { { 0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
                { { -0.25f, -0.25f * m_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
            };

            const UINT vertexBufferSize = sizeof(triangleVertices);

            // Note: using upload heaps to transfer static data like vert buffers is not 
            // recommended. Every time the GPU needs it, the upload heap will be marshalled 
            // over. Please read up on Default Heap usage. An upload heap is used here for 
            // code simplicity and because there are very few verts to actually transfer.
            /* D3D_CHECK_FAILURE(m_device->CreateCommittedResource(
                &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
                D3D12_HEAP_FLAG_NONE,
                &CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_VertexBuffer)));*/

            auto allocDesc = D3D12MA::ALLOCATION_DESC{ .HeapType = D3D12_HEAP_TYPE_UPLOAD };

            auto resourceDesc = CD3DX12_RESOURCE_DESC1::Buffer(vertexBufferSize);

            allocator->CreateResource3(
                &allocDesc,
                &resourceDesc,
                D3D12_BARRIER_LAYOUT_UNDEFINED,
                0,
                0,
                NULL,
                &m_VertexBufferAllocation,
                IID_NULL,
                nullptr
            );

            // Copy the triangle data to the vertex buffer.
            UINT8* pVertexDataBegin;
            CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
            D3D_CHECK_FAILURE(m_VertexBufferAllocation->GetResource()->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
            memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
            m_VertexBufferAllocation->GetResource()->Unmap(0, nullptr);

            // Initialize the vertex buffer view.
            m_vertexBufferView.BufferLocation = m_VertexBufferAllocation->GetResource()->GetGPUVirtualAddress();
            m_vertexBufferView.StrideInBytes = sizeof(Vertex);
            m_vertexBufferView.SizeInBytes = vertexBufferSize;
        }
	}

	static void SetStateAndDraw(ID3D12GraphicsCommandList10* list)
	{
        //then we draw our triangle to the offscreen render target
        list->SetPipelineState(m_DrawObject.Get());
        list->SetGraphicsRootSignature(m_rootSignature.Get());
        list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        list->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        list->DrawInstanced(3, 1, 0, 0);
	}

    static void Shutdown()
	{
        //nothing to clean up in here;
	}

	inline static ComPtr<ID3D12RootSignature> m_rootSignature;
	inline static ComPtr<ID3D12PipelineState> m_DrawObject;
	inline static D3D12MA::Allocation* m_VertexBufferAllocation;
	inline static D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
};