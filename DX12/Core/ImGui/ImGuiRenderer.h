#pragma once

#include "Core/MinWindows.h"

#include <d3d12.h>

#include <memory>

#include <wrl.h>

class Window;

class ImGuiRenderer
{
public:
    ImGuiRenderer(std::shared_ptr<Window> window);

    void Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);

private:
    static bool InitImGui();

    std::shared_ptr<Window> m_window;
    bool m_showingDemoWindow = true;
};
