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
    std::shared_ptr<Window> window = std::make_shared<Window>(windowHandle, width, height);

    s_hwndWindowMap.emplace(windowHandle, window);

    return window;
}

Window::EventHandlerId Window::RegisterDestroyEventHandler(Window::DestroyEventHandler &&handler)
{
    m_destroyEventHandlers.emplace_back(std::move(handler));
    return EventHandlerId(m_destroyEventHandlers.size() - 1);
}

Window::EventHandlerId Window::RegisterKeyEventHandler(Window::KeyEventHandler &&handler)
{
    m_keyEventHandlers.emplace_back(std::move(handler));
    return EventHandlerId(m_keyEventHandlers.size() - 1);
}

Window::EventHandlerId Window::RegisterMouseMotionEventHandler(Window::MouseMotionEventHandler &&handler)
{
    m_mouseMotionEventHandlers.emplace_back(std::move(handler));
    return EventHandlerId(m_mouseMotionEventHandlers.size() - 1);
}

Window::EventHandlerId Window::RegisterMouseButtonEventHandler(Window::MouseButtonEventHandler &&handler)
{
    m_mouseButtonEventHandlers.emplace_back(std::move(handler));
    return EventHandlerId(m_mouseButtonEventHandlers.size() - 1);
}

Window::EventHandlerId Window::RegisterMouseWheelEventHandler(Window::MouseWheelEventHandler &&handler)
{
    m_mouseWheelEventHandlers.emplace_back(std::move(handler));
    return EventHandlerId(m_mouseWheelEventHandlers.size() - 1);
}

Window::EventHandlerId Window::RegisterResizeEventHandler(Window::ResizeEventHandler &&handler)
{
    m_resizeEventHandlers.emplace_back(std::move(handler));
    return EventHandlerId(m_resizeEventHandlers.size() - 1);
}

uint32_t Window::GetNumBackBuffers() const
{
    return static_cast<uint32_t>(m_backBuffers.size());
}

D3D12_CPU_DESCRIPTOR_HANDLE Window::GetCurrentRenderTargetView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(m_currentBackBufferIndex), m_RTVDescriptorSize);
}

UINT Window::Present()
{
    UINT syncInterval = m_vSync ? 1 : 0;
    UINT presentFlags = Engine::Get().IsTearingSupported() && !m_vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));
    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    return m_currentBackBufferIndex;
}

void Window::Minimize()
{
    CloseWindow(m_windowHandle);
}

void Window::Destroy()
{
    DestroyWindow(m_windowHandle);
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
    if (s_hwndWindowMap.contains(hwnd))
    {
        std::shared_ptr<Window> window = s_hwndWindowMap.at(hwnd);
        return window->ProcessMessage(message, wParam, lParam);
    }
    // TODO add event handling
    return DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT Window::ProcessMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        HandlePaintEvent();
    }
    break;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        HandleKeyDownMessage(message, wParam, lParam);
    }
    break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
    {
        HandleKeyUpMessage(message, wParam, lParam);
    }
    break;
    // The default window procedure will play a system notification sound
    // when pressing the Alt+Enter keyboard combination if this message is
    // not handled.
    case WM_SYSCHAR:
        break;
    case WM_MOUSEMOVE:
    {
        HandleMouseMoveMessage(message, wParam, lParam);
    }
    break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    {
        HandleMouseButtonDownMessage(message, wParam, lParam);
    }
    break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    {
        HandleMouseButtonUpMessage(message, wParam, lParam);
    }
    break;
    case WM_MOUSEWHEEL:
    {
        HandleMouseWheelMessage(message, wParam, lParam);
    }
    break;
    case WM_SIZE:
    {
        HandleResizeMessage(message, wParam, lParam);
    }
    break;
    case WM_DESTROY:
    {
        HandleDestroyEvent();
    }
    break;
    default:
        return DefWindowProcW(m_windowHandle, message, wParam, lParam);
    }
    return 0;
}

void Window::HandlePaintEvent()
{
    PAINTSTRUCT ps;
    BeginPaint(m_windowHandle, &ps);
    EndPaint(m_windowHandle, &ps);
    ProcessPaintEvent();
}

