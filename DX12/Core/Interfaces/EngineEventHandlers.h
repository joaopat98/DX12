#pragma once

class IUpdateEventHandler
{
public:
    virtual ~IUpdateEventHandler() = default;

    virtual void Update(double deltaTime) = 0;
};

class IRenderEventHandler
{
public:
    virtual ~IRenderEventHandler() = default;

    virtual void Render() = 0;
};

class IPaintEventHandler
{
public:
    virtual ~IPaintEventHandler() = default;

    virtual void Paint() = 0;
};