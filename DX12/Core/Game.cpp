#include "Game.h"
#include "Window.h"
#include "DXHelpers.h"
#include "CommandQueue.h"
#include <iostream>

#include "d3dx12_core.h"
#include "d3dx12_pipeline_state_stream.h"
#include "d3dx12_root_signature.h"

using namespace Microsoft::WRL;

#include <d3dcompiler.h>

#include <algorithm>

using namespace DirectX;

// Vertex data for a colored cube.
struct VertexPosColor
{
    XMFLOAT3 Position;
    XMFLOAT3 Color;
};

static VertexPosColor g_vertices[8] = {
    {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f)}, // 0
    {XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f)},  // 1
    {XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f)},   // 2
    {XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f)},  // 3
    {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f)},  // 4
    {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f)},   // 5
    {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f)},    // 6
    {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f)}    // 7
};

static WORD g_indexes[36] =
    {
        0, 1, 2, 0, 2, 3,
        4, 6, 5, 4, 7, 6,
        4, 5, 1, 4, 1, 0,
        3, 2, 6, 3, 6, 7,
        1, 5, 6, 1, 6, 2,
        4, 0, 3, 4, 3, 7};

constexpr size_t g_numRows = 300;
constexpr size_t g_numColumns = 300;
constexpr float g_xStride = 0.05f;
constexpr float g_yStride = 0.05f;
constexpr float g_cubeSize = 0.01f;
constexpr size_t g_numInstances = g_numRows * g_numColumns;

Game::Game()
    : m_scissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX)), m_FoV(45.0f)
{
    m_window = Engine::Get().CreateWindow(L"Game", 720, 480);
    m_windowWidth = m_window->GetWidth();
    m_windowHeight = m_window->GetHeight();
    m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_windowWidth), static_cast<float>(m_windowHeight));

    m_fenceValues.resize(m_window->GetNumBackBuffers());
    ::ShowWindow(m_window->GetWindowHandle(), SW_SHOW);
    m_window->RegisterKeyEventHandler([this](const KeyEventArgs &event)
                                      { this->OnKeyEvent(event); });
    m_window->RegisterResizeEventHandler([this](const ResizeEventArgs &event)
                                         { this->OnResizeEvent(event); });
    m_window->RegisterDestroyEventHandler([this](const HWND)
                                          { this->OnWindowDestroyed(); });

    m_imGuiRenderer.emplace(m_window);
}

void Game::Startup()
{
    auto device = Engine::Get().GetDevice();
    auto commandQueue = Engine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    auto commandList = commandQueue->GetCommandList();

    // Upload vertex buffer data.
    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    DXHelpers::UpdateBufferResource(device, commandList.Get(),
                                    m_vertexBuffer, &intermediateVertexBuffer,
                                    _countof(g_vertices), sizeof(VertexPosColor), g_vertices);

    // Create the vertex buffer view.
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.SizeInBytes = sizeof(g_vertices);
    m_vertexBufferView.StrideInBytes = sizeof(VertexPosColor);

    // Upload index buffer data.
    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    DXHelpers::UpdateBufferResource(device, commandList.Get(),
                                    m_indexBuffer, &intermediateIndexBuffer,
                                    _countof(g_indexes), sizeof(WORD), g_indexes);

    // Create index buffer view.
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    m_indexBufferView.SizeInBytes = sizeof(g_indexes);

    CreateInstanceBuffer();

    // Create the descriptor heap for the depth-stencil view.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    assert(SUCCEEDED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap))));

    TCHAR Dir[512];
    GetCurrentDirectory(512, Dir);

    OutputDebugString(Dir);

    // Load the vertex shader.
    ComPtr<ID3DBlob> vertexShaderBlob;
    assert(SUCCEEDED(D3DReadFileToBlob(L"Shaders\\Cube_vs.cso", &vertexShaderBlob)));

    // Load the pixel shader.
    ComPtr<ID3DBlob> pixelShaderBlob;
    assert(SUCCEEDED(D3DReadFileToBlob(L"Shaders\\Cube_ps.cso", &pixelShaderBlob)));

    // Create the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
        {"MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
        {"MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1},
        {"MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1}};

    // Create a root signature.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    // A single 32-bit constant root parameter that is used by the vertex shader.
    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

    // Serialize the root signature.
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    assert(SUCCEEDED(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription, featureData.HighestVersion, &rootSignatureBlob, &errorBlob)));
    // Create the root signature.
    assert(SUCCEEDED(device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature))));

    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS vertexShader;
        CD3DX12_PIPELINE_STATE_STREAM_PS pixelShader;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    pipelineStateStream.pRootSignature = m_rootSignature.Get();
    pipelineStateStream.inputLayout = {inputLayout, _countof(inputLayout)};
    pipelineStateStream.primitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.vertexShader = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.pixelShader = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
        sizeof(PipelineStateStream), &pipelineStateStream};
    assert(SUCCEEDED(device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pipelineState))));

    auto fenceValue = commandQueue->ExecuteCommandList(commandList);
    commandQueue->WaitForFenceValue(fenceValue);

    // Resize/Create the depth buffer.
    ResizeDepthBuffer(m_windowWidth, m_windowHeight);
}

