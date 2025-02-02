#pragma once

#include "MinWindows.h"

#undef GetCurrentTime

class Clock{
public:
    Clock();

    void Reset();
    void Update();

    double GetCurrentTime();
private:
    LARGE_INTEGER m_curTicks;
    LARGE_INTEGER m_startTicks;
    LARGE_INTEGER m_tickFrequency;
};