#pragma once

#include "engine/core/Resource.h"
#include "raylib.h"
#include <string_view>

namespace eng {

class Model;

class Texture : public Resource
{
    ENG_RESOURCE(Texture)

    friend Model;

public:
    Texture(const std::string_view& path);
    Texture(Texture&& other);
    ~Texture();

    int GetWidth() const { return m_Texture.width; }
    int GetHeight() const { return m_Texture.height; }
    Vector2 GetSize() const { return Vector2(m_Texture.width, m_Texture.height); }
    bool Exists() const { return m_Texture.id != 0; }

    void Enable();
    void Disable();

    virtual void Render(const Vector2& position, bool flipX = false, bool flipY = false,
                        Color tint = WHITE);

protected:
    Texture2D m_Texture;
};

class TextureAtlas : public Texture
{
    ENG_RESOURCE(TextureAtlas)

public:
    TextureAtlas(int cellWidth, int cellHeight, const std::string_view& path);

    int GetCellWidth() const { return (int)m_CurrentCell.width; }
    int GetCellHeight() const { return (int)m_CurrentCell.height; }

    float GetCellX() const { return m_CurrentCell.x; }
    float GetCellY() const { return m_CurrentCell.y; }

    Vector2 GetCellPosition() const { return Vector2(m_CurrentCell.x, m_CurrentCell.y); }
    Vector2 GetCellSize() const { return Vector2(m_CurrentCell.width, m_CurrentCell.height); }

    void SetCellPosition(int column, int row);

    void Render(const Vector2& position, bool flipX = false, bool flipY = false,
                Color tint = WHITE) override;

private:
    Rectangle m_CurrentCell;
};

} // namespace eng
