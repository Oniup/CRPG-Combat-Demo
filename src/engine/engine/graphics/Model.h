#pragma once

#include "engine/core/Resource.h"
#include "engine/graphics/Texture.h"
#include "raylib.h"
#include "raymath.h"

namespace eng {

class Model : public Resource
{
    ENG_RESOURCE(Model)

public:
    static Model CreatePlane(Vector2 size = Vector2Ones, int subX = 1, int subY = 1);
    static Model CreateCube(Vector3 size = Vector3Ones);
    static Model CreateCylinder(int slices = 8, float radius = 0.5f, float height = 1.0f);

    Model(const Mesh& mesh);
    Model(const std::string_view& path);
    Model(Model&& model);
    ~Model();

    Model& operator=(Model&& model);

    bool IsValid() const { return m_Model.meshCount > 0 && m_Model.meshMaterial; }
    void AssignTexture(Texture* texture, MaterialMapIndex materialTextureID);

    void Render(Vector3 position, Color tint = WHITE, Vector3 scale = Vector3Ones,
                Vector3 rotation = Vector3Zeros);
    void RenderWired(Vector3 position, Color tint = WHITE, Vector3 scale = Vector3Ones,
                     Vector3 rotation = Vector3Zeros);

private:
    ::Model m_Model;
};

} // namespace eng
