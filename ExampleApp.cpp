#include "ExampleApp.h"

#include <random>
#include <tuple>
#include <utility>
#include <vector>
#include <string>

#include "GeometryGenerator.h"

namespace hlab {

using namespace std;

std::random_device rd;
std::mt19937 gen(rd());

float RandomFloat(std::pair<float, float> range) {
    std::uniform_real_distribution<float> dist(range.first, range.second);
    return dist(gen);
}

ExampleApp::ExampleApp() : AppBase(), m_BasicPixelConstantBufferData() {}

bool ExampleApp::Initialize() {

    if (!AppBase::Initialize())
        return false;

    // 지구 텍스춰 출처
    // https://stackoverflow.com/questions/31799670/applying-map-of-the-earth-texture-a-sphere
    AppBase::CreateTexture("korea.jpg", m_texture, m_textureResourceView);
    AppBase::CreateTexture("wall.jpg", m_texture2, m_textureResourceView2);

    // Texture sampler 만들기
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    // Create the Sample State
    m_device->CreateSamplerState(&sampDesc, m_samplerState.GetAddressOf());

    // Geometry 정의
    MeshData meshData = GeometryGenerator::MakeGrid(5.0f, 3.0f, 80, 60);

    m_mesh = std::make_shared<Mesh>();

    AppBase::CreateVertexBuffer(meshData.vertices, m_mesh->m_vertexBuffer);
    m_mesh->m_indexCount = UINT(meshData.indices.size());
    AppBase::CreateIndexBuffer(meshData.indices, m_mesh->m_indexBuffer);

    // ConstantBuffer 만들기
    m_BasicVertexConstantBufferData.model = Matrix();
    m_BasicVertexConstantBufferData.view = Matrix();
    m_BasicVertexConstantBufferData.projection = Matrix();

    AppBase::CreateConstantBuffer(m_BasicVertexConstantBufferData,
                                  m_mesh->m_vertexConstantBuffer);

    AppBase::CreateConstantBuffer(m_BasicPixelConstantBufferData,
                                  m_mesh->m_pixelConstantBuffer);

    // 내가 추가하는 time값 버퍼 

    D3D11_BUFFER_DESC timeBufferDesc = {};
    timeBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    timeBufferDesc.ByteWidth = sizeof(TimeBufferType);
    timeBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    timeBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    timeBufferDesc.MiscFlags = 0;
    timeBufferDesc.StructureByteStride = 0;

    m_device->CreateBuffer(&timeBufferDesc, nullptr,
                           m_timeBuffer.GetAddressOf());

    // 내가 추가한 wave
    D3D11_BUFFER_DESC bd = {};
    bd.Usage = D3D11_USAGE_DYNAMIC;
    bd.ByteWidth = sizeof(WaveBuffer);
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    bd.MiscFlags = 0;

    m_device->CreateBuffer(&bd, nullptr, m_waveConstantBuffer.GetAddressOf());

    // POSITION에 float3를 보낼 경우 내부적으로 마지막에 1을 덧붙여서 float4를
    // 만듭니다.
    // https://learn.microsoft.com/en-us/windows-hardware/drivers/display/supplying-default-values-for-texture-coordinates-in-vertex-declaration
    vector<D3D11_INPUT_ELEMENT_DESC> basicInputElements = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 4 * 3 + 4 * 3,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    AppBase::CreateVertexShaderAndInputLayout(
        L"BasicVertexShader.hlsl", basicInputElements, m_basicVertexShader,
        m_basicInputLayout);

    AppBase::CreatePixelShader(L"BasicPixelShader.hlsl", m_basicPixelShader);

    // 노멀 벡터 그리기
    // 문제를 단순화하기 위해 InputLayout은 BasicVertexShader와 같이 사용합시다.
    m_normalLines = std::make_shared<Mesh>();

    std::vector<Vertex> normalVertices;
    std::vector<uint16_t> normalIndices;
    for (size_t i = 0; i < meshData.vertices.size(); i++) {

        auto v = meshData.vertices[i];

        v.texcoord.x = 0.0f; // 시작점 표시
        normalVertices.push_back(v);

        v.texcoord.x = 1.0f; // 끝점 표시
        normalVertices.push_back(v);

        normalIndices.push_back(uint16_t(2 * i));
        normalIndices.push_back(uint16_t(2 * i + 1));
    }

    AppBase::CreateVertexBuffer(normalVertices, m_normalLines->m_vertexBuffer);
    m_normalLines->m_indexCount = UINT(normalIndices.size());
    AppBase::CreateIndexBuffer(normalIndices, m_normalLines->m_indexBuffer);
    AppBase::CreateConstantBuffer(m_normalVertexConstantBufferData,
                                  m_normalLines->m_vertexConstantBuffer);

    AppBase::CreateVertexShaderAndInputLayout(
        L"NormalVertexShader.hlsl", basicInputElements, m_normalVertexShader,
        m_basicInputLayout);
    AppBase::CreatePixelShader(L"NormalPixelShader.hlsl", m_normalPixelShader);

    std::pair<float, float> amplitudeRange = {0.05f, 0.1f};
    std::pair<float, float> waveLengthRange = {0.5f, 1.0f};
    std::pair<float, float> speedRange = {0.1f, 0.9f};

    float amplitudeFBM = 0.82f;
    for (int i = 0; i < MAX_WAVES; i++) {
        waveData.waves[i] = {static_cast<float>(pow(amplitudeFBM, i + 1)),
                             1.0f,
                             RandomFloat(speedRange), 0.0f};
    };

    return true;
}

void ExampleApp::UpdateTimeBuffer(float timeInSeconds) {
    D3D11_MAPPED_SUBRESOURCE mappedResource = {};
    m_context->Map(m_timeBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                   &mappedResource);

    auto *dataPtr = reinterpret_cast<TimeBufferType *>(mappedResource.pData);
    dataPtr->time = timeInSeconds;

    m_context->Unmap(m_timeBuffer.Get(), 0);

    ID3D11Buffer *bufferArray[] = {m_timeBuffer.Get()};
    m_context->VSSetConstantBuffers(1, 1, bufferArray);
}

void ExampleApp::Update(float dt) {

    using namespace DirectX;

    static float totalTime = 0.0f;
    totalTime += dt;

    UpdateTimeBuffer(totalTime);

    // 모델의 변환
    m_BasicVertexConstantBufferData.model =
        Matrix::CreateScale(m_modelScaling) *
        Matrix::CreateRotationY(m_modelRotation.y) *
        Matrix::CreateRotationX(m_modelRotation.x) *
        Matrix::CreateRotationZ(m_modelRotation.z) *
        Matrix::CreateTranslation(m_modelTranslation);
    m_BasicVertexConstantBufferData.model =
        m_BasicVertexConstantBufferData.model.Transpose();

    m_BasicVertexConstantBufferData.invTranspose =
        m_BasicVertexConstantBufferData.model;
    m_BasicVertexConstantBufferData.invTranspose.Translation(Vector3(0.0f));
    m_BasicVertexConstantBufferData.invTranspose =
        m_BasicVertexConstantBufferData.invTranspose.Transpose().Invert();

    // 시점 변환
    m_BasicVertexConstantBufferData.view =
        Matrix::CreateRotationY(m_viewRot) *
        Matrix::CreateTranslation(0.0f, 0.0f, 2.0f);

    m_BasicPixelConstantBufferData.eyeWorld = Vector3::Transform(
        Vector3(0.0f), m_BasicVertexConstantBufferData.view.Invert());

    m_BasicVertexConstantBufferData.view =
        m_BasicVertexConstantBufferData.view.Transpose();

    // 프로젝션
    const float aspect = AppBase::GetAspectRatio(); // <- GUI에서 조절
    if (m_usePerspectiveProjection) {
        m_BasicVertexConstantBufferData.projection = XMMatrixPerspectiveFovLH(
            XMConvertToRadians(m_projFovAngleY), aspect, m_nearZ, m_farZ);
    } else {
        m_BasicVertexConstantBufferData.projection =
            XMMatrixOrthographicOffCenterLH(-aspect, aspect, -1.0f, 1.0f,
                                            m_nearZ, m_farZ);
    }
    m_BasicVertexConstantBufferData.projection =
        m_BasicVertexConstantBufferData.projection.Transpose();

    // Constant를 CPU에서 GPU로 복사
    AppBase::UpdateBuffer(m_BasicVertexConstantBufferData,
                          m_mesh->m_vertexConstantBuffer);

    m_BasicPixelConstantBufferData.material.diffuse =
        Vector3(m_materialDiffuse);
    m_BasicPixelConstantBufferData.material.specular =
        Vector3(m_materialSpecular);

    // 여러 개 조명 사용 예시
    for (int i = 0; i < MAX_LIGHTS; i++) {
        // 다른 조명 끄기
        if (i != m_lightType) {
            m_BasicPixelConstantBufferData.lights[i].strength *= 0.0f;
        } else {
            m_BasicPixelConstantBufferData.lights[i] = m_lightFromGUI;
        }
    }

    AppBase::UpdateBuffer(m_BasicPixelConstantBufferData,
                          m_mesh->m_pixelConstantBuffer);

    // 노멀 벡터 그리기
    if (m_drawNormals && m_drawNormalsDirtyFlag) {

        // m_normalVertexConstantBufferData.model =
        //     m_BasicVertexConstantBufferData.model;
        // m_normalVertexConstantBufferData.view =
        //     m_BasicVertexConstantBufferData.view;
        // m_normalVertexConstantBufferData.projection =
        //     m_BasicVertexConstantBufferData.projection;
        // m_normalVertexConstantBufferData.invTranspose =
        //     m_BasicVertexConstantBufferData.invTranspose;

        AppBase::UpdateBuffer(m_normalVertexConstantBufferData,
                              m_normalLines->m_vertexConstantBuffer);

        m_drawNormalsDirtyFlag = false;
    }
}

void ExampleApp::Render() {

    // RS: Rasterizer stage
    // OM: Output-Merger stage
    // VS: Vertex Shader
    // PS: Pixel Shader
    // IA: Input-Assembler stage

    SetViewport();

    float clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    m_context->ClearRenderTargetView(m_renderTargetView.Get(), clearColor);
    m_context->ClearDepthStencilView(m_depthStencilView.Get(),
                                     D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                                     1.0f, 0);
    m_context->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(),
                                  m_depthStencilView.Get());
    m_context->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

