#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxtk/SimpleMath.h>
#include <memory>

#include "Mesh.h"

namespace hlab {

using DirectX::SimpleMath::Matrix;
using DirectX::SimpleMath::Vector3;

struct Material {
    Vector3 ambient = Vector3(0.0f);
    float shiness = 0.01f;
    Vector3 diffuse = Vector3(0.0f);
    float dummy1;
    Vector3 specular = Vector3(1.0f);
    float dummy2;
};

}