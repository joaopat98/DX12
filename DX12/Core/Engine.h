#pragma once

#include "MinWindows.h"

// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

#include "directx/d3d12.h"
#include <dxgi1_6.h>
#include <wrl.h>

#include <string>
#include <memory>
#include <unordered_map>

#include "Interfaces/EngineEventHandlers.h"
#include "Clock.h"

class CommandQueue;
class Window;

class Engine
{
public:
    Engine() = delete;
    Engine(Engine &&) = delete;
    Engine& operator=(Engine& other) = delete;

    static void Init(HINSTANCE applicationInstance, std::wstring cmdLine);
    static Engine &Get();

    HINSTANCE GetApplicationInstance() { return m_applicationInstance; }

    std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandQueueType);

    Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice() { return m_device; }

    bool IsTearingSupported() const { return m_isTearingSupported; }

    std::shared_ptr<Window> CreateWindow(const wchar_t *windowTitle, uint32_t width, uint32_t height);

    void RegisterStartupEventHandler(std::shared_ptr<IStartupEventHandler> startupEventHandler);
    void RegisterUpdateEventHandler(std::shared_ptr<IUpdateEventHandler> updateEventHandler);
    void RegisterRenderEventHandler(std::shared_ptr<IRenderEventHandler> renderEventHandler);

    void WaitForGPU();

    void Run();
    void Exit();

private:
    Engine(HINSTANCE applicationInstance, std::wstring cmdLine);

    bool CheckTearingSupport();

private:
    static Engine *s_singleton;

    const HINSTANCE m_applicationInstance;

    bool m_shouldRun;

    Clock m_clock;

    Microsoft::WRL::ComPtr<IDXGIAdapter4> m_adapter;
    Microsoft::WRL::ComPtr<ID3D12Device2> m_device;

    std::shared_ptr<CommandQueue> m_directCommandQueue;
    std::shared_ptr<CommandQueue> m_computeCommandQueue;
    std::shared_ptr<CommandQueue> m_copyCommandQueue;

    std::vector<std::shared_ptr<IStartupEventHandler>> m_startupEventHandlers;
    std::vector<std::shared_ptr<IUpdateEventHandler>> m_updateEventHandlers;
    std::vector<std::shared_ptr<IRenderEventHandler>> m_renderEventHandlers;

    bool m_isTearingSupported;
};