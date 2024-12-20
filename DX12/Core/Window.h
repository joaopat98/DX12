#pragma once

#include "MinWindows.h"
#include <wrl.h>

#pragma warning(push)
#pragma warning(disable : 4365)
#pragma warning(disable : 4626)
// DirectX 12 specific headers.
#include <dxgi1_6.h>
#pragma warning(pop)

#include <vector>
#include <unordered_map>

#include "Core/Events.h"

class Window
{
public:
    Window() = delete;
    Window(Window &&) = delete;
    Window &operator=(const Window &other) = delete;

    static std::shared_ptr<Window> Create(const wchar_t *windowTitle, uint32_t width, uint32_t height);

private:
    explicit Window(HWND windowHandle, uint32_t width, uint32_t height);

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    LRESULT ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);

    // Window message handlers
    void HandlePaintEvent();
    void HandleKeyDownMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void HandleKeyUpMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void HandleMouseButtonDownMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void HandleMouseButtonUpMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void HandleMouseWheelMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void HandleMouseMoveMessage(UINT message, WPARAM wParam, LPARAM lParam);

    // Input Event Handlers
    void OnKeyDown(KeyEventArgs event);

    const HWND m_windowHandle;

    uint32_t m_width;
    uint32_t m_height;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;

    uint32_t m_currentBackBufferIndex;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
    uint32_t m_RTVDescriptorSize;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_backBuffers;

    static const wchar_t *s_windowClassName;
    static const uint32_t s_numBuffers;

    static std::unordered_map<HWND, std::shared_ptr<Window>> s_hwndWindowMap;
};