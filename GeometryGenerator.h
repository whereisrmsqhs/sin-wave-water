#pragma once

#include <directxtk/SimpleMath.h>
#include <string>

#include "Vertex.h"
#include "MeshData.h"

namespace hlab {

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;

class GeometryGenerator {
  public:
    static vector<MeshData> ReadFromFile(std::string basePath,
                                         std::string filename);

    static MeshData MakeSquare();
    static MeshData MakeGrid(const float width, const float height,
                             const int numSlices, const int numStacks);
    static MeshData MakeBox(const float scale = 1.0f);
    static MeshData MakeCylinder(const float bottomRadius,
                                 const float topRadius, float height,
                                 int numSlices);
    static MeshData MakeSphere(const float radius, const int numSlices,
                               const int numStacks);
    static MeshData MakeTetrahedron();
    static MeshData MakeIcosahedron();
    static MeshData SubdivideToSphere(const float radius, MeshData meshData);
};
} // namespace hlab
