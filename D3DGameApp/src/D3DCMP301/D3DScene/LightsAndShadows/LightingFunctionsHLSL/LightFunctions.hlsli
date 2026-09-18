#pragma once

#include "LSSharedStructs.hlsli"

#ifndef LIGHTFUNCTIONS_HLSLI
#define LIGHTFUNCTIONS_HLSLI

uint3 Load3x16BitIndices(ByteAddressBuffer buffer, uint offsetBytes)
{
    uint3 indices;

    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint dwordAlignedOffset = offsetBytes & ~3;
    const uint2 four16BitIndices = buffer.Load2(dwordAlignedOffset);

    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes)
    {
        indices.x = four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 16) & 0xffff;
        indices.z = four16BitIndices.y & 0xffff;
    }
    else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
    {
        indices.x = (four16BitIndices.x >> 16) & 0xffff;
        indices.y = four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 16) & 0xffff;
    }

    return indices;
}

uint3 Load3x8BitIndices(ByteAddressBuffer buffer, uint offsetBytes)
{
    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // We need to read three 8-bit indices (3 bytes).
    // We must load 8 bytes (2x uints) to handle any alignment case,
    // e.g., if the 3-byte sequence starts at byte 2 or 3 of a 4-byte dword.
    // Aligned offset 0: { 0 1 2 | 3 } { 4 5 6 | 7 }
    // offsetBytes = 0: needs bytes [0, 1, 2]
    // offsetBytes = 1: needs bytes [1, 2, 3]
    // offsetBytes = 2: needs bytes [2, 3, 4]
    // offsetBytes = 3: needs bytes [3, 4, 5]
    // Loading 8 bytes from the aligned offset covers all 4 cases.
    const uint dwordAlignedOffset = offsetBytes & ~3;
    const uint2 twoUints = buffer.Load2(dwordAlignedOffset);

    // Get the byte offset *within* the 8-byte (64-bit) chunk we just read
    // This will be 0, 1, 2, or 3
    const uint offsetInChunk = offsetBytes - dwordAlignedOffset;

    // Unpack all 8 bytes into a temporary array
    // This is often compiled efficiently by the shader compiler
    uint bytes[8];
    bytes[0] = twoUints.x & 0xff;
    bytes[1] = (twoUints.x >> 8) & 0xff;
    bytes[2] = (twoUints.x >> 16) & 0xff;
    bytes[3] = (twoUints.x >> 24) & 0xff;
    bytes[4] = twoUints.y & 0xff;
    bytes[5] = (twoUints.y >> 8) & 0xff;
    bytes[6] = (twoUints.y >> 16) & 0xff;
    bytes[7] = (twoUints.y >> 24) & 0xff;

    // Select the three correct bytes based on our original offset
    // The index [offsetInChunk + 2] is safe because:
    // - Max offsetInChunk is 3.
    // - Max index accessed is bytes[3 + 2] = bytes[5], which is valid (0-7).
    return uint3(bytes[offsetInChunk], bytes[offsetInChunk + 1], bytes[offsetInChunk + 2]);
}


float3 WorldPosFromDepth(float2 UV, float depth, FrameData Frame)
{
    float4 ndcPos = float4(
        UV.x * 2.0 - 1.0,
        (1.0 - UV.y) * 2.0 - 1.0,
        depth,
        1.0
    );

    float4 viewPos = mul(Frame.InvProjection, ndcPos);
    viewPos = viewPos / viewPos.w;

    float4 worldPos = mul(Frame.InvView, viewPos);
    return worldPos.xyz;
}

