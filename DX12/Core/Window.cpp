#include "Window.h"

#include "WinHelpers.h"
#include "DXHelpers.h"
#include "Engine.h"
#include "CommandQueue.h"

const wchar_t *Window::s_windowClassName = L"DXWindow";
const uint32_t Window::s_numBuffers = 3;
std::unordered_map<HWND, std::shared_ptr<Window>> Window::s_hwndWindowMap;

std::shared_ptr<Window> Window::Create(const wchar_t *windowTitle, uint32_t width, uint32_t height)
{
    static bool RegisterWindowClassHelper = []()
    {
        WinHelpers::RegisterWindowClass(Engine::Get().GetApplicationInstance(), s_windowClassName, &Window::WndProc);
        return true;
    }();

    HWND windowHandle = WinHelpers::CreateWindow(s_windowClassName, Engine::Get().GetApplicationInstance(), windowTitle, width, height);
    std::shared_ptr<Window> window = std::make_shared<Window>(windowHandle);

    s_hwndWindowMap.emplace(windowHandle, window);

    return window;
}

Window::Window(HWND windowHandle, uint32_t width, uint32_t height)
    : m_windowHandle(windowHandle), m_width(width), m_height(height)
{
    m_swapChain = DXHelpers::CreateSwapChain(m_windowHandle, Engine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetD3D12CommandQueue(), m_width, m_height, s_numBuffers);
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    auto device = Engine::Get().GetDevice();
    m_RTVDescriptorHeap = DXHelpers::CreateDescriptorHeap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, s_numBuffers);
    m_RTVDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    m_backBuffers.resize(s_numBuffers);
    DXHelpers::UpdateRenderTargetViews(device, m_swapChain, m_RTVDescriptorHeap, m_backBuffers);
}

LRESULT Window::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    std::shared_ptr<Window> window = s_hwndWindowMap.at(hwnd);

    if (window)
    {
        return window->ProcessMessage(message, wParam, lParam);
    }
    // TODO add event handling
    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT Window::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT
    }
    return 0;
}

void Window::HandlePaintEvent()
{
}

void Window::HandleKeyDownMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    MSG charMsg;
    // Get the Unicode character (UTF-16)
    unsigned int c = 0;
    // For printable characters, the next message will be WM_CHAR.
    // This message contains the character code we need to send the KeyPressed event.
    // Inspired by the SDL 1.2 implementation.
    if (PeekMessage(&charMsg, m_windowHandle, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
    {
        GetMessage(&charMsg, m_windowHandle, 0, 0);
        c = static_cast<unsigned int>( charMsg.wParam );
    }
    bool shift = ( GetAsyncKeyState(VK_SHIFT) & 0x8000 ) != 0;
    bool control = ( GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool alt = ( GetAsyncKeyState(VK_MENU) & 0x8000 ) != 0;
    Key::Key key = (KeyCode::Key)wParam;
    unsigned int scanCode = (lParam & 0x00FF0000) >> 16;
    KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
    pWindow->OnKeyPressed(keyEventArgs);
}

void Window::HandleKeyUpMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
}

void Window::HandleMouseButtonDownMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
}

void Window::HandleMouseButtonUpMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
}

void Window::HandleMouseWheelMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
}

void Window::HandleMouseMoveMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
}
