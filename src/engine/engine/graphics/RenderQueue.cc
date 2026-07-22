#include "engine/graphics/RenderQueue.h"

#include "engine/core/Error.h"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>

namespace eng {

void RenderQueue::Submit(DrawCommand command, bool isTransparent)
{
    DASSERT(command.position != DrawCommand::InvalidPosition,
            "Must provide a position when submitting to the render queue");

    if (command.model || command.shapeFn)
    {
        if (isTransparent)
            m_TransparentQueue.push_back(command);
        else
            m_OpaqueQueue.push_back(command);
        return;
    }

    FATAL("Requires a model or provide a shape draw function when submitting to the render queue");
}

void RenderQueue::Render(Camera3D camera)
{
    Vector3 forward = Vector3Normalize(camera.target - camera.position);
    auto sortFunc   = [camera, forward](const DrawCommand& cmd1, const DrawCommand& cmd2)
    {
        Vector3 dir1 = cmd1.position - camera.position;
        Vector3 dir2 = cmd2.position - camera.position;

        // Length of projected point direction from camera perspective along forward vector
        float proj1 = Vector3DotProduct(dir1, forward);
        float proj2 = Vector3DotProduct(dir2, forward);

        // Render further object before closer
        return proj1 > proj2;
    };

    std::sort(m_OpaqueQueue.begin(), m_OpaqueQueue.end(), sortFunc);
    std::sort(m_TransparentQueue.begin(), m_TransparentQueue.end(), sortFunc);

    BeginMode3D(camera);
    {
        Render(m_OpaqueQueue);
        Render(m_TransparentQueue);
    }
    EndMode3D();
}

void RenderQueue::Clear()
{
    m_OpaqueQueue.clear();
    m_TransparentQueue.clear();
}

void RenderQueue::Render(std::vector<DrawCommand>& queue)
{
    for (DrawCommand& cmd : queue)
    {
        if (cmd.model)
        {
            if (cmd.wired)
                cmd.model->RenderWired(cmd.position, cmd.color, cmd.scale, cmd.rotation);
            else
                cmd.model->Render(cmd.position, cmd.color, cmd.scale, cmd.rotation);
        }
        else
            cmd.shapeFn(cmd);
    }
}

} // namespace eng
