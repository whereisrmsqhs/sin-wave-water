#include "Common.hlsli" // 쉐이더에서도 include 사용 가능

Texture2D g_texture0 : register(t0);
TextureCube g_diffuseCube : register(t1);
TextureCube g_specularCube : register(t2);
SamplerState g_sampler : register(s0);

cbuffer BasicPixelConstantBuffer : register(b0)
{
    float3 eyeWorld;
    bool useTexture;
    Material material;
    Light light[MAX_LIGHTS];
};

float3 SchlickFrestnel(float3 normal, float3 toEye)
{
    float frestnelR0 = float3(0.02, 0.02, 0.02);
    
    float normalDotView = saturate(dot(normal, toEye));
    
    float f0 = 1.0f - normalDotView;
    
    return frestnelR0 + (1.0 - frestnelR0) * pow(f0, 4.0);
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 toEye = normalize(eyeWorld - input.posWorld);
    float3 color = float3(0.01, 0.02, 0.01);
    // Pink
    // float3 color = float3(0.5, 0.2, 0.2);
    
    int i = 0;

    [unroll]
    for (i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        color += ComputeDirectionalLight(light[i], material, input.normalWorld, toEye);
    }

    [unroll]
    for (i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
    {
        color += ComputePointLight(light[i], material, input.posWorld, input.normalWorld, toEye);
    }

    [unroll]
    for (i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        color += ComputeSpotLight(light[i], material, input.posWorld, input.normalWorld, toEye);
    }
    
    // Calculate distance fog.
    float depth = length(input.posWorld);
    float density = 0.02;
    float fogColor = float3(0.9, 0.6, 0.6);
    float fogFactor = exp(-pow(depth * density, 2));

    // Calculate image based lighting.
    float4 diffuse = g_diffuseCube.Sample(g_sampler, input.normalWorld);
    diffuse.xyz *= material.diffuse;
    float4 specular = g_specularCube.Sample(g_sampler, reflect(-toEye, input.normalWorld));
    specular *= pow((specular.x + specular.y + specular.z) / 3.0, material.shininess);
    specular.xyz *= (material.specular * 0.75);
        
    // Calculate frestnel effect
    float3 frestnel = SchlickFrestnel(input.normalWorld, toEye);
    specular.xyz *= frestnel;
    
    float3 lighting = diffuse + specular;
 
    float3 finalColor = saturate(lerp(fogColor, lighting, fogFactor));
    
    return float4(finalColor, 1.0);
    
    // Tone mapping + gamma (있으면 더 좋음)
    finalColor = finalColor / (1.0 + finalColor);  // Reinhard
    finalColor = pow(finalColor, 1.0/2.2);         // Gamma
    
    return float4(finalColor, 1.0);
    
    // return useTexture
    //      ? float4(finalColor, 1.0) * g_texture0.Sample(g_sampler, input.texcoord)
    //      : float4(finalColor, 1.0) + diffuse + specular;
}