void Game::Update(double deltaTime)
{
    wchar_t str[20];
    swprintf_s(str, L"FPS: %f\n", 1.0f / deltaTime);
    SetWindowText(m_window->GetWindowHandle(), str);
    OutputDebugString(str);

    m_currentTime += deltaTime;

    // Update the view matrix.
    const DirectX::XMVECTOR eyePosition = DirectX::XMVectorSet(0, 0, -10, 1);
    const DirectX::XMVECTOR focusPoint = DirectX::XMVectorSet(0, 0, 0, 1);
    const DirectX::XMVECTOR upDirection = DirectX::XMVectorSet(0, 1, 0, 0);
    m_viewMatrix = DirectX::XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    // Update the projection matrix.
    float aspectRatio = static_cast<float>(m_windowWidth) / static_cast<float>(m_windowHeight);
    m_projectionMatrix = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_FoV), aspectRatio, 0.1f, 100.0f);

    UpdateInstanceData();
}

void Game::Render()
{
    auto commandQueue = Engine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();

    UINT currentBackBufferIndex = m_window->GetCurrentBackBufferIndex();
    auto backBuffer = m_window->GetCurrentBackBuffer();
    auto rtv = m_window->GetCurrentRenderTargetView();
    auto dsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

    // Clear the render targets.
    {
        DXHelpers::TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        FLOAT clearColor[] = {0.4f, 0.6f, 0.9f, 1.0f};

        commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    UpdateInstanceBuffer(commandList);

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetVertexBuffers(1, 1, &m_instanceBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    commandList->RSSetViewports(1, &m_viewport);
    commandList->RSSetScissorRects(1, &m_scissorRect);

    commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

    // Update the VP matrix
    DirectX::XMMATRIX vpMatrix = DirectX::XMMatrixMultiply(m_viewMatrix, m_projectionMatrix);
    commandList->SetGraphicsRoot32BitConstants(0, sizeof(DirectX::XMMATRIX) / 4, &vpMatrix, 0);

    commandList->DrawIndexedInstanced(_countof(g_indexes), g_numInstances, 0, 0, 0);

    m_imGuiRenderer->Render(commandList);

    // Present
    {
        DXHelpers::TransitionResource(commandList, backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

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
        case KeyCode::Key::Enter:
            if (!event.Alt)
            {
                break;
            }
        case KeyCode::Key::F11:
        case KeyCode::Key::F:
            m_window->ToggleFullscreen();
            break;
        case KeyCode::Key::Escape:
            m_window->Destroy();
            break;
        default:
            break;
        }
    }
}

void Game::OnResizeEvent(const ResizeEventArgs &event)
{
    if (event.Width != m_windowWidth || event.Height != m_windowHeight)
    {
        m_windowWidth = event.Width;
        m_windowHeight = event.Height;

        m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(event.Width), static_cast<float>(event.Height));

        ResizeDepthBuffer(event.Width, event.Height);
    }
}

void Game::OnWindowDestroyed()
{
    Engine::Get().Exit();
}

void Game::ResizeDepthBuffer(uint32_t width, uint32_t height)
{
    // Flush any GPU commands that might be referencing the depth buffer.
    Engine::Get().WaitForGPU();

    width = std::max(1u, width);
    height = std::max(1u, height);

    auto device = Engine::Get().GetDevice();

    // Resize screen dependent resources.
    // Create a depth buffer.
    D3D12_CLEAR_VALUE optimizedClearValue = {};
    optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    optimizedClearValue.DepthStencil = {1.0f, 0};

    CD3DX12_HEAP_PROPERTIES depthBufferHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC depthBufferTexDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    assert(SUCCEEDED(device->CreateCommittedResource(
        &depthBufferHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthBufferTexDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &optimizedClearValue,
        IID_PPV_ARGS(&m_depthBuffer))));

    // Update the depth-stencil view.
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Texture2D.MipSlice = 0;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    device->CreateDepthStencilView(m_depthBuffer.Get(), &dsv, m_DSVHeap->GetCPUDescriptorHandleForHeapStart());
}

void Game::CreateInstanceBuffer()
{
    auto device = Engine::Get().GetDevice();
    LONG_PTR bufferSize = g_numInstances * sizeof(InstanceData);

    CD3DX12_HEAP_PROPERTIES instanceHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC instanceBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    assert(SUCCEEDED(device->CreateCommittedResource(
        &instanceHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &instanceBufferDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&m_instanceBuffer))));

    CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

    assert(SUCCEEDED(device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &uploadBufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_instanceUploadBuffer))));

    // Create instance buffer view.
    m_instanceBufferView.BufferLocation = m_instanceBuffer->GetGPUVirtualAddress();
    m_instanceBufferView.StrideInBytes = sizeof(InstanceData);
    m_instanceBufferView.SizeInBytes = g_numInstances * sizeof(InstanceData);
}

