#include "engine/core/Timer.h"

#include "engine/core/Error.h"
#include "raylib.h"
#include <limits>

namespace eng {

Timer::Timer(double duration)
    : m_Start(std::numeric_limits<double>::max() - duration)

{
    SetDuration(duration);
}

void Timer::SetDuration(double duration)
{
    if (duration < 0)
        ERROR("Duration should be positive, this method will abs {}", duration);
    m_Duration = std::abs(duration);
}

void Timer::Start()
{
    m_Start = GetTime();
}

double Timer::GetEndTime() const
{
    return m_Start + m_Duration;
}

bool Timer::IsComplete()
{
    return GetTime() > GetEndTime();
}

} // namespace eng
