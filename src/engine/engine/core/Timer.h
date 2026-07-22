#pragma once

namespace eng {

class Timer
{
public:
    Timer(double duration);

    void SetDuration(double duration);

    void Start();

    double GetEndTime() const;
    bool IsComplete();

private:
    double m_Start;
    double m_Duration;
};

} // namespace eng