    m_context->VSSetShader(m_basicVertexShader.Get(), 0, 0);
    m_context->VSSetConstantBuffers(
        0, 1, m_mesh->m_vertexConstantBuffer.GetAddressOf());

    ID3D11ShaderResourceView *pixelResources[2] = {
        m_textureResourceView.Get(), m_textureResourceView2.Get()};
    m_context->PSSetShaderResources(0, 2, pixelResources);
    m_context->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

    m_context->PSSetConstantBuffers(
        0, 1, m_mesh->m_pixelConstantBuffer.GetAddressOf());
    m_context->PSSetShader(m_basicPixelShader.Get(), 0, 0);

    if (m_drawAsWire) {
        m_context->RSSetState(m_wireRasterizerSate.Get());
    } else {
        m_context->RSSetState(m_solidRasterizerSate.Get());
    }

    // 2. ConstantBuffer 업데이트
    D3D11_MAPPED_SUBRESOURCE mapped;
    m_context->Map(m_waveConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                   &mapped);
    memcpy(mapped.pData, &waveData, sizeof(WaveBuffer));
    m_context->Unmap(m_waveConstantBuffer.Get(), 0);

    // 3. 바인딩
    m_context->VSSetConstantBuffers(2, 1, m_waveConstantBuffer.GetAddressOf());

