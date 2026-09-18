
#include "../Vertex.h"
#include "../LightingFunctionsHLSL/LSSharedStructs.hlsli"
#include "../LightingFunctionsHLSL/LightFunctions.hlsli"


cbuffer FRAMEDATA : register(b0, space0)
{
    FrameData Frame;
}

cbuffer INOUT : register(b1)
{
    uint instanceID;
    uint StartingGeometry;
}


[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint dtid : SV_DispatchThreadID,
    uint gtid : SV_GroupThreadID,
    
    uint gid : SV_GroupID,
    //in payload AS_Payload payload, //not using as yet
    out vertices VS_OUT verts[64],
    out indices uint3 tris[126]
)
{

    StructuredBuffer<PrimitiveStridesAndOffsets> PrimitiveOffsets = ResourceDescriptorHeap[Frame.GeometryBufferSRV_IDX];
    ByteAddressBuffer VertexAndIndexData = ResourceDescriptorHeap[Frame.VertexIndexBufferSRV_IDX];
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[Frame.InstanceDataSRV_IDX];

	PrimitiveStridesAndOffsets Meshlet = PrimitiveOffsets[StartingGeometry + gid.x];

    SetMeshOutputCounts(Meshlet.VertexCount, Meshlet.IndexCount / 3);

    InstanceData instance = Instances[instanceID];

    PrimitiveStridesAndOffsets meshlet = PrimitiveOffsets[StartingGeometry + gid.x];

    for (uint i = gtid; i < meshlet.VertexCount; i += 128)
    {
        Vertex input = VertexAndIndexData.Load<Vertex>(Meshlet.VertexBufferOffsetInBytes + Meshlet.VertexBufferStride * i);
        
        VS_OUT output;

        output.pos = mul(Instances[instanceID].m_WorldTransform, float4(input.position, 1.f));
        output.worldpos = output.pos;
        output.pos = mul(Frame.ViewMatrix, output.pos);
        output.pos = mul(Frame.ProjectionMatrix, output.pos);
        output.uv = input.uv;

        output.normal = mul((float3x3) (Instances[instanceID].m_WorldTransform), input.normal);
        
        verts[gtid] = output;
    }

    for (uint i = gtid; i < meshlet.IndexCount / 3; i += 128)
    {
        uint baseIndex = meshlet.IndexBufferOffsetInBytes + i * 3 * meshlet.IndexBufferStride;
        
        if (Meshlet.IndexBufferStride == 2)
            tris[gtid] = Load3x16BitIndices(VertexAndIndexData, baseIndex);
		else if (Meshlet.IndexBufferStride == 1)
            tris[gtid] = Load3x8BitIndices(VertexAndIndexData, baseIndex);
		else
            tris[gtid] = VertexAndIndexData.Load3(baseIndex);
    }
}