void Game::UpdateInstanceData()
{
    if (m_instanceData.get() == nullptr)
    {
        m_instanceData = std::make_unique<InstanceData[]>(g_numInstances);
    }
    auto testData = std::make_unique<unsigned char[]>(16000);
    for (int32_t x = 0; x < (int32_t)g_numRows; x++)
    {
        for (int32_t y = 0; y < (int32_t)g_numColumns; y++)
        {
            float angle = static_cast<float>((m_currentTime + (double)x) * 90.0);
            const DirectX::XMVECTOR rotationAxis = DirectX::XMVectorSet(0, 1, 1, 0);
            m_instanceData[x * g_numColumns + y].model = DirectX::XMMatrixScaling(g_cubeSize, g_cubeSize, g_cubeSize) * DirectX::XMMatrixRotationAxis(rotationAxis, DirectX::XMConvertToRadians(angle)) * DirectX::XMMatrixTranslation((float)(x - (int32_t)g_numRows / 2) * g_xStride, (float)(y - (int32_t)g_numColumns / 2) * g_yStride, 0);
        }
    }
}

void Game::UpdateInstanceBuffer(ComPtr<ID3D12GraphicsCommandList2> commandList)
{
    void *mappedData;
    D3D12_RANGE readRange = {0, 0}; // We won't read from this resource on the CPU
    m_instanceUploadBuffer->Map(0, &readRange, &mappedData);
    memcpy(mappedData, m_instanceData.get(), g_numInstances * sizeof(InstanceData));
    m_instanceUploadBuffer->Unmap(0, nullptr);

    DXHelpers::TransitionResource(commandList, m_instanceBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
    commandList->CopyBufferRegion(m_instanceBuffer.Get(), 0, m_instanceUploadBuffer.Get(), 0, g_numInstances * sizeof(InstanceData));
    DXHelpers::TransitionResource(commandList, m_instanceBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
}

void Game::InitImGui()
{
}
