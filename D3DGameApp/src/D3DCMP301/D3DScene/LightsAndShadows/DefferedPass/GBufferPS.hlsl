#include "../LightingFunctionsHLSL/LSSharedStructs.hlsli"


cbuffer FRAMEDATA : register(b0)
{
    FrameData Frame;
}

cbuffer INOUT : register(b1)
{
    uint InstanceId;
    uint lightId;
}

struct Output
{
    float4 Albedo : SV_Target0;
    float4 Normal : SV_Target1;
    float  Depth : SV_Depth;
};



Output main(VS_OUT input)
{
	Output output;

    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[Frame.InstanceDataSRV_IDX];

    Texture2D texture = ResourceDescriptorHeap[Instances[InstanceId].AlbedoTextureID];
    SamplerState TextureSampler = SamplerDescriptorHeap[Instances[InstanceId].SamplerID];

	float4 SampledColour = texture.Sample(TextureSampler, input.uv);

    if (SampledColour.a < 0.99f) discard;

	output.Albedo.rgb = SampledColour.rgb;
    output.Normal.rgb = input.normal;

    output.Albedo.a = Instances[InstanceId].SpecularData.specularStrength;
    output.Normal.a = Instances[InstanceId].SpecularData.specularPower;

	output.Depth = input.pos.z;

    return output;
}
