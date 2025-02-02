#pragma once

#include "Interfaces/EngineEventHandlers.h"
#include "Engine.h"
#include "Events.h"

class Game
    : public std::enable_shared_from_this<Game>,
      public IUpdateEventHandler,
      public IRenderEventHandler,
      public IPaintEventHandler
{
public:
    Game();

    virtual void Update(double deltaTime) override;
    virtual void Render() override;
    virtual void Paint() override;

protected:
    void OnKeyEvent(const KeyEventArgs& event);

private:
    std::shared_ptr<Window> m_window;

    std::vector<uint64_t> m_fenceValues;
};