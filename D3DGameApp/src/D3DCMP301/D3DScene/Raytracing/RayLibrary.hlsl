
#include "../LightsAndShadows/Vertex.h"
#include "../LightsAndShadows/LightingFunctionsHLSL/LSSharedStructs.hlsli"
#include "../LightsAndShadows/LightingFunctionsHLSL/LightFunctions.hlsli"

ConstantBuffer<FrameData> g_sceneCB : register(b0);

cbuffer InOut : register(b1)
{
    uint InGBufferIDX;
    uint OutUavIDX;
};


// Generate a ray in world space for a camera pixel corresponding to an index from the dispatched 2D grid.
inline void GenerateCameraRay(uint2 index, out float3 origin, out float3 direction)
{
    float2 screenPos = float2(index) / DispatchRaysDimensions().xy;

    float4 ndcPos = float4(
        screenPos.x * 2.0 - 1.0,
        (1.0 - screenPos.y) * 2.0 - 1.0,
        0,
        1.0
    );

    float4 viewPos = mul(g_sceneCB.InvProjection, ndcPos);
    viewPos = viewPos / viewPos.w;

    float4 worldPos = mul(g_sceneCB.InvView, viewPos);
   

    origin = g_sceneCB.CameraPosition.xyz;
    direction = normalize(worldPos.xyz - origin);
}

// Retrieve hit world position.
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct RayPayload
{
    float4 color;
};

[shader("raygeneration")]
void MyRaygenShader()
{
    float3 rayDir;
    float3 origin;

    // Generate a ray for a camera pixel corresponding to an index from the dispatched 2D grid.
    GenerateCameraRay(DispatchRaysIndex().xy, origin, rayDir);

    // Trace the ray.
    // Set the ray's extents.
    RayDesc ray;
    ray.Origin = origin;
    ray.Direction = rayDir;
    // Set TMin to a non-zero small value to avoid aliasing issues due to floating - point errors.
    // TMin should be kept small to prevent missing geometry at close contact areas.
    ray.TMin = 0.001;
    ray.TMax = 10000.0;
    RayPayload payload = { float4(0, 0, 0, 0) };

    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[g_sceneCB.RaytracingSceneSRV_IDX];

    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xFF, 0, 1, 0, ray, payload);

    RWTexture2D<float4> RenderTarget = ResourceDescriptorHeap[OutUavIDX];

    // Write the raytraced color to the output texture.
    //RenderTarget[DispatchRaysIndex().xy] = float4(0.4f, 0.2f, 0.01f, 1.0f);
    RenderTarget[DispatchRaysIndex().xy] = payload.color;
}

float4 unpack8BitRGBAtoFloat4Norm(uint rgba)
{
    float4 colour = float4(rgba >> 24u, (rgba & 0x00FF0000) >> 16u, (rgba & 0x0000FF00) >> 8u, rgba & 0x000000FF);
    colour *= 0.00392156862f;
	return colour;
}

[shader("closesthit")]
void MyClosestHitShader(inout RayPayload payload, in MyAttributes attr)
{
    float3 hitPosition = HitWorldPosition();

    //StructuredBuffer<Light> Lights =  ResourceDescriptorHeap[g_sceneCB.LighBufferIDX];
    StructuredBuffer<PrimitiveStridesAndOffsets> PrimitiveOffsets = ResourceDescriptorHeap[g_sceneCB.GeometryBufferSRV_IDX];
    ByteAddressBuffer VertexAndIndexData = ResourceDescriptorHeap[g_sceneCB.VertexIndexBufferSRV_IDX];
    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[g_sceneCB.InstanceDataSRV_IDX];

    PrimitiveStridesAndOffsets primitive = PrimitiveOffsets[InstanceIndex() + GeometryIndex()];
    InstanceData instance = Instances[InstanceID()];

    Texture2D<float4> InstanceTexture = ResourceDescriptorHeap[NonUniformResourceIndex(instance.AlbedoTextureID)];
    SamplerState InstanceSampler = ResourceDescriptorHeap[NonUniformResourceIndex(instance.SamplerID)];

    // Get the base index of the triangle's first 16 bit index.
    uint indexSizeInBytes = primitive.IndexBufferStride;
    uint indicesPerTriangle = 3;
    uint triangleIndexStride = indicesPerTriangle * indexSizeInBytes;
    uint baseIndex = primitive.IndexBufferOffsetInBytes + PrimitiveIndex() * triangleIndexStride;

    //// Load up 3 16 bit indices for the triangle.
    uint3 indices;
    if(primitive.IndexBufferStride == 2)
        indices = Load3x16BitIndices(VertexAndIndexData, baseIndex);
    else if (primitive.IndexBufferStride == 1)
		indices = Load3x8BitIndices(VertexAndIndexData, baseIndex);
    else
        indices = VertexAndIndexData.Load3(baseIndex);

    Vertex vertices[3];

    vertices[0] = VertexAndIndexData.Load<Vertex>(primitive.VertexBufferOffsetInBytes + primitive.VertexBufferStride*indices[0]);
    vertices[1] = VertexAndIndexData.Load<Vertex>(primitive.VertexBufferOffsetInBytes + primitive.VertexBufferStride*indices[1]);
    vertices[2] = VertexAndIndexData.Load<Vertex>(primitive.VertexBufferOffsetInBytes + primitive.VertexBufferStride*indices[2]);

    float2 uv = vertices[0].uv +
        attr.barycentrics.x * (vertices[1].uv - vertices[0].uv) +
        attr.barycentrics.y * (vertices[2].uv - vertices[0].uv);


    payload.color = InstanceTexture.SampleLevel(InstanceSampler, uv, 0);
};

