#include "Engine.h"

#include <cassert>

#include "directx/d3dx12.h"
#include "DXHelpers.h"
#include "CommandQueue.h"

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

    m_adapter = DXHelpers::GetAdapter(false);
    m_device = DXHelpers::CreateDevice(m_adapter);

    m_directCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_computeCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
    m_copyCommandQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_COPY);
}
