#include "engine/ai/InfluenceMap.h"

#include <algorithm>

namespace eng {

InfluenceMap::InfluenceMap(unsigned width, unsigned height, unsigned length,
                           InfluenceModificationType modType, InfluenceDropOffType dropOff,
                           float threshold, float maxInfluence, float minInfluence,
                           float decayAmountPerTurn, float forceInfluenceAmount)
    : m_Width(width),
      m_Height(height),
      m_Length(length),
      m_MinInfluence(minInfluence),
      m_MaxInfluence(maxInfluence),
      m_Threshold(threshold),
      m_ForceInfluenceAmount(forceInfluenceAmount),
      m_ModType(modType),
      m_DropOffType(dropOff),
      m_DecayAmountPerTurn(decayAmountPerTurn)
{

    // Flood fill cost data
    m_Costs.resize(width * height * length);
    std::fill(m_Costs.begin(), m_Costs.end(), 0.0f);
}

void InfluenceMap::DecayInfluence()
{
    for (unsigned i = 0; i < m_Costs.size(); i++)
    {
        if (m_Costs[i] == 0.0f)
            continue;
        m_Costs[i] = std::min(0.0f, m_Costs[i] - m_DecayAmountPerTurn);
    }
}

void InfluenceMap::PropagateInfluence(unsigned index, float influence, float maxRadius,
                                      bool inverse)
{
    float maxRadiusCells = GetMaxRadiusCells(influence);
    // TODO: ...
}

void InfluenceMap::SetSingleCellInfluence(unsigned index, float influence)
{
    m_Costs[index] = influence;
}

void InfluenceMap::ApplyInfluence(float& value, unsigned index) const
{
    float cost = m_Costs.at(index);
    if (cost == 0.0f)
        return;

    if (m_ForceInfluenceAmount != 0)
        cost = m_ForceInfluenceAmount;
    switch (m_ModType)
    {
    case InfluenceModificationType::Add:      value += cost; break;
    case InfluenceModificationType::Subtract: value -= cost; break;
    case InfluenceModificationType::Multiply: value *= cost; break;
    };
}

void InfluenceMap::Clear()
{
    for (unsigned i = 0; i < m_Costs.size(); i++)
        m_Costs[i] = 0;
}

float InfluenceMap::GetMaxRadiusCells(float influence) const
{
    switch (m_DropOffType)
    {
    case InfluenceDropOffType::Linear:    return std::ceil((influence / m_Threshold) - 1);
    case InfluenceDropOffType::RapidTail: return std::ceil(std::sqrt(influence / m_Threshold) - 1);
    }
}

} // namespace eng
