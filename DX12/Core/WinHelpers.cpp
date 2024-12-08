#include "WinHelpers.h"

#include <cassert>
#include <utility>



namespace WinHelpers
{
    HANDLE CreateEventHandle()
    {
        HANDLE event;

        event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(event && "Failed to create fence event.");

        return event;
    }

    void RegisterWindowClass(HINSTANCE hInst, const wchar_t *windowClassName, WNDPROC WndProc)
    {
        // Register a window class for creating our render window with.
        WNDCLASSEXW windowClass = {
            .cbSize = sizeof(WNDCLASSEX),
            .style = CS_HREDRAW | CS_VREDRAW,
            .lpfnWndProc = WndProc,
            .cbClsExtra = 0,
            .cbWndExtra = 0,
            .hInstance = hInst,
            .hIcon = ::LoadIcon(hInst, NULL),
            .hCursor = ::LoadCursor(NULL, IDC_ARROW),
            .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
            .lpszMenuName = NULL,
            .lpszClassName = windowClassName,
            .hIconSm = ::LoadIcon(hInst, NULL)};

        static ATOM atom = ::RegisterClassExW(&windowClass);
        assert(atom > 0);
    }

    HWND CreateWindow(const wchar_t *windowClassName, HINSTANCE hInst, const wchar_t *windowTitle, uint32_t width, uint32_t height)
    {
        int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

        RECT windowRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
        ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

        int windowWidth = windowRect.right - windowRect.left;
        int windowHeight = windowRect.bottom - windowRect.top;

        // Center the window within the screen. Clamp to 0, 0 for the top-left corner.
        int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
        int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

        HWND hWnd = ::CreateWindowExW(
            NULL,
            windowClassName,
            windowTitle,
            WS_OVERLAPPEDWINDOW,
            windowX,
            windowY,
            windowWidth,
            windowHeight,
            NULL,
            NULL,
            hInst,
            nullptr);

        assert(hWnd && "Failed to create window");

        return hWnd;
    }
}