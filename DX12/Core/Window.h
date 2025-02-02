#pragma once

#include "MinWindows.h"
#include <wrl.h>

#include "DXHelpers.h"

#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

#include "Events.h"

class Window
{
public:
    using PaintEventHandler = std::function<void()>;
    using KeyEventHandler = std::function<void(const KeyEventArgs &event)>;
    using MouseMotionEventHandler = std::function<void(const MouseMotionEventArgs &event)>;
    using MouseButtonEventHandler = std::function<void(const MouseButtonEventArgs &event)>;
    using MouseWheelEventHandler = std::function<void(const MouseWheelEventArgs &event)>;
    using ResizeEventHandler = std::function<void(const ResizeEventArgs &event)>;

    struct EventHandlerId
    {
        EventHandlerId() = delete;

    private:
        explicit EventHandlerId(size_t index) : m_index(index) {};

        size_t m_index;

        friend class Window;
    };

    explicit Window(HWND windowHandle, uint32_t width, uint32_t height);
    Window() = delete;
    Window(Window &&) = delete;
    Window &operator=(const Window &other) = delete;

    static std::shared_ptr<Window> Create(const wchar_t *windowTitle, uint32_t width, uint32_t height);

    EventHandlerId RegisterPaintEventHandler(PaintEventHandler &&handler);
    EventHandlerId RegisterKeyEventHandler(KeyEventHandler &&handler);
    EventHandlerId RegisterMouseMotionEventHandler(MouseMotionEventHandler &&handler);
    EventHandlerId RegisterMouseButtonEventHandler(MouseButtonEventHandler &&handler);
    EventHandlerId RegisterMouseWheelEventHandler(MouseWheelEventHandler &&handler);
    EventHandlerId RegisterResizeEventHandler(ResizeEventHandler &&handler);

    HWND GetWindowHandle() const { return m_windowHandle; }
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }

    uint32_t GetCurrentBackBufferIndex() const { return m_currentBackBufferIndex; }
    Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer() const { return m_backBuffers[m_currentBackBufferIndex]; }
    uint32_t GetNumBackBuffers() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

    bool GetVSync() { return m_vSync; }
    void SetVSync(bool vsync) { m_vSync = vsync; }

    UINT Present();

    void Close();


private:

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    LRESULT ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam);

    // Window message handlers
    void HandlePaintEvent();
    void HandleDestroyEvent();
    void HandleKeyDownMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void HandleKeyUpMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void HandleMouseButtonDownMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void HandleMouseButtonUpMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void HandleMouseWheelMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void HandleMouseMoveMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void HandleResizeMessage(UINT message, WPARAM wParam, LPARAM lParam);

    // Input Event Handlers
    void ProcessPaintEvent();
    void ProcessKeyEvent(const KeyEventArgs &event);
    void ProcessMouseMotionEvent(const MouseMotionEventArgs &event);
    void ProcessMouseButtonEvent(const MouseButtonEventArgs &event);
    void ProcessMouseWheelEvent(const MouseWheelEventArgs &event);
    void ProcessResizeEvent(const ResizeEventArgs &event);

    void ResizeSwapChainBuffers(uint32_t width, uint32_t height);

    const HWND m_windowHandle;

    uint32_t m_width;
    uint32_t m_height;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;

    uint32_t m_currentBackBufferIndex;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVDescriptorHeap;
    uint32_t m_RTVDescriptorSize;

    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_backBuffers;

    std::vector<PaintEventHandler> m_PaintEventHandlers;
    std::vector<KeyEventHandler> m_keyEventHandlers;
    std::vector<MouseMotionEventHandler> m_mouseMotionEventHandlers;
    std::vector<MouseButtonEventHandler> m_mouseButtonEventHandlers;
    std::vector<MouseWheelEventHandler> m_mouseWheelEventHandlers;
    std::vector<ResizeEventHandler> m_resizeEventHandlers;

    bool m_vSync = true;

    static const wchar_t *s_windowClassName;
    static const uint32_t s_numBuffers;

    static std::unordered_map<HWND, std::shared_ptr<Window>> s_hwndWindowMap;
};