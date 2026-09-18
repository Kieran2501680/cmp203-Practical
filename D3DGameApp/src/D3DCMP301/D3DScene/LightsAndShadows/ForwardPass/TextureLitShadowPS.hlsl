#include "../LightingFunctionsHLSL/LSSharedStructs.hlsli"
#include "../LightingFunctionsHLSL/LightFunctions.hlsli"

cbuffer FRAMEDATA : register(b0, space0)
{
    FrameData Frame;
}

cbuffer INOUT : register(b1)
{
    uint InstanceId;
    uint outputIDX;
}

float4 main(VS_OUT input) : SV_Target0
{
    //return float4(1, 1, 1, 1);

    StructuredBuffer<InstanceData> Instances = ResourceDescriptorHeap[Frame.InstanceDataSRV_IDX];
    StructuredBuffer<Light> Lights = ResourceDescriptorHeap[Frame.LightBufferSRV_IDX];

    Texture2D texture = ResourceDescriptorHeap[Instances[InstanceId].AlbedoTextureID];
    SamplerState TextureSampler = SamplerDescriptorHeap[Instances[InstanceId].SamplerID];

    float shadowMapBias = 0.005f;

    float4 LightComponent = Frame.AmbientLight;
    float4 SpecularComponent = float4(0,0,0,0);

    float4 object_colour = texture.Sample(TextureSampler, input.uv);

    for(uint i =0; i < Frame.LightCount; i++)
    {
        float3 lightDirection = -normalize(Lights[i].LightDirection);
        float3 viewDirection = normalize(Frame.CameraPosition - input.worldpos);

        if (Lights[i].Type == LightType::LightDirectional)
        {
            Texture2D DepthTexture = ResourceDescriptorHeap[Lights[i].ShadowMapID];
            SamplerState DepthSampler = SamplerDescriptorHeap[Lights[i].ShadowMapSamplerID];

            float4 lightViewPos = mul(Lights[i].LightView, input.worldpos);
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
                    LightComponent +=  Lights[i].DiffuseColour * calculateLightingIntensity(input.normal, lightDirection);
                    SpecularComponent +=  Lights[i].DiffuseColour * calcSpecularIntensity(Instances[Frame.InstanceDataSRV_IDX].SpecularData.specularPower, Instances[Frame.InstanceDataSRV_IDX].SpecularData.specularStrength, viewDirection, input.normal, lightDirection);
                }
            }
        }
        else
        {
            float3 lightVector = Lights[i].LightPosition - input.worldpos;
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
                        LightComponent += Lights[i].DiffuseColour * Lights[i].DiffuseColour * calculateLightingIntensity(input.normal, lightVector) * attune * falloff;
                        SpecularComponent += Lights[i].DiffuseColour * Lights[i].DiffuseColour * calcSpecularIntensity(Instances[Frame.InstanceDataSRV_IDX].SpecularData.specularPower, Instances[Frame.InstanceDataSRV_IDX].SpecularData.specularStrength, viewDirection, input.normal, lightDirection) * attune * falloff;
                    }

                    Texture2D DepthTexture = ResourceDescriptorHeap[Lights[i].ShadowMapID];
                    SamplerState DepthSampler = SamplerDescriptorHeap[Lights[i].ShadowMapSamplerID];

                    float4 lightViewPos = mul(Lights[i].LightView, input.worldpos);
                    lightViewPos = mul(Lights[i].LightProjection, lightViewPos);

                    // Calculate the projected texture coordinates.
                    float2 pTexCoord = getProjectiveCoords(lightViewPos);

                    // Shadow test. Is or isn't in shadow
                    if (hasDepthData(pTexCoord))
                    {
                        // Has depth map data
                        if (!isInShadow(DepthTexture, pTexCoord, lightViewPos, shadowMapBias, DepthSampler))
                        {
                            LightComponent += Lights[i].DiffuseColour * calculateLightingIntensity(input.normal, lightVector) * attune;
                            SpecularComponent += Lights[i].DiffuseColour * calcSpecularIntensity(Instances[Frame.InstanceDataSRV_IDX].SpecularData.specularPower, Instances[Frame.InstanceDataSRV_IDX].SpecularData.specularStrength, viewDirection, input.normal, lightDirection) * attune;
                        }
                    }
                }
				else //point light
                {
                    Texture2D DepthTexture = ResourceDescriptorHeap[Lights[i].ShadowMapID];
                    SamplerState DepthSampler = SamplerDescriptorHeap[Lights[i].ShadowMapSamplerID];

                    float4 lightViewPos = mul(Lights[i].LightView, input.worldpos);
                    lightViewPos = mul(Lights[i].LightProjection, lightViewPos);

                    // Calculate the projected texture coordinates.
                    float2 pTexCoord = getProjectiveCoords(lightViewPos);

                    // Shadow test. Is or isn't in shadow
                    if (hasDepthData(pTexCoord))
                    {
                        // Has depth map data
                        if (!isInShadow(DepthTexture, pTexCoord, lightViewPos, shadowMapBias, DepthSampler))
                        {
                            LightComponent += Lights[i].DiffuseColour * calculateLightingIntensity(input.normal, lightDirection) * attune;
                            SpecularComponent += Lights[i].DiffuseColour * calcSpecularIntensity(Instances[Frame.InstanceDataSRV_IDX].SpecularData.specularPower, Instances[Frame.InstanceDataSRV_IDX].SpecularData.specularStrength, viewDirection, input.normal, lightDirection) * attune;
                        }
                    }
                }
            }
        }
    }

    return saturate(LightComponent+SpecularComponent) * object_colour;
}