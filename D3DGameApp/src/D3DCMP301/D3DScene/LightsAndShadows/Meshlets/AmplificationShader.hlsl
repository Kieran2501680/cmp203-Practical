
#include "../LightingFunctionsHLSL/LSSharedStructs.hlsli"

cbuffer FRAMEDATA : register(b0, space0)
{
    FrameData Frame;
}

cbuffer INOUT : register(b1)
{
    uint instanceID;
    uint StartingGeometry;
}

groupshared AS_Payload s_Payload;

[NumThreads(1, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint dtid : SV_DispatchThreadID, uint gid : SV_GroupID)
{

    DispatchMesh(1, 1, 1, s_Payload);
}