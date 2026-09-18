#pragma once

#ifndef STRUCTS_HLSLI
#define STRUCTS_HLSLI

//need to separate between c++ and hlsl

#if defined(__cplusplus)
#include "Skateboard/Mathematics.h"
#include <d3d12.h>
using namespace glm;
using namespace Skateboard;
#define SEMANTIC_POSITION ;
#define SEMANTIC_InstanceID ;
#define SEMANTIC_RT_INDEX ;
#define SEMANTIC(name) ;
#define ALIGNMENT(x) alignas(x)

#else //its hlsl

#define SEMANTIC_POSITION : SV_POSITION;
#define SEMANTIC_InstanceID : SV_InstanceID;
#define SEMANTIC_RT_INDEX : SV_RenderTargetArrayIndex;
#define SEMANTIC(name) : name;
#define ALIGNMENT(x)

#endif

struct PrimitiveStridesAndOffsets
{
    uint IndexBufferOffsetInBytes;
    uint IndexBufferStride : 2;
    uint IndexCount : 30;
    uint VertexBufferOffsetInBytes;
    uint VertexBufferStride : 8;
    uint VertexCount : 24;
};

struct VS_IN
{
    float3 pos SEMANTIC(POSITION)
    float3 normal SEMANTIC(NORMAL)
    float2 uv SEMANTIC(TEXCOORD)
};

struct VS_OUT
{
    float4 pos SEMANTIC_POSITION
    float4 worldpos SEMANTIC(WORLD)
    float3 normal SEMANTIC(NORMAL)
    float2 uv SEMANTIC(TEXCOORD)
};

#define WAVE_SIZE 32

struct AS_Payload
{
    uint MeshletIndices[WAVE_SIZE];
};

struct ShadowVS_OUT
{
    float4 pos SEMANTIC_POSITION
    uint RT_ID SEMANTIC_RT_INDEX
};

enum LightType
{
    LightDirectional = 0,
	LightPoint = 1,
	LightSpot = 2
};

struct InstID
{
    unsigned int InstanceIndex;
};

struct LightID
{
    unsigned int LightIndex;
};

enum FrameFlags
{
    FrameFlag_None = 0,
    FrameFlag_VisualiseMeshlets = 1 << 0,
    FrameFlag_EnableRaytracedShadows = 1 << 1,
    FrameFlag_EnableDepthmapShadows = 1 << 2,
    FrameFlag_EnableAmbientOcclusion = 1 << 3
};

struct ALIGNMENT(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) FrameData
{
    matrix ViewMatrix;
    matrix ProjectionMatrix;
    matrix InvView;
    matrix InvProjection;

    float4 AmbientLight;
    float3 CameraPosition;

    uint FrameIndex;

    uint LightBufferSRV_IDX;
    uint LightCount;

    uint InstanceDataSRV_IDX;
    uint VertexIndexBufferSRV_IDX;

    uint GeometryBufferSRV_IDX;
    uint RaytracingSceneSRV_IDX;

    FrameFlags Flags;
};

struct SpecularData
{
    float specularPower;
    float specularStrength;
};

struct InstanceData
{
    matrix m_WorldTransform;
   
    uint AlbedoTextureID;
    uint SamplerID;

    SpecularData SpecularData;
};

struct Light
{
    float4 DiffuseColour;
	
    float ConstantAttenuation;
    float LinearAttenuation;
    float SquareAttenuation;
    float CutOffDistance;

    float3 LightPosition;
    float InnerCone;

    float3 LightDirection;
    float OuterCone;

    float FalloffPower;
    LightType Type;

    matrix LightProjection;
    matrix LightView;

    uint ShadowMapID;
    uint ShadowMapSamplerID;
};

#endif
