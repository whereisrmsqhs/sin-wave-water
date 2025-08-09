#include "Common.hlsli" // 쉐이더에서도 include 사용 가능

Texture2D g_texture0 : register(t0);
Texture2D g_texture1 : register(t1);
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
    float3 color = float3(0.01, 0.05, 0.9);

    [unroll]
    for (int i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        color += ComputeDirectionalLight(light[i], material, input.normalWorld, toEye);
    }

    [unroll]
    for (int i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; ++i)
    {
        color += ComputePointLight(light[i], material, input.posWorld, input.normalWorld, toEye);
    }

    [unroll]
    for (int i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        color += ComputeSpotLight(light[i], material, input.posWorld, input.normalWorld, toEye);
    }

    // ======= Distance Fog 추가 =======
    float fogStart = 20.0;
    float fogEnd = 100.0;
    float3 fogColor = float3(0.7, 0.8, 0.9); // or whatever color suits your scene

    float fogDistance = length(input.posWorld - eyeWorld);
    float fogFactor = saturate((fogDistance - fogStart) / (fogEnd - fogStart));

    color = lerp(color, fogColor, fogFactor);
    // ==================================

    return useTexture
        ? float4(color, 1.0) * g_texture0.Sample(g_sampler, input.texcoord)
        : float4(color, 1.0);
}
