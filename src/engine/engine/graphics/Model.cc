#include "engine/graphics/Model.h"

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"

namespace eng {

Model Model::CreatePlane(Vector2 size, int subX, int subY)
{
    Mesh planeMesh = GenMeshPlane(size.x, size.y, subX, subY);
    return Model(planeMesh);
}

Model Model::CreateCube(Vector3 size)
{
    Mesh cubeMesh = GenMeshCube(size.x, size.y, size.z);
    return Model(cubeMesh);
}

Model Model::CreateCylinder(int slices, float radius, float height)
{
    Mesh cylinderMesh = GenMeshCylinder(radius, height, slices);
    return Model(cylinderMesh);
}

Model::Model(const Mesh& mesh)
    : m_Model(LoadModelFromMesh(mesh))
{
}

Model::Model(const std::string_view& path)
    : m_Model(LoadModel(path.data()))
{
}

Model::Model(Model&& model)
    : m_Model(model.m_Model)
{
    model.m_Model = ::Model{};
}

Model::~Model()
{
    UnloadModel(m_Model);
}

Model& Model::operator=(Model&& model)
{
    UnloadModel(m_Model);
    m_Model       = model.m_Model;
    model.m_Model = ::Model{};
    return *this;
}

void Model::AssignTexture(Texture* texture, MaterialMapIndex materialTextureID)
{
    m_Model.materials[0].maps[materialTextureID].texture = texture->m_Texture;
}

void Model::Render(Vector3 position, Color tint, Vector3 scale, Vector3 rotation)
{
    m_Model.transform = MatrixScale(scale.x, scale.y, scale.z);
    m_Model.transform = MatrixMultiply(m_Model.transform, MatrixRotateXYZ(rotation));
    DrawModel(m_Model, position, 1.0f, tint);
}

void Model::RenderWired(Vector3 position, Color tint, Vector3 scale, Vector3 rotation)
{
    m_Model.transform = MatrixScale(scale.x, scale.y, scale.z);
    m_Model.transform = MatrixMultiply(m_Model.transform, MatrixRotateXYZ(rotation));
    DrawModelWires(m_Model, position, 1.0f, tint);
}

} // namespace eng
