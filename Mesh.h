#pragma once

#include <d3d11.h>
#include <windows.h>
#include <wrl.h> // ComPtr

namespace hlab {

using Microsoft::WRL::ComPtr;

// ���� �޽��� ������ �׸� �� ���۵��� ����
struct Mesh {

    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
    ComPtr<ID3D11Buffer> vertexConstantBuffer;
    ComPtr<ID3D11Buffer> pixelConstantBuffer;

    ComPtr<ID3D11Texture2D> texture;
    ComPtr<ID3D11ShaderResourceView> textureResourceView;

    UINT m_indexCount = 0;
};
} // namespace hlab