[shader("miss")]
void MyMissShader(inout RayPayload payload)
{
    float4 background = float4(0.0f, 0.2f, 0.4f, 1.0f);
    payload.color = background;
}

//Shadow Rays

struct ShadowHitInfo
{
    bool isHit;
};

struct Attributes
{
    float2 uv;
};

bool ShootShadowRay(float3 orig, float TMin_Sbias, Light light)
{
    ShadowHitInfo hitInfo = { false };

    RayDesc       rayShadow;
    rayShadow.TMin = TMin_Sbias;
    rayShadow.Origin = orig;

    switch (light.Type)
    {
    case LightDirectional:
        rayShadow.TMax = 10000.0;
        rayShadow.Direction = normalize(-light.LightDirection);
		break;
    default:
        rayShadow.TMax = length(light.LightPosition - orig);
        rayShadow.Direction = normalize(light.LightPosition - orig);
    }
    

    uint rayFlags = RAY_FLAG_NONE;
    //for opaque geometry nothing will pass though, so we can skip closest hit shader
//|
//   RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;

    RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[g_sceneCB.RaytracingSceneSRV_IDX];

    // Trace our ray. 
    TraceRay(Scene, rayFlags, 0xFF, 1, 0, 1, rayShadow, hitInfo);

    return hitInfo.isHit;
}

[shader("closesthit")]
void ShadowClosestHit(inout ShadowHitInfo hit, Attributes bary)
{
    hit.isHit = true;
}

[shader("miss")]
void ShadowMiss(inout ShadowHitInfo hit : SV_RayPayload)
{
    hit.isHit = false;
}

//Ambinet Occlusion Rays

struct AORayPayload {
    float aoVal;  // Stores 0 on a ray hit, 1 on ray miss
};

float shootAmbientOcclusionRay(float3 orig, float3 dir,
    float minT, float maxT)
{
    // Setup AO payload.  By default, assume our AO ray will *hit* (value = 0)
    AORayPayload  rayPayload = { 0.0f };

    // Describe the ray we're shooting
    RayDesc       rayAO = { orig, minT, dir, maxT };

    // We're going to tell our ray to never run the closest-hit shader and to
    //     stop as soon as we find *any* intersection
    uint rayFlags = RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
        RAY_FLAG_SKIP_CLOSEST_HIT_SHADER;

	RaytracingAccelerationStructure Scene = ResourceDescriptorHeap[g_sceneCB.RaytracingSceneSRV_IDX];

    // Trace our ray. 
    TraceRay(Scene, rayFlags, 0xFF, 0, 0, 0, rayAO, rayPayload);

    // Return our AO value out of the ray payload.
    return rayPayload.aoVal;
}

[shader("miss")]
void AoMiss(inout AORayPayload rayData)
{
    rayData.aoVal = 1.0f;
}

[shader("anyhit")]
void AoAnyHit(inout AORayPayload rayData, MyAttributes attribs)
{
    //if (alphaTestFails(attribs)) IgnoreHit();
	//IgnoreHit();
}

[shader("closesthit")]
void AoClosestHit(inout AORayPayload, MyAttributes attr)
{
}

[shader("raygeneration")]
void AmbientOcclusionRaygen()
{
    // Where is this thread's ray on screen?
    uint2 pixIdx = DispatchRaysIndex();
    uint2 numPix = DispatchRaysDimensions();

    Texture2D AlbedoBuffer = ResourceDescriptorHeap[InGBufferIDX];
    Texture2D NormalBuffer = ResourceDescriptorHeap[InGBufferIDX + 1];
    Texture2D<float> DepthBuffer = ResourceDescriptorHeap[InGBufferIDX + 2];

	StructuredBuffer<Light> Lights = ResourceDescriptorHeap[g_sceneCB.LightBufferSRV_IDX];
    RWTexture2D<float4> Output = ResourceDescriptorHeap[OutUavIDX];

    // Initialize a random seed, per-pixel and per-frame
    uint randSeed = initRand(pixIdx.x + pixIdx.y * numPix.x, g_sceneCB.FrameIndex);

    // Load the position and normal from our g-buffer
    float4 worldPos = float4(WorldPosFromDepth(float2(pixIdx) / numPix, DepthBuffer[pixIdx], g_sceneCB), 1);
    float3 worldNorm = NormalBuffer[pixIdx].xyz;

    //
    float SpecularStrength = AlbedoBuffer[pixIdx].w;
    float SpecularPower = NormalBuffer[pixIdx].w;

    // Default ambient occlusion value if we hit the background
    float aoVal = 1.0f;

    // worldPos.w == 0 for background pixels; only shoot AO rays elsewhere
    if (worldPos.w != 0.0f)
    {
        // Random ray, sampled on cosine-weighted hemisphere around normal 
        float3 worldDir = getCosHemisphereSample(randSeed, worldNorm);

		float  gMinT = 0.01f;          // Start just above the surface to avoid self-intersection
		float  gAORadius = 2.0f;       // How far to trace our AO rays

        // Shoot our ambient occlusion ray and update the final AO value
        aoVal = shootAmbientOcclusionRay(worldPos.xyz, worldDir, gMinT, gAORadius);
    }

	float4 LightValue = float4(0, 0, 0, 0);

    for(int i =0; i < g_sceneCB.LightCount; i++)
    {
	    if(!ShootShadowRay(worldPos.xyz,0.1, Lights[i]))
	    {
            LightValue += BlinPhongShading(Lights[i], g_sceneCB.CameraPosition, worldPos.xyz, worldNorm, SpecularStrength, SpecularPower);
	    }
    }

    LightValue+= g_sceneCB.AmbientLight * aoVal;

	Output[pixIdx] = AlbedoBuffer[pixIdx] * LightValue;
}