    // 버텍스/인덱스 버퍼 설정
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_context->IASetInputLayout(m_basicInputLayout.Get());
    m_context->IASetVertexBuffers(0, 1, m_mesh->m_vertexBuffer.GetAddressOf(),
                                  &stride, &offset);
    m_context->IASetIndexBuffer(m_mesh->m_indexBuffer.Get(),
                                DXGI_FORMAT_R16_UINT, 0);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_context->DrawIndexed(m_mesh->m_indexCount, 0, 0);

    // 노멀 벡터 그리기
    if (m_drawNormals) {
        m_context->VSSetShader(m_normalVertexShader.Get(), 0, 0);

        ID3D11Buffer *pptr[2] = {m_mesh->m_vertexConstantBuffer.Get(),
                                 m_normalLines->m_vertexConstantBuffer.Get()};
        m_context->VSSetConstantBuffers(0, 2, pptr);

        m_context->PSSetShader(m_normalPixelShader.Get(), 0, 0);
        // m_context->IASetInputLayout(m_basicInputLayout.Get());
        m_context->IASetVertexBuffers(
            0, 1, m_normalLines->m_vertexBuffer.GetAddressOf(), &stride,
            &offset);
        m_context->IASetIndexBuffer(m_normalLines->m_indexBuffer.Get(),
                                    DXGI_FORMAT_R16_UINT, 0);
        m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        m_context->DrawIndexed(m_normalLines->m_indexCount, 0, 0);
    }
}

