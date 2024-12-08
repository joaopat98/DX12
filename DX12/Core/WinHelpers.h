#pragma once

#include "MinWindows.h" // For HRESULT
#include <exception>
// In order to define a function called CreateWindow, the Windows macro needs to
// be undefined.
#if defined(CreateWindow)
#undef CreateWindow
#endif

// From DXSampleHelper.h
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}

namespace WinHelpers
{
    HANDLE CreateEventHandle();
    void RegisterWindowClass(HINSTANCE hInst, const wchar_t *windowClassName, WNDPROC WndProc);
    HWND CreateWindow(const wchar_t *windowClassName, HINSTANCE hInst, const wchar_t *windowTitle, uint32_t width, uint32_t height);
}