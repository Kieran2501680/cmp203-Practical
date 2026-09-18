#include "../LightingFunctionsHLSL/LSSharedStructs.hlsli"
#include "../LightingFunctionsHLSL/LightFunctions.hlsli"

cbuffer FRAMEDATA : register(b0, space0)
{
    FrameData Frame;
}

cbuffer INOUT : register(b1)
{
    uint GBufferFirstIndex;
    uint OutputUAV;
}

[numthreads(16, 16, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	Texture2D AlbedoBuffer = ResourceDescriptorHeap[GBufferFirstIndex];
	Texture2D NormalBuffer =  ResourceDescriptorHeap[GBufferFirstIndex+1];
	Texture2D<float> DepthBuffer =  ResourceDescriptorHeap[GBufferFirstIndex+2];

	RWTexture2D<float4> Output = ResourceDescriptorHeap[OutputUAV];

    StructuredBuffer<Light> Lights = ResourceDescriptorHeap[Frame.LightBufferSRV_IDX];

    float shadowMapBias = 0.005f;

    float4 LightComponent = Frame.AmbientLight;
    float4 SpecularComponent = float4(0, 0, 0, 0);

    uint2 TextureSize;
	AlbedoBuffer.GetDimensions(TextureSize.x, TextureSize.y);

	// Out of bounds check
    if (TextureSize.x < DTid.x || TextureSize.y < DTid.y) return;

    
	float2 uv = float2(float2(DTid.xy) / TextureSize);
	float3 normal = normalize(NormalBuffer.Load(uint3(DTid.xy, 0)).xyz);
	float4 worldpos = float4(WorldPosFromDepth(uv, DepthBuffer.Load(uint3(DTid.xy,0)), Frame),1);
    float3 object_colour = AlbedoBuffer.Load(uint3(DTid.xy, 0)).xyz;
	float SpecularStrength = AlbedoBuffer.Load(uint3(DTid.xy, 0)).w;
    float SpecularPower = NormalBuffer.Load(uint3(DTid.xy, 0)).w;


    for (uint i = 0; i < Frame.LightCount; i++)
    {
        float3 lightDirection = -normalize(Lights[i].LightDirection);
        float3 viewDirection = normalize(Frame.CameraPosition - worldpos);

        if (Lights[i].Type == LightType::LightDirectional)
        {
            Texture2D DepthTexture = ResourceDescriptorHeap[Lights[i].ShadowMapID];
            SamplerState DepthSampler = SamplerDescriptorHeap[Lights[i].ShadowMapSamplerID];

            float4 lightViewPos = mul(Lights[i].LightView, worldpos);
            lightViewPos = mul(Lights[i].LightProjection, lightViewPos);

            // Calculate the projected texture coordinates.
            float2 pTexCoord = getProjectiveCoords(lightViewPos);

            // Shadow test. Is or isn't in shadow
            if (hasDepthData(pTexCoord))
            {
                // Has depth map data
                if (!isInShadow(DepthTexture, pTexCoord, lightViewPos, shadowMapBias, DepthSampler))
                {
                  // is NOT in shadow, therefore light
                  LightComponent += Lights[i].DiffuseColour * calculateLightingIntensity(normal, lightDirection);
                  SpecularComponent += Lights[i].DiffuseColour * calcSpecularIntensity(SpecularPower, SpecularStrength, viewDirection, normal, lightDirection);
				}
            }
        }
        else
        {
            float3 lightVector = Lights[i].LightPosition - worldpos;
            float amplitude = length(lightVector);
            lightVector = normalize(lightVector);

            if (amplitude < Lights[i].CutOffDistance)
            {
                float attune = 1.f / (Lights[i].ConstantAttenuation + Lights[i].LinearAttenuation * amplitude + Lights[i].SquareAttenuation * amplitude * amplitude);

                if (Lights[i].Type == LightSpot)
                {
                    float dot_p = dot(lightDirection, lightVector);

                    if (dot_p < Lights[i].OuterCone)
                    {
                        float falloff = calculateSpotFalloffFactor(dot_p, Lights[i]);
                        LightComponent += Lights[i].DiffuseColour * Lights[i].DiffuseColour * calculateLightingIntensity(normal, lightVector) * attune * falloff;
                        SpecularComponent += Lights[i].DiffuseColour * Lights[i].DiffuseColour * calcSpecularIntensity(SpecularPower, SpecularStrength, viewDirection, normal, lightDirection) * attune * falloff;
                    }

                    Texture2D DepthTexture = ResourceDescriptorHeap[Lights[i].ShadowMapID];
                    SamplerState DepthSampler = SamplerDescriptorHeap[Lights[i].ShadowMapSamplerID];

                    float4 lightViewPos = mul(Lights[i].LightView, worldpos);
                    lightViewPos = mul(Lights[i].LightProjection, lightViewPos);

                    // Calculate the projected texture coordinates.
                    float2 pTexCoord = getProjectiveCoords(lightViewPos);

                    // Shadow test. Is or isn't in shadow
                    if (hasDepthData(pTexCoord))
                    {
                        // Has depth map data
                        if (!isInShadow(DepthTexture, pTexCoord, lightViewPos, shadowMapBias, DepthSampler))
                        {
                            LightComponent += Lights[i].DiffuseColour * calculateLightingIntensity(normal, lightVector) * attune;
                            SpecularComponent += Lights[i].DiffuseColour * calcSpecularIntensity(SpecularPower, SpecularStrength, viewDirection, normal, lightDirection) * attune;
                        }
                    }
                }
                else //point light
                {
                    Texture2D DepthTexture = ResourceDescriptorHeap[Lights[i].ShadowMapID];
                    SamplerState DepthSampler = SamplerDescriptorHeap[Lights[i].ShadowMapSamplerID];

                    float4 lightViewPos = mul(Lights[i].LightView, worldpos);
                    lightViewPos = mul(Lights[i].LightProjection, lightViewPos);

                    // Calculate the projected texture coordinates.
                    float2 pTexCoord = getProjectiveCoords(lightViewPos);

                    // Shadow test. Is or isn't in shadow
                    if (hasDepthData(pTexCoord))
                    {
                        // Has depth map data
                        if (!isInShadow(DepthTexture, pTexCoord, lightViewPos, shadowMapBias, DepthSampler))
                        {
                            LightComponent += Lights[i].DiffuseColour * calculateLightingIntensity(normal, lightDirection) * attune;
                            SpecularComponent += Lights[i].DiffuseColour * calcSpecularIntensity(SpecularPower, SpecularStrength, viewDirection, normal, lightDirection) * attune;
                        }
                    }
                }
            }
        }
    }

    Output[DTid.xy] = float4(saturate(LightComponent) * object_colour,1);
}