// Generates a seed for a random number generator from 2 inputs plus a backoff
uint initRand(uint val0, uint val1, uint backoff = 16)
{
    uint v0 = val0, v1 = val1, s0 = 0;

	[unroll]
    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float nextRand(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

// Utility function to get a vector perpendicular to an input vector 
//    (from "Efficient Construction of Perpendicular Vectors Without Branching")
float3 getPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

// Get a cosine-weighted random vector centered around a specified normal direction.
float3 getCosHemisphereSample(inout uint randSeed, float3 hitNorm)
{
	// Get 2 random numbers to select our sample with
    float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

	// Cosine weighted hemisphere sample from RNG
    float3 bitangent = getPerpendicularVector(hitNorm);
    float3 tangent = cross(bitangent, hitNorm);
    float r = sqrt(randVal.x);
    float phi = 2.0f * 3.14159265f * randVal.y;

	// Get our cosine-weighted hemisphere lobe sample direction
    return tangent * (r * cos(phi).x) + bitangent * (r * sin(phi)) + hitNorm.xyz * sqrt(1 - randVal.x);
}

// This function tests if the alpha test fails, given the attributes of the current hit. 
//   -> Can legally be called in a DXR any-hit shader or a DXR closest-hit shader, and 
//      accesses Falcor helpers and data structures to extract and perform the alpha test.
//bool alphaTestFails(BuiltinIntersectionAttribs attribs)
//{
//	// Run a Falcor helper to extract the current hit point's geometric data
//    VS_OUT vsOut = getVertexAttributes(PrimitiveIndex(), attribs);

//	// Extracts the diffuse color from the material (the alpha component is opacity)
//    float4 baseColor = sampleTexture(gMaterial.resources.baseColor, gMaterial.resources.samplerState,
//		vsOut.texC, gMaterial.baseColor, EXTRACT_DIFFUSE_TYPE(gMaterial.flags));

//	// Test if this hit point fails a standard alpha test.  
//    return (baseColor.a < gMaterial.alphaThreshold);
//}

// Is the gemoetry in our shadow map
bool hasDepthData(float2 uv)
{
    if (uv.x < 0.f || uv.x > 1.f || uv.y < 0.f || uv.y > 1.f)
    {
        return false;
    }
    return true;
}

bool isInShadow(Texture2D sMap, float2 uv, float4 lightViewPosition, float bias, SamplerState shadowSampler)
{
    // Sample the shadow map (get depth of geometry)
    float depthValue = sMap.Sample(shadowSampler, uv).r;
	// Calculate the depth from the light.
    float lightDepthValue = lightViewPosition.z / lightViewPosition.w;
    lightDepthValue += bias;

	// Compare the depth of the shadow map value and the depth of the light to determine whether to shadow or to light this pixel.
    if (lightDepthValue > depthValue)
    {
        return false;
    }
    return true;
}

float2 getProjectiveCoords(float4 lightViewPosition)
{
    // Calculate the projected texture coordinates.
    float2 projTex = lightViewPosition.xy / lightViewPosition.w;
    projTex *= float2(0.5, -0.5);
    projTex += float2(0.5f, 0.5f);
    return projTex;
}

float calculateSpotFalloffFactor(float alpha, Light light)
{
    float falloffFactor = pow((alpha - light.OuterCone) / (light.InnerCone - light.OuterCone), light.FalloffPower);
    return falloffFactor;
}

// Calculate lighting intensity based on direction and normal. Combine with light colour.

float calculateLightingIntensity(float3 normal, float3 lightvector)
{
    float intensity = saturate(dot(normal, normalize(lightvector)));
    return intensity;
}

//calculate specular lightting based on view angle Phong model

float calcSpecularIntensity(float SpecularPower, float SpecularStrength, float3 inputView, float3 inputNormal, float3 lightVector)
{
    float3 reflectDir = reflect(-lightVector, inputNormal);
    float spec = pow(max(dot(inputView, reflectDir), 0.0), SpecularPower);
    return spec * SpecularStrength;

   // float3 halfway = normalize(lightVector + inputView);
   // float specularIntensity = pow(max(dot(inputNormal, halfway), 0.0), SpecularPower);
   // return specularIntensity;
    
}

float4 BlinPhongShading(Light light, float3 CameraPos, float3 WorldPos, float3 Normal, float SpecularStrength, float SpecularPower)
{
    float3 lightDirection = -normalize(light.LightDirection);
    float3 viewDirection = normalize(CameraPos - WorldPos);

    float4 LightComponent = float4(0, 0, 0, 1);
    float4 SpecularComponent = float4(0, 0, 0, 1);

    if (light.Type == LightType::LightDirectional)
    {
        LightComponent += light.DiffuseColour * calculateLightingIntensity(Normal, lightDirection);
        SpecularComponent += light.DiffuseColour * calcSpecularIntensity(SpecularPower, SpecularStrength, viewDirection, Normal, lightDirection);
    }
    else
    {
        float3 lightVector = light.LightPosition - WorldPos;
        float amplitude = length(lightVector);
        lightVector = normalize(lightVector);

        if (amplitude < light.CutOffDistance)
        {
            float attune = 1.f / (light.ConstantAttenuation + light.LinearAttenuation * amplitude + light.SquareAttenuation * amplitude * amplitude);

            if (light.Type == LightSpot)
            {
                float dot_p = dot(lightDirection, lightVector);

                if (dot_p < light.OuterCone)
                {
                    float falloff = calculateSpotFalloffFactor(dot_p, light);
                    LightComponent += light.DiffuseColour * light.DiffuseColour * calculateLightingIntensity(Normal, lightVector) * attune * falloff;
                    SpecularComponent += light.DiffuseColour * light.DiffuseColour * calcSpecularIntensity(SpecularPower, SpecularStrength, viewDirection, Normal, lightDirection) * attune * falloff;
                }


                float4 lightViewPos = mul(light.LightView, WorldPos);
                lightViewPos = mul(light.LightProjection, lightViewPos);

                // Calculate the projected texture coordinates.
                float2 pTexCoord = getProjectiveCoords(lightViewPos);

                LightComponent += light.DiffuseColour * calculateLightingIntensity(Normal, lightVector) * attune;
                SpecularComponent += light.DiffuseColour * calcSpecularIntensity(SpecularPower, SpecularStrength, viewDirection, Normal, lightDirection) * attune;
         
            }
            else //point light
            {
                float4 lightViewPos = mul(light.LightView, WorldPos);
                lightViewPos = mul(light.LightProjection, lightViewPos);

                // Calculate the projected texture coordinates.
                float2 pTexCoord = getProjectiveCoords(lightViewPos);

                LightComponent += light.DiffuseColour * calculateLightingIntensity(Normal, lightDirection) * attune;
                SpecularComponent += light.DiffuseColour * calcSpecularIntensity(SpecularPower, SpecularStrength, viewDirection, Normal, lightDirection) * attune;
            }
        }
    }

    return saturate(LightComponent + SpecularComponent);
};

#endif
