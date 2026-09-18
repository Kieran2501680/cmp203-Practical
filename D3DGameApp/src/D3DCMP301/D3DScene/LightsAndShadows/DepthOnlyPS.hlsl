
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

float main(ShadowVS_OUT input) : SV_Depth
{
	return input.pos.z / input.pos.w;
}