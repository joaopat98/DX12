#include "Clock.h"

Clock::Clock()
{
    Reset();
}

void Clock::Reset()
{
    ::QueryPerformanceFrequency(&m_tickFrequency);
    ::QueryPerformanceCounter(&m_startTicks);
    m_curTicks = m_startTicks;
}

void Clock::Update()
{
    QueryPerformanceCounter(&m_curTicks);
}

double Clock::GetCurrentTime()
{
    return (double)(m_curTicks.QuadPart - m_startTicks.QuadPart) / (double)m_tickFrequency.QuadPart;
}
