#include "Game.h"
#include "Window.h"
#include "DXHelpers.h"
#include "CommandQueue.h"
#include <iostream>

Game::Game()
{
    m_window = Engine::Get().CreateWindow(L"Game", 720, 480);
    m_fenceValues.resize(m_window->GetNumBackBuffers());
    ::ShowWindow(m_window->GetWindowHandle(), SW_SHOW);
    m_window->RegisterKeyEventHandler([this](const KeyEventArgs &event)
                                      { this->OnKeyEvent(event); });
}

void Game::Update(double deltaTime)
{
    char str[20];
    sprintf_s(str, "FPS: %f", 1.0f / deltaTime);
    SetWindowText(m_window->GetWindowHandle(), str);
}

void Game::Render()
{
    auto backBuffer = m_window->GetCurrentBackBuffer();

    auto commandQueue = Engine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);

    auto commandList = commandQueue->GetCommandList();

    auto currentBackBufferIndex = m_window->GetCurrentBackBufferIndex();

    // Clear the render target.
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &barrier);

        FLOAT clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};
        auto rtv = m_window->GetCurrentRenderTargetView();

        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }

    // Present
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &barrier);

        m_fenceValues[currentBackBufferIndex] = commandQueue->ExecuteCommandList(commandList);

        currentBackBufferIndex = m_window->Present();

        commandQueue->WaitForFenceValue(m_fenceValues[currentBackBufferIndex]);
    }
}

void Game::Paint()
{
}

void Game::OnKeyEvent(const KeyEventArgs &event)
{
    if (event.State == KeyEventArgs::KeyState::Released)
    {
        switch (event.Key)
        {
        case KeyCode::Key::V:
            m_window->SetVSync(!m_window->GetVSync());
            break;
        default:
            break;
        }
    }
}
