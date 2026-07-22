#include "engine/graphics/Texture.h"

#include "engine/core/Error.h"
#include "raylib.h"
#include "rlgl.h"

namespace eng {

Texture::Texture(const std::string_view& path)
    : m_Texture(LoadTexture(path.data()))
{
    ASSERT(m_Texture.id != 0, "Failed to load sprite from path '{}'", path);
}

Texture::Texture(Texture&& other)
    : m_Texture(other.m_Texture)
{
    other.m_Texture.id = 0;
}

Texture::~Texture()
{
    if (m_Texture.id != 0)
    {
        UnloadTexture(m_Texture);
        m_Texture.id = 0;
    }
}

void Texture::Enable()
{
    rlSetTexture(m_Texture.id);
}

void Texture::Disable()
{
    rlSetTexture(m_Texture.id);
}

void Texture::Render(const Vector2& position, bool flipX, bool flipY, Color tint)
{
    auto source = Rectangle{
        .x      = 0,
        .y      = 0,
        .width  = (float)(flipX ? -m_Texture.width : m_Texture.width),
        .height = (float)(flipX ? -m_Texture.height : m_Texture.height),
    };
    DrawTextureRec(m_Texture, source, position, tint);
}

TextureAtlas::TextureAtlas(int cellWidth, int cellHeight, const std::string_view& path)
    : Texture(path),
      m_CurrentCell(Rectangle{
          .x      = 0,
          .y      = 0,
          .width  = (float)cellWidth,
          .height = (float)cellHeight,
      })

{
}

void TextureAtlas::SetCellPosition(int column, int row)
{
    m_CurrentCell.x = column * m_CurrentCell.width;
    m_CurrentCell.y = row * m_CurrentCell.height;
}

void TextureAtlas::Render(const Vector2& position, bool flipX, bool flipY, Color tint)
{
    auto source = Rectangle{
        .x      = m_CurrentCell.x,
        .y      = m_CurrentCell.y,
        .width  = flipX ? -m_CurrentCell.width : m_CurrentCell.width,
        .height = flipX ? -m_CurrentCell.height : m_CurrentCell.height,
    };
    DrawTextureRec(m_Texture, source, position, tint);
}

} // namespace eng
