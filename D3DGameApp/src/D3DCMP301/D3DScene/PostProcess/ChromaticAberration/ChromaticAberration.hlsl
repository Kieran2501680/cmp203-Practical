
struct ChromAbParams
{
	float2 UVoffset;
	uint SamplerIDX;
};

#ifndef __cplusplus

struct InoutID
{
	uint InputIDX;
	uint OutputIDX;
};

cbuffer Data : register(b0)
{
	ChromAbParams Params;
}

ConstantBuffer<InoutID> Input : register(b1);

[numthreads(32, 32, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	RWTexture2D<float4> outputTexture = ResourceDescriptorHeap[Input.OutputIDX];
	Texture2D inputTexture = ResourceDescriptorHeap[Input.InputIDX];

	SamplerState TextureSampler = SamplerDescriptorHeap[Params.SamplerIDX];

	uint2 dims;
	outputTexture.GetDimensions(dims.x, dims.y);

	float2 UV = float2(DTid.xy) / float2(dims);

	if(DTid.x < dims.x & DTid.y < dims.y)
	{
		float r = inputTexture.SampleLevel(TextureSampler, UV + Params.UVoffset, 0).r;
		float g = inputTexture.SampleLevel(TextureSampler, UV, 0).g;
		float b = inputTexture.SampleLevel(TextureSampler, UV - Params.UVoffset, 0).b;

		outputTexture[DTid.xy] = float4(r, g, b, 1.0f);
	}
}

#endif
