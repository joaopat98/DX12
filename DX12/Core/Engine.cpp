#include "Engine.h"

#include <cassert>

#pragma warning(push)
#pragma warning(disable : 4365)
#pragma warning(disable : 4626)
// #pragma warning(disable : 2220)
#include "directx/d3dx12.h"
#include "CommandQueue.h"
#pragma warning(pop)
#include "Window.h"
#include <iostream>

Engine *Engine::s_singleton = nullptr;

void Engine::Init(HINSTANCE applicationInstance, std::wstring cmdLine)
{
    assert(s_singleton == nullptr);
    static Engine staticInstance = Engine(applicationInstance, cmdLine);
    s_singleton = &staticInstance;
}

Engine &Engine::Get()
{
    assert(s_singleton != nullptr);
    return *s_singleton;
}

std::shared_ptr<CommandQueue> Engine::GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandQueueType)
{
    switch (commandQueueType)
    {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
        return m_directCommandQueue;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
        return m_computeCommandQueue;
    case D3D12_COMMAND_LIST_TYPE_COPY:
        return m_copyCommandQueue;
    default:
        assert(false && "Invalid command queue type.");
    }
    return nullptr;
}

std::shared_ptr<Window> Engine::CreateWindow(const wchar_t *windowTitle, uint32_t width, uint32_t height)
{
    std::shared_ptr<Window> window = Window::Create(windowTitle, width, height);

    return window;
}

void Engine::RegisterStartupEventHandler(std::shared_ptr<IStartupEventHandler> startupEventHandler)
{
    m_startupEventHandlers.push_back(startupEventHandler);
}

void Engine::RegisterUpdateEventHandler(std::shared_ptr<IUpdateEventHandler> updateEventHandler)
{
    m_updateEventHandlers.push_back(updateEventHandler);
}

void Engine::RegisterRenderEventHandler(std::shared_ptr<IRenderEventHandler> renderEventHandler)
{
    m_renderEventHandlers.push_back(renderEventHandler);
}

void Engine::WaitForGPU()

{
    m_directCommandQueue->WaitForFenceValue(m_directCommandQueue->Signal());
    m_computeCommandQueue->WaitForFenceValue(m_computeCommandQueue->Signal());
    m_copyCommandQueue->WaitForFenceValue(m_copyCommandQueue->Signal());
}

void Engine::Run()
{
    m_shouldRun = true;

    m_clock.Reset();
    double curTime = m_clock.GetCurrentTime();

    for (std::shared_ptr<IStartupEventHandler> &startupEventHandler : m_startupEventHandlers)
    {
        startupEventHandler->Startup();
    }

    while (m_shouldRun)
    {
        MSG msg = {0};
        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            std::cout << msg.message << "\n";
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        m_clock.Update();
        double newTime = m_clock.GetCurrentTime();
        double deltaTime = newTime - curTime;
        curTime = newTime;

        for (std::shared_ptr<IUpdateEventHandler> &updateEventHandler : m_updateEventHandlers)
        {
            updateEventHandler->Update(deltaTime);
        }

        for (std::shared_ptr<IRenderEventHandler> &renderEventHandler : m_renderEventHandlers)
        {
            renderEventHandler->Render();
        }
    }

    WaitForGPU();
}

void Engine::Exit()
{
    m_shouldRun = false;
}

Engine::Engine(HINSTANCE applicationInstance, std::wstring cmdLine) : m_applicationInstance(applicationInstance)
{

    // Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
    // Using this awareness context allows the client area of the window
    // to achieve 100% scaling while still allowing non-client window content to
    // be rendered in a DPI sensitive fashion.
    SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

#if defined(_DEBUG)
    // Always enable the debug layer before doing anything DX12 related
    // so all possible errors generated while creating DX12 objects
    // are caught by the debug layer.
    Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
    assert(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))));
    debugInterface->EnableDebugLayer();
#endif

    m_isTearingSupported = CheckTearingSupport();

    m_adapter = DXHelpers::GetAdapter(false);
    m_device = DXHelpers::CreateDevice(m_adapter);

    m_directCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_computeCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_copyCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_COPY);
}

bool Engine::CheckTearingSupport()
{
    BOOL allowTearing = FALSE;

    // Rather than create the DXGI 1.5 factory interface directly, we create the
    // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the
    // graphics debugging tools which will not support the 1.5 factory interface
    // until a future update.
    Microsoft::WRL::ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
    {
        Microsoft::WRL::ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5)))
        {
            if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            {
                allowTearing = FALSE;
            }
        }
    }

    return allowTearing == TRUE;
}
