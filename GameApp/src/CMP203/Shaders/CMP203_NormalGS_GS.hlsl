#include "Structs.hlsli"

struct PixelInput
{
    float4 position : SV_Position; // Careful here, it's different!!
    float3 normal : NORMAL;
};

ConstantBuffer<Constants> PushData : register(b0, space0);
StructuredBuffer<InstanceData> Instances : register(t0, space0);

cbuffer FRAMEDATA : register(b1, space0)
{
    Frame FrameData;
}

[maxvertexcount(6)]
void main(triangle VertexOutput input[3], inout LineStream<PixelInput> lStream)
{
    PixelInput output;
    for (int i = 0; i < 3; i++)
    {
        output.normal = input[i].normal;
        
        output.position = input[i].position;
        output.position = mul(FrameData.View, output.position);
        output.position = mul(FrameData.Projection, output.position);
        
        lStream.Append(output);
        
        output.position = input[i].position + float4(input[i].normal,0.f);
        output.position = mul(FrameData.View, output.position);
        output.position = mul(FrameData.Projection, output.position);
        
        lStream.Append(output);
        lStream.RestartStrip();
    }
}
