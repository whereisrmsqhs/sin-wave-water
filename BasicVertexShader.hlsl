#include "Common.hlsli" // ���̴������� include ��� ����

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
    float dummy; // padding ���߱� ���� �ʿ�
};

cbuffer WaveBuffer : register(b2)
{
    Wave waves[MAX_WAVES];
};

PixelShaderInput main(VertexShaderInput input)
{
    // ��(Model) ����� �� �ڽ��� �������� 
    // ���� ��ǥ�迡���� ��ġ�� ��ȯ�� �����ݴϴ�.
    // �� ��ǥ���� ��ġ -> [�� ��� ���ϱ�] -> ���� ��ǥ���� ��ġ
    // -> [�� ��� ���ϱ�] -> �� ��ǥ���� ��ġ -> [�������� ��� ���ϱ�]
    // -> ��ũ�� ��ǥ���� ��ġ
    
    // �� ��ǥ��� NDC�̱� ������ ���� ��ǥ�� �̿��ؼ� ���� ���
    
    float3 modifiedPos = input.posModel;
    
    float waveOffset = 0.0f;
    float dx = 0.0;
    float dz = 0.0;

    // z�࿡ sin ��� ������ �߰� (x ��ġ�� �ð� ���)
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
    
    output.posWorld = pos.xyz; // ���� ��ġ ���� ����

    pos = mul(pos, view);
    pos = mul(pos, projection);

    output.posProj = pos;
    output.texcoord = input.texcoord;
    output.color = float3(0.0, 0.0, 0.0); // �ٸ� ���̴����� ���
    
    float4 normal = float4(input.normalModel, 0.0f);
    output.normalWorld = mul(normal, invTranspose).xyz;
    output.normalWorld = normalize(output.normalWorld);

    return output;
}
;