void Window::HandleDestroyEvent()
{
    s_hwndWindowMap.erase(m_windowHandle);
    ProcessDestroyEvent();
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
        c = static_cast<unsigned int>(charMsg.wParam);
    }
    bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    KeyCode::Key key = (KeyCode::Key)wParam;
    unsigned int scanCode = (lParam & 0x00FF0000) >> 16;
    KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
    ProcessKeyEvent(keyEventArgs);
}

void Window::HandleKeyUpMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    KeyCode::Key key = (KeyCode::Key)wParam;
    unsigned int c = 0;
    unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

    // Determine which key was released by converting the key code and the scan code
    // to a printable character (if possible).
    // Inspired by the SDL 1.2 implementation.
    unsigned char keyboardState[256];
    GetKeyboardState(keyboardState);
    wchar_t translatedCharacters[4];
    if (int result = ToUnicodeEx(static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
    {
        c = translatedCharacters[0];
    }

    KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, shift, control, alt);
    ProcessKeyEvent(keyEventArgs);
}

MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID)
{
    MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
    switch (messageID)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Left;
    }
    break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Right;
    }
    break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Middle;
    }
    break;
    }

    return mouseButton;
}

void Window::HandleMouseButtonDownMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    bool lButton = (wParam & MK_LBUTTON) != 0;
    bool rButton = (wParam & MK_RBUTTON) != 0;
    bool mButton = (wParam & MK_MBUTTON) != 0;
    bool shift = (wParam & MK_SHIFT) != 0;
    bool control = (wParam & MK_CONTROL) != 0;

    int x = ((int)(short)LOWORD(lParam));
    int y = ((int)(short)HIWORD(lParam));

    MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y);
    ProcessMouseButtonEvent(mouseButtonEventArgs);
}

void Window::HandleMouseButtonUpMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    bool lButton = (wParam & MK_LBUTTON) != 0;
    bool rButton = (wParam & MK_RBUTTON) != 0;
    bool mButton = (wParam & MK_MBUTTON) != 0;
    bool shift = (wParam & MK_SHIFT) != 0;
    bool control = (wParam & MK_CONTROL) != 0;

    int x = ((int)(short)LOWORD(lParam));
    int y = ((int)(short)HIWORD(lParam));

    MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y);
    ProcessMouseButtonEvent(mouseButtonEventArgs);
}

void Window::HandleMouseWheelMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    // The distance the mouse wheel is rotated.
    // A positive value indicates the wheel was rotated to the right.
    // A negative value indicates the wheel was rotated to the left.
    float zDelta = (float)((int)(short)HIWORD(wParam)) / (float)WHEEL_DELTA;
    short keyStates = (short)LOWORD(wParam);

    bool lButton = (keyStates & MK_LBUTTON) != 0;
    bool rButton = (keyStates & MK_RBUTTON) != 0;
    bool mButton = (keyStates & MK_MBUTTON) != 0;
    bool shift = (keyStates & MK_SHIFT) != 0;
    bool control = (keyStates & MK_CONTROL) != 0;

    int x = ((int)(short)LOWORD(lParam));
    int y = ((int)(short)HIWORD(lParam));

    // Convert the screen coordinates to client coordinates.
    POINT clientToScreenPoint;
    clientToScreenPoint.x = x;
    clientToScreenPoint.y = y;
    ScreenToClient(m_windowHandle, &clientToScreenPoint);

    MouseWheelEventArgs mouseWheelEventArgs(zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
    ProcessMouseWheelEvent(mouseWheelEventArgs);
}

void Window::HandleMouseMoveMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    bool lButton = (wParam & MK_LBUTTON) != 0;
    bool rButton = (wParam & MK_RBUTTON) != 0;
    bool mButton = (wParam & MK_MBUTTON) != 0;
    bool shift = (wParam & MK_SHIFT) != 0;
    bool control = (wParam & MK_CONTROL) != 0;

    int x = ((int)(short)LOWORD(lParam));
    int y = ((int)(short)HIWORD(lParam));

    MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y);
    ProcessMouseMotionEvent(mouseMotionEventArgs);
}

void Window::HandleResizeMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    uint32_t width = ((uint32_t)(short)LOWORD(lParam));
    uint32_t height = ((uint32_t)(short)HIWORD(lParam));

    ResizeEventArgs resizeEventArgs(width, height);
    ProcessResizeEvent(resizeEventArgs);
}

void Window::ProcessPaintEvent()
{
    for (const PaintEventHandler &handler : m_PaintEventHandlers)
    {
        handler();
    }
}

