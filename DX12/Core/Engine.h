#pragma once

#include "MinWindows.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <string>
#include <memory>

class CommandQueue;

class Engine
{
public:
    Engine() = delete;
    Engine(Engine &&) = delete;

    static void Init(HINSTANCE applicationInstance, std::wstring cmdLine);
    static Engine &Get();

    HINSTANCE GetApplicationInstance() { return m_applicationInstance; }

    std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE commandQueueType);

    Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice() { return m_device; }

private:
    // Constructor/Singleton
    Engine(HINSTANCE applicationInstance, std::wstring cmdLine);
    static Engine *s_singleton;

private:
    const HINSTANCE m_applicationInstance;

    Microsoft::WRL::ComPtr<IDXGIAdapter4> m_adapter;
    Microsoft::WRL::ComPtr<ID3D12Device2> m_device;

    std::shared_ptr<CommandQueue> m_directCommandQueue;
    std::shared_ptr<CommandQueue> m_computeCommandQueue;
    std::shared_ptr<CommandQueue> m_copyCommandQueue;
};