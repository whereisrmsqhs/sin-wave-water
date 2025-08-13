#include "Common.hlsli" // 쉐이더에서도 include 사용 가능

#define MAX_WAVES 20

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
    float amplitude;
    float waveLength;
    float speed;
    float dummy; // padding 맞추기 위해 필요
};

cbuffer WaveBuffer : register(b2)
{
    Wave waves[MAX_WAVES];
};

PixelShaderInput main(VertexShaderInput input)
{
    // 모델(Model) 행렬은 모델 자신의 원점에서 
    // 월드 좌표계에서의 위치로 변환을 시켜줍니다.
    // 모델 좌표계의 위치 -> [모델 행렬 곱하기] -> 월드 좌표계의 위치
    // -> [뷰 행렬 곱하기] -> 뷰 좌표계의 위치 -> [프로젝션 행렬 곱하기]
    // -> 스크린 좌표계의 위치
    
    // 뷰 좌표계는 NDC이기 때문에 월드 좌표를 이용해서 조명 계산
    
    float3 modifiedPos = input.posModel;
    
    float waveOffset = 0.0f;
    float dx = 0.0;
    float dz = 0.0;

    // z축에 sin 기반 변형을 추가 (x 위치와 시간 기반)
    static const float2 dirs[MAX_WAVES] =
    {
        normalize(float2(1.0, 0.5)),
        normalize(float2(-0.8, 0.3)),
        normalize(float2(0.6, -0.9)),
        normalize(float2(-0.2, -1.0)),
        normalize(float2(0.3, 0.3)),
        normalize(float2(1.0, 0.5)),
        normalize(float2(-0.8, 0.3)),
        normalize(float2(0.6, -0.9)),
        normalize(float2(-0.2, -1.0)),
        normalize(float2(0.3, 0.3)),
        normalize(float2(1.0, 0.5)),
        normalize(float2(-0.8, 0.3)),
        normalize(float2(0.6, -0.9)),
        normalize(float2(-0.2, -1.0)),
        normalize(float2(0.3, 0.3)),
        normalize(float2(1.0, 0.5)),
        normalize(float2(-0.8, 0.3)),
        normalize(float2(0.6, -0.9)),
        normalize(float2(-0.2, -1.0)),
        normalize(float2(0.3, 0.3)),
    };

    float frequencyFBM = 1.11;
    
    for (int i = 0; i < MAX_WAVES; i++)
    {
        float2 posXZ = float2(modifiedPos.x, modifiedPos.z);
        float2 waveDir = dirs[i];
        float2 wavePos = waveDir * time * waves[i].speed;

        float2 relative = posXZ - wavePos;

        float frequency = 2 / waves[i].waveLength * pow(frequencyFBM, i + 1);
        float phase = dot(waveDir, relative) * frequency;

        float sinPhase = sin(phase);
        float cosPhase = cos(phase);

        waveOffset += 0.08 * waves[i].amplitude * sinPhase;
        dx += 0.1 * waves[i].amplitude * cosPhase * frequency * waveDir.x;
        dz += 0.1 * waves[i].amplitude * cosPhase * frequency * waveDir.y;
    }
    modifiedPos.y += waveOffset;

    float4 pos = float4(modifiedPos, 1.0f);
    pos = mul(pos, model);
    
    PixelShaderInput output;
    
    output.posWorld = pos.xyz; // 월드 위치 따로 저장

    pos = mul(pos, view);
    pos = mul(pos, projection);

    output.posProj = pos;
    output.texcoord = input.texcoord;
    output.color = float3(0.0, 0.0, 0.0); // 다른 쉐이더에서 사용
    
    float4 normal = float4(input.normalModel, 0.0f);
    output.normalWorld = mul(normal, invTranspose).xyz;
    output.normalWorld = normalize(output.normalWorld);

    return output;
}
;