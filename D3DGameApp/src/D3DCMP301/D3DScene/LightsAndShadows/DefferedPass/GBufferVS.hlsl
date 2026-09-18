
#include "../LightingFunctionsHLSL/LSSharedStructs.hlsli"


cbuffer FRAMEDATA : register(b0, space0)
{
    FrameData Frame;
}

cbuffer INOUT : register(b1)
{
    uint instanceID;
    uint unused;
}

VS_OUT main(in VS_IN input)
{
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[Frame.InstanceDataSRV_IDX];

    VS_OUT output;

    output.pos = mul(Instances[instanceID].m_WorldTransform, float4(input.pos, 1.f));
    output.worldpos = output.pos;
    output.pos = mul(Frame.ViewMatrix, output.pos);
    output.pos = mul(Frame.ProjectionMatrix, output.pos);
    output.uv = input.uv;

    output.normal = mul((float3x3)(Instances[instanceID].m_WorldTransform), input.normal);

    return output;
}




