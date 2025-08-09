#pragma once

#include <d3d11.h>
#include <windows.h>
#include <wrl.h> // ComPtr

namespace hlab {

using Microsoft::WRL::ComPtr;

// 같은 메쉬를 여러번 그릴 때 버퍼들을 재사용
struct Mesh {

    ComPtr<ID3D11Buffer> m_vertexBuffer;
    ComPtr<ID3D11Buffer> m_indexBuffer;
    ComPtr<ID3D11Buffer> m_vertexConstantBuffer;
    ComPtr<ID3D11Buffer> m_pixelConstantBuffer;

    UINT m_indexCount = 0;
};
} // namespace hlab
