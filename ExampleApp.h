#pragma once

#include <algorithm>
#include <iostream>
#include <memory>

#include "AppBase.h"
#include "GeometryGenerator.h"
#include "Mesh.h"
#include "CubeMapping.h"

#define MAX_WAVES 32

namespace hlab {

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;
using DirectX::SimpleMath::Vector4;
 
// 조명
struct Light { 
    Vector3 strength = Vector3(0.5f);              // 12
    float fallOffStart = 0.0f;                     // 4
    Vector3 direction = Vector3(1.0f,-1.0f, 1.0f); // 12
    float fallOffEnd = 10.0f;                      // 4
    Vector3 position = Vector3(0.0f, -2.0f, 0.0f); // 12
    float spotPower = 1.0f;                        // 4
};

struct BasicVertexConstantBuffer {
    Matrix model;
    Matrix invTranspose;
    Matrix view;
    Matrix projection;
}; 

static_assert((sizeof(BasicVertexConstantBuffer) % 16) == 0,
              "Constant Buffer size must be 16-byte aligned");

#define MAX_LIGHTS 3

struct BasicPixelConstantBuffer {
    Vector3 eyeWorld;         // 12
    bool useTexture;          // 4
    Material material;        // 48
    Light lights[MAX_LIGHTS]; // 48 * MAX_LIGHTS
};

static_assert((sizeof(BasicPixelConstantBuffer) % 16) == 0,
              "Constant Buffer size must be 16-byte aligned");

struct NormalVertexConstantBuffer {
    // Matrix model;
    // Matrix invTranspose;
    // Matrix view;
    // Matrix projection;
    float scale = 0.1f;
    float dummy[3];
};

struct TimeBufferType {
    float time;
    float padding[3];
};

struct Wave {
    float xDirection;
    float zDirection;
    float speed;
    float dummy;
};

struct WaveBuffer {
    Wave waves[MAX_WAVES];
};

class ExampleApp : public AppBase {
  public:
    ExampleApp();

    virtual bool Initialize() override;
    virtual void UpdateGUI() override;
    virtual void Update(float dt) override;
    virtual void Render() override;

    void UpdateTimeBuffer(float timeInSeconds);
    void InitializeCubeMapping();

  protected:
    ComPtr<ID3D11VertexShader> m_basicVertexShader;
    ComPtr<ID3D11PixelShader> m_basicPixelShader;
    ComPtr<ID3D11InputLayout> m_basicInputLayout;

    // struct Mesh로 이동
    // ComPtr<ID3D11Buffer> m_vertexBuffer;
    // ComPtr<ID3D11Buffer> m_indexBuffer;
    // ComPtr<ID3D11Buffer> m_BasicVertexConstantBuffer;
    // ComPtr<ID3D11Buffer> m_pixelShaderConstantBuffer;
    // UINT m_indexCount;

    shared_ptr<Mesh> m_mesh;

    ComPtr<ID3D11Buffer> m_timeBuffer;

    // Texturing
    ComPtr<ID3D11Texture2D> m_texture;
    ComPtr<ID3D11ShaderResourceView> m_textureResourceView;
    ComPtr<ID3D11Texture2D> m_texture2;
    ComPtr<ID3D11ShaderResourceView> m_textureResourceView2;
    ComPtr<ID3D11SamplerState> m_samplerState;

    BasicVertexConstantBuffer m_BasicVertexConstantBufferData;
    BasicPixelConstantBuffer m_BasicPixelConstantBufferData;

    bool m_usePerspectiveProjection = true;
    Vector3 m_modelTranslation = Vector3(0.0f, 2.231f, 15.0f);
    Vector3 m_modelRotation = Vector3(-0.16f, 0.163f, 0.0f);
    Vector3 m_modelScaling = Vector3(0.5f, 0.8f, 1.0f);
    float m_viewRot = 0.0f;

    float m_projFovAngleY = 70.0f;
    float m_nearZ = 0.01f;
    float m_farZ = 100.0f;

    int m_lightType = 0;
    Light m_lightFromGUI;
    float m_materialDiffuse = 0.759f;
    float m_materialSpecular = 0.796f;

    // 노멀 벡터 그리기
    ComPtr<ID3D11VertexShader> m_normalVertexShader;
    ComPtr<ID3D11PixelShader> m_normalPixelShader;
    // ComPtr<ID3D11InputLayout> m_normalInputLayout; // 다른 쉐이더와 같이 사용

    shared_ptr<Mesh> m_normalLines;
    NormalVertexConstantBuffer m_normalVertexConstantBufferData;
    bool m_drawNormals = false;
    bool m_drawNormalsDirtyFlag = false;

    ComPtr<ID3D11Buffer> m_waveConstantBuffer;
    WaveBuffer waveData = {};

    CubeMapping m_cubeMapping;
};
} // namespace hlab