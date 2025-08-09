#pragma once

#include <directxtk/SimpleMath.h>
#include <vector>

namespace hlab {

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;

struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 texcoord;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};

class GeometryGenerator {
  public:
    static MeshData MakeSquare();
    static MeshData MakeBox();
    static MeshData MakeGrid(const float width, const float height,
                             const int numSlices, const int numStacks);
    static MeshData MakeCylinder(const float bottomRadius,
                                 const float topRadius, float height,
                                 int sliceCount);
    static MeshData MakeSphere(const float radius, const int numSlices,
                               const int numStacks);
    static MeshData MakeTetrahedron();
    static MeshData MakeIcosahedron();
    static MeshData SubdivideToSphere(const float radius, MeshData meshData);
};
} // namespace hlab
