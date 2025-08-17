#include "Common.hlsli" // 쉐이더에서도 include 사용 가능

TextureCube g_diffuseCube : register(t0);
TextureCube g_specularCube : register(t1);
SamplerState g_sampler : register(s0);

cbuffer TimeBuffer : register(b1)
{
    float time;
    float3 padding;
};

float3x3 RotationX(float angle)
{
    float c = cos(angle);
    float s = sin(angle);

    return float3x3(
        1, 0, 0,
        0, c, -s,
        0, s, c
    );
}

float4 main(PixelShaderInput input) : SV_TARGET
{
    float angle = time * 0.01; // 회전 속도 조절
    float s = sin(angle);
    float c = cos(angle);

    float3 dir = normalize(input.posWorld); // 또는 reflect 벡터
    float3 rotatedDir;
    rotatedDir.x = c * dir.x + s * dir.z;
    rotatedDir.y = dir.y;
    rotatedDir.z = -s * dir.x + c * dir.z;

    return g_specularCube.Sample(g_sampler, rotatedDir);

}