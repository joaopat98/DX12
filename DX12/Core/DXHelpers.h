#pragma once

#include "MinWindows.h"

#pragma warning(push)
#pragma warning(disable : 4365)
#pragma warning(disable : 4626)
// DirectX 12 specific headers.
#include <dxgi1_6.h>
#include "directx/d3dx12.h"
#pragma warning(pop)

#include <wrl.h>

namespace DXHelpers
{
    using namespace Microsoft::WRL;

    // Debug/Support
    void EnableDebugLayer();
    bool CheckTearingSupport();

    // Device/Adapter
    ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
    ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter);

    // Commands
    ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
    ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);

    // SwapChain/RTVs
    ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount);
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
    void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap, std::vector<ComPtr<ID3D12Resource>> &backBuffers);

    // Events/Sync
    ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device);
    uint64_t SignalCommandQueue(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t &fenceValue);
    void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, uint32_t durationMs = 0xffffffff);
    void FlushCommandQueue(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t &fenceValue, HANDLE fenceEvent);
}