// safe header (ASCII only)
#include "Common.hlsli"

#define MAX_WAVES 24
#define PI 3.1415926

cbuffer BasicVertexConstantBuffer : register(b0)
{
    matrix model;
    matrix invTranspose;
    matrix view;
    matrix projection;
};

cbuffer TimeBuffer : register(b1)
{
    float time;
    float3 padding;
};

struct Wave
{
    float xDirection;
    float zDirection;
    float speed;
    float dummy; // padding 맞추기 위해 필요
};

cbuffer WaveBuffer : register(b2)
{
    Wave waves[MAX_WAVES];
};

float Hash(float2 p) {
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}

PixelShaderInput main(VertexShaderInput input)
{
    // 모델(Model) 행렬은 모델 자신의 원점에서 
    // 월드 좌표계에서의 위치로 변환을 시켜줍니다.
    // 모델 좌표계의 위치 -> [모델 행렬 곱하기] -> 월드 좌표계의 위치
    // -> [뷰 행렬 곱하기] -> 뷰 좌표계의 위치 -> [프로젝션 행렬 곱하기]
    // -> 스크린 좌표계의 위치
    
    // 뷰 좌표계는 NDC이기 때문에 월드 좌표를 이용해서 조명 계산
    
    float3 modifiedPos = input.posModel;
    
    float x = input.posModel.x;
    float z = input.posModel.z;
    
    float total_dy_dx = 0.0;
    float total_dy_dz = 0.0;
    
    float amplitude = 0.09;
    float waveLegnth = 2.0;
    
    float2 prevWave = float2(0, 0);
    float weight = 1.0;
    for (int i = 0; i < MAX_WAVES; i++)
    {
        float frequency = 1.0 * PI / waveLegnth;
        float phase = waves[i].speed * frequency;
        float arg = (dot(float2(x, z) + 0.5 * prevWave, normalize(float2(waves[i].xDirection, waves[i].zDirection)))) * frequency + time * phase;
        // 새로운 waveoffset 함수
        float waveoffset = amplitude * (exp(sin(arg)) - 1) * weight;
    
        // 미분값 계산
        float derivative_base = amplitude * exp(sin(arg)) * cos(arg) * frequency;
        
        float dy_dx = derivative_base * waves[i].xDirection;
        float dy_dz = derivative_base * waves[i].zDirection;
        
        prevWave = float2(dy_dx, dy_dz);
        total_dy_dx += dy_dx; // x 방향 성분
        total_dy_dz += dy_dz; // z 방향 성분 (normalize(1,3)의 z 성분)
    
        modifiedPos.y += waveoffset;
        
        amplitude *= 0.9;
        waveLegnth *= 1.23;
    }
    
    float3 tangentX = float3(1, total_dy_dx, 0);
    float3 tangentZ = float3(0, total_dy_dz, 1);
    float4 normal = float4(normalize(input.normalModel + cross(tangentZ, tangentX)), 0.0);
    // float4 normal = float4(cross(tangentZ, tangentX), 0.0);
    
    float4 pos = float4(modifiedPos, 1.0f);
    pos = mul(pos, model);
    
    PixelShaderInput output;
    
    output.posWorld = pos.xyz; // 월드 위치 따로 저장

    pos = mul(pos, view);
    pos = mul(pos, projection);

    output.posProj = pos;
    output.texcoord = input.texcoord;
    output.color = float3(0.0, 0.0, 0.0); // 다른 쉐이더에서 사용
    
    output.normalWorld = mul(normal, invTranspose).xyz;
    output.normalWorld = normalize(output.normalWorld);

    return output;
};