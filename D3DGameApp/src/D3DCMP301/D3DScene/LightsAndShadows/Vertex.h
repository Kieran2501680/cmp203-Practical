#pragma once

struct Vertex
{
	float3 position;
	float3 normal;
	float2 uv;

#ifdef __cplusplus
	inline static D3D12_INPUT_LAYOUT_DESC GetD3DLayout();
	inline static Skateboard::BufferLayout GetSKTBDLayout();
#endif

};


#ifdef __cplusplus

#include "Windows/WindowsPlatform/DirectX12/Graphics/RHI/D3DGraphicsContext.h"

//get d3d layout for this vertex structure
inline D3D12_INPUT_LAYOUT_DESC Vertex::GetD3DLayout()
{
	static D3D12_INPUT_ELEMENT_DESC elements[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	return { elements, 3 };
}

//get skateboard equivalent layout for this vertex structure
inline Skateboard::BufferLayout Vertex::GetSKTBDLayout()
{
	using namespace Skateboard;
	return {
		{ POSITION, ShaderDataType_::Float3 },
		{ NORMAL,   ShaderDataType_::Float3 },
		{ TEXCOORD, ShaderDataType_::Float2 }
	};
};
#endif