void Window::ProcessDestroyEvent()
{
    for (const DestroyEventHandler &handler : m_destroyEventHandlers)
    {
        handler(m_windowHandle);
    }
}

void Window::ProcessKeyEvent(const KeyEventArgs &event)
{
    for (const KeyEventHandler &handler : m_keyEventHandlers)
    {
        handler(event);
    }
}

void Window::ProcessMouseMotionEvent(const MouseMotionEventArgs &event)
{
    for (const MouseMotionEventHandler &handler : m_mouseMotionEventHandlers)
    {
        handler(event);
    }
}

void Window::ProcessMouseButtonEvent(const MouseButtonEventArgs &event)
{
    for (const MouseButtonEventHandler &handler : m_mouseButtonEventHandlers)
    {
        handler(event);
    }
}

void Window::ProcessMouseWheelEvent(const MouseWheelEventArgs &event)
{
    for (const MouseWheelEventHandler &handler : m_mouseWheelEventHandlers)
    {
        handler(event);
    }
}

void Window::ProcessResizeEvent(const ResizeEventArgs &event)
{
    if (event.Width != m_width || event.Height != m_height)
    {
        m_width = event.Width;
        m_height = event.Height;

        ResizeSwapChainBuffers(m_width, m_height);

        for (const ResizeEventHandler &handler : m_resizeEventHandlers)
        {
            handler(event);
        }
    }
}

void Window::ResizeSwapChainBuffers(uint32_t width, uint32_t height)
{
    // Stall the CPU until the GPU is finished with any queued render
    // commands. This is required before we can resize the swap chain buffers.
    Engine::Get().WaitForGPU();

    // Before the buffers can be resized, all references to those buffers
    // need to be released.
    for (size_t i = 0; i < m_backBuffers.size(); ++i)
    {
        m_backBuffers[i].Reset();
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    assert(SUCCEEDED(m_swapChain->GetDesc(&swapChainDesc)));
    assert(SUCCEEDED(m_swapChain->ResizeBuffers((UINT)m_backBuffers.size(),
                                                width, height,
                                                swapChainDesc.BufferDesc.Format,
                                                swapChainDesc.Flags)));

    // BOOL fullscreenState;
    // m_SwapChain->GetFullscreenState(&fullscreenState, nullptr);
    // m_fullscreen = fullscreenState == TRUE;

    m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    DXHelpers::UpdateRenderTargetViews(Engine::Get().GetDevice(), m_swapChain, m_RTVDescriptorHeap, m_backBuffers);
}

bool Window::IsFullScreen() const
{
    return m_fullscreen;
}

// Set the fullscreen state of the window.
void Window::SetFullscreen(bool fullscreen)
{
    if (m_fullscreen != fullscreen)
    {
        m_fullscreen = fullscreen;

        if (m_fullscreen) // Switching to fullscreen.
        {
            // Store the current window dimensions so they can be restored
            // when switching out of fullscreen state.
            ::GetWindowRect(m_windowHandle, &m_windowedRect);

            // Set the window style to a borderless window so the client area fills
            // the entire screen.
            UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

            ::SetWindowLongW(m_windowHandle, GWL_STYLE, windowStyle);

            // Query the name of the nearest display device for the window.
            // This is required to set the fullscreen dimensions of the window
            // when using a multi-monitor setup.
            HMONITOR hMonitor = ::MonitorFromWindow(m_windowHandle, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            ::GetMonitorInfo(hMonitor, &monitorInfo);

            ::SetWindowPos(m_windowHandle, HWND_TOPMOST,
                           monitorInfo.rcMonitor.left,
                           monitorInfo.rcMonitor.top,
                           monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                           monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                           SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(m_windowHandle, SW_MAXIMIZE);
        }
        else
        {
            // Restore all the window decorators.
            ::SetWindowLong(m_windowHandle, GWL_STYLE, WS_OVERLAPPEDWINDOW);

            ::SetWindowPos(m_windowHandle, HWND_NOTOPMOST,
                           m_windowedRect.left,
                           m_windowedRect.top,
                           m_windowedRect.right - m_windowedRect.left,
                           m_windowedRect.bottom - m_windowedRect.top,
                           SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(m_windowHandle, SW_NORMAL);
        }
    }
}

void Window::ToggleFullscreen()
{
    SetFullscreen(!m_fullscreen);
}
