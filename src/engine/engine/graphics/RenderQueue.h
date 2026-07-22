#pragma once

#include "engine/graphics/Model.h"
#include "raylib.h"
#include "raymath.h"
#include <functional>
#include <limits>
#include <vector>

namespace eng {

struct DrawCommand
{
    using Pfn_Shape = std::function<void(DrawCommand& command)>;

    static constexpr Vector3 InvalidPosition = Vector3{
        .x = std::numeric_limits<float>::max(),
        .y = std::numeric_limits<float>::max(),
        .z = std::numeric_limits<float>::max(),
    };

    eng::Model* model = nullptr;
    Pfn_Shape shapeFn = nullptr;
    Vector3 position  = InvalidPosition;
    Vector3 rotation  = Vector3Zeros;
    Vector3 scale     = Vector3Ones;
    Color color       = WHITE;
    bool wired        = false;
};

class RenderQueue
{
public:
    void Submit(DrawCommand command, bool isTransparent = false);

    void Render(Camera3D camera);
    void RenderDebug(Camera3D camera);
    void Clear();

private:
    static void Render(std::vector<DrawCommand>& queue);

    std::vector<DrawCommand> m_OpaqueQueue;
    std::vector<DrawCommand> m_TransparentQueue;
};

} // namespace eng
