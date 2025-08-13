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

float4 main(PixelShaderInput input) : SV_TARGET
{
    float3 toEye = normalize(eyeWorld - input.posWorld);
    float3 color = float3(0.0, 0.0, 0.0);
    
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

    float4 diffuse = g_diffuseCube.Sample(g_sampler, input.normalWorld);
    diffuse.xyz *= material.diffuse;
    float4 specular = g_specularCube.Sample(g_sampler, reflect(-toEye, input.normalWorld));
    specular.xyz *= material.specular;
    
    return float4(color, 1.0) + diffuse + specular;
    
    // return useTexture
       //  ? float4(color, 1.0) * g_texture0.Sample(g_sampler, input.texcoord)
       //  : float4(color, 1.0);
}
