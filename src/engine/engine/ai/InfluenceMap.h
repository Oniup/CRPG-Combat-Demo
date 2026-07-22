#pragma once

#include "engine/core/Resource.h"

#include <vector>

namespace eng {

enum class InfluenceDropOffType
{
    Linear,
    RapidTail
};

enum class InfluenceModificationType
{
    Add,
    Subtract,
    Multiply,
};

// Common Influence map names
static constexpr std::string_view InfluenceMapName_HighGround = "High Ground Influence";
static constexpr std::string_view InfluenceMapName_TileWeight = "Tile Weight Influence";

class InfluenceMap : public Resource
{
    ENG_RESOURCE(InfluenceMap)

public:
    InfluenceMap(unsigned width, unsigned height, unsigned length,
                 InfluenceModificationType modType = InfluenceModificationType::Add,
                 InfluenceDropOffType dropOff = InfluenceDropOffType::Linear, float threshold = 0,
                 float maxInfluence = 10.f, float minInfluence = 0.0f,
                 float decayAmountPerTurn = 0.0f, float forceInfluenceAmount = 0.0f);

    void DecayInfluence();

    void PropagateInfluence(unsigned index, float influence, float maxRadius = 0.0f,
                            bool inverse = false);
    void SetSingleCellInfluence(unsigned index, float influence);

    void ApplyInfluence(float& value, unsigned index) const;

    void Clear();

private:
    float GetMaxRadiusCells(float influence) const;

    unsigned m_Width;
    unsigned m_Height;
    unsigned m_Length;
    float m_MinInfluence;
    float m_MaxInfluence;
    float m_Threshold;
    float m_DecayAmountPerTurn;
    float m_ForceInfluenceAmount;
    InfluenceModificationType m_ModType;
    InfluenceDropOffType m_DropOffType;
    std::vector<float> m_Costs;
};

} // namespace eng
