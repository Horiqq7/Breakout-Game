#pragma once

#include <string>

#include "core/gpu/mesh.h"
#include "utils/glm_utils.h"

namespace object2D
{
    Mesh* CreateRectangleMesh(const std::string& name, glm::vec3 leftBottomCorner,
        float width, float height, glm::vec3 color, bool fill);
    Mesh* CreateCircleMesh(const std::string& name, glm::vec3 center, float radius, glm::vec3 color, int segments);
    Mesh* CreateStartButtonMesh(const std::string& name, glm::vec3 color);
}