void ExampleApp::UpdateGUI() {

    ImGui::Checkbox("Use Texture", &m_BasicPixelConstantBufferData.useTexture);
    ImGui::Checkbox("Wireframe", &m_drawAsWire);
    ImGui::Checkbox("Draw Normals", &m_drawNormals);
    if (ImGui::SliderFloat("Normal scale",
                           &m_normalVertexConstantBufferData.scale, 0.0f,
                           1.0f)) {
        m_drawNormalsDirtyFlag = true;
    }
    ImGui::SliderFloat3("m_modelTranslation", &m_modelTranslation.x, -2.0f,
                        2.0f);
    ImGui::SliderFloat3("m_modelRotation", &m_modelRotation.x, -3.14f, 3.14f);
    ImGui::SliderFloat3("m_modelScaling", &m_modelScaling.x, 0.1f, 2.0f);
    ImGui::SliderFloat("m_viewRot", &m_viewRot, -3.14f, 3.14f);

    ImGui::SliderFloat("Material Shininess",
                       &m_BasicPixelConstantBufferData.material.shininess, 1.0f,
                       256.0f);

    if (ImGui::RadioButton("Directional Light", m_lightType == 0)) {
        m_lightType = 0;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Point Light", m_lightType == 1)) {
        m_lightType = 1;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Spot Light", m_lightType == 2)) {
        m_lightType = 2;
    }

    ImGui::SliderFloat("Material Diffuse", &m_materialDiffuse, 0.0f, 1.0f);
    ImGui::SliderFloat("Material Specular", &m_materialSpecular, 0.0f, 1.0f);

    ImGui::SliderFloat3("Light Position", &m_lightFromGUI.position.x, -5.0f,
                        5.0f);

    ImGui::SliderFloat("Light fallOffStart", &m_lightFromGUI.fallOffStart, 0.0f,
                       5.0f);

    ImGui::SliderFloat("Light fallOffEnd", &m_lightFromGUI.fallOffEnd, 0.0f,
                       10.0f);

    ImGui::SliderFloat("Light spotPower", &m_lightFromGUI.spotPower, 1.0f,
                       512.0f);

for (int i = 0; i < MAX_WAVES; ++i) {
        ImGui::SliderFloat(("Amplitude " + std::to_string(i)).c_str(),
                           &waveData.waves[i].amplitude, 0.0f, 1.0f);
        ImGui::SliderFloat(("WaveLength " + std::to_string(i)).c_str(),
                           &waveData.waves[i].waveLength, 0.1f, 5.0f);
        ImGui::SliderFloat(("Speed " + std::to_string(i)).c_str(),
                           &waveData.waves[i].speed, 0.0f, 3.0f);
    }
}

} // namespace hlab
