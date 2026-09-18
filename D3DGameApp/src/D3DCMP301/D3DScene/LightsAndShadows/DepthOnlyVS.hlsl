#include "LightingFunctionsHLSL/LSSharedStructs.hlsli"
#include "LightingFunctionsHLSL/LightFunctions.hlsli"

cbuffer FRAMEDATA : register(b0, space0)
{
    FrameData Frame;
}

cbuffer INOUT : register(b1)
{
    uint InstanceId;
    uint lightId;
}

ShadowVS_OUT main(in VS_IN input, uint inst : SV_InstanceID)
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[Frame.InstanceDataSRV_IDX];
    StructuredBuffer<Light> Lights = ResourceDescriptorHeap[Frame.LightBufferSRV_IDX];

    ShadowVS_OUT output;

    output.pos = mul(Instances[InstanceId].m_WorldTransform, float4(input.pos, 1.f));
    output.pos = mul(Lights[lightId].LightView, output.pos);
    output.pos = mul(Lights[lightId].LightProjection, output.pos);

    output.RT_ID = inst;

    return output;
}