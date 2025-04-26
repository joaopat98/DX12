#include "DXHelpers.h"

#include "WinHelpers.h"
#include <cassert>

#include "d3dx12_barriers.h"
#include "d3dx12_core.h"
#include "d3dx12_resource_helpers.h"
#include "d3dx12_root_signature.h"


namespace DXHelpers
{
    void EnableDebugLayer()
    {
#if defined(_DEBUG)
        // Always enable the debug layer before doing anything DX12 related
        // so all possible errors generated while creating DX12 objects
        // are caught by the debug layer.
        ComPtr<ID3D12Debug> debugInterface;
        assert(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))));
        debugInterface->EnableDebugLayer();
#endif
    }

    ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
    {

        ComPtr<IDXGIFactory4> dxgiFactory;
        UINT createFactoryFlags = 0;
#if defined(_DEBUG)
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        assert(SUCCEEDED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory))));

        ComPtr<IDXGIAdapter1> dxgiAdapter1;
        ComPtr<IDXGIAdapter4> dxgiAdapter4;

        if (useWarp)
        {
            assert(SUCCEEDED(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1))));
            assert(SUCCEEDED(dxgiAdapter1.As(&dxgiAdapter4)));
        }
        else
        {
            SIZE_T maxDedicatedVideoMemory = 0;
            for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
            {
                DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
                dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

                // Check to see if the adapter can create a D3D12 device without actually
                // creating it. The adapter with the largest dedicated video memory
                // is favored.
                if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                    SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
                                                D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                    dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
                {
                    maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                    assert(SUCCEEDED(dxgiAdapter1.As(&dxgiAdapter4)));
                }
            }
        }

        return dxgiAdapter4;
    }

    ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter)
    {
        ComPtr<ID3D12Device2> d3d12Device2;
        assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2))));

        // Enable debug messages in debug mode.
#ifdef _DEBUG
        ComPtr<ID3D12InfoQueue> pInfoQueue;
        if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
        {
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
            pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

            // Suppress whole categories of messages
            // D3D12_MESSAGE_CATEGORY Categories[] = {};

            // Suppress messages based on their severity level
            D3D12_MESSAGE_SEVERITY Severities[] =
                {
                    D3D12_MESSAGE_SEVERITY_INFO};

            // Suppress individual messages by their ID
            D3D12_MESSAGE_ID DenyIds[] = {
                D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE, // I'm really not sure how to avoid this message.
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                     // This warning occurs when using capture frame while graphics debugging.
            };

            D3D12_INFO_QUEUE_FILTER NewFilter = {};
            // NewFilter.DenyList.NumCategories = _countof(Categories);
            // NewFilter.DenyList.pCategoryList = Categories;
            NewFilter.DenyList.NumSeverities = _countof(Severities);
            NewFilter.DenyList.pSeverityList = Severities;
            NewFilter.DenyList.NumIDs = _countof(DenyIds);
            NewFilter.DenyList.pIDList = DenyIds;

            assert(SUCCEEDED(pInfoQueue->PushStorageFilter(&NewFilter)));
        }
#endif

        return d3d12Device2;
    }

    ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = type;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 0;

        assert(SUCCEEDED(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue))));

        return d3d12CommandQueue;
    }

    bool CheckTearingSupport()
    {
        BOOL allowTearing = FALSE;

        // Rather than create the DXGI 1.5 factory interface directly, we create the
        // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the
        // graphics debugging tools which will not support the 1.5 factory interface
        // until a future update.
        ComPtr<IDXGIFactory4> factory4;
        if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
        {
            ComPtr<IDXGIFactory5> factory5;
            if (SUCCEEDED(factory4.As(&factory5)))
            {
                if (FAILED(factory5->CheckFeatureSupport(
                        DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                        &allowTearing, sizeof(allowTearing))))
                {
                    allowTearing = FALSE;
                }
            }
        }

        return allowTearing == TRUE;
    }

    ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount)
    {
        ComPtr<IDXGISwapChain4> dxgiSwapChain4;
        ComPtr<IDXGIFactory4> dxgiFactory4;
        UINT createFactoryFlags = 0;
#ifdef _DEBUG
        createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

        assert(SUCCEEDED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4))));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width = width,
            .Height = height,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .Stereo = FALSE,
            .SampleDesc = {1, 0},
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = bufferCount,
            .Scaling = DXGI_SCALING_STRETCH,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
            .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
            // It is recommended to always allow tearing if tearing support is available.
            .Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u};

        ComPtr<IDXGISwapChain1> swapChain1;
        assert(SUCCEEDED(dxgiFactory4->CreateSwapChainForHwnd(
            commandQueue.Get(),
            hWnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain1)));

        // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
        // will be handled manually.
        assert(SUCCEEDED(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER)));

        assert(SUCCEEDED(swapChain1.As(&dxgiSwapChain4)));

        return dxgiSwapChain4;
    }

    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    {
        ComPtr<ID3D12DescriptorHeap> descriptorHeap;

        D3D12_DESCRIPTOR_HEAP_DESC desc = {
            .Type = type,
            .NumDescriptors = numDescriptors};

        assert(SUCCEEDED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap))));

        return descriptorHeap;
    }

    void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap, std::vector<ComPtr<ID3D12Resource>> &backBuffers)
    {
        UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

        for (uint8_t i = 0; i < backBuffers.size(); ++i)
        {
            ComPtr<ID3D12Resource> backBuffer;
            assert(SUCCEEDED(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer))));

            device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

            backBuffers[i] = backBuffer;

            rtvHandle.Offset((INT)rtvDescriptorSize);
        }
    }

    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12CommandAllocator> commandAllocator;
        assert(SUCCEEDED(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator))));

        return commandAllocator;
    }

    ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
    {
        ComPtr<ID3D12GraphicsCommandList> commandList;
        assert(SUCCEEDED(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList))));

        assert(SUCCEEDED(commandList->Close()));

        return commandList;
    }

    ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device)
    {
        ComPtr<ID3D12Fence> fence;

        assert(SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))));

        return fence;
    }

    uint64_t SignalCommandQueue(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t &fenceValue)
    {
        uint64_t fenceValueForSignal = ++fenceValue;
        assert(SUCCEEDED(commandQueue->Signal(fence.Get(), fenceValueForSignal)));

        return fenceValueForSignal;
    }

    void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, uint32_t durationMs)
    {
        if (fence->GetCompletedValue() < fenceValue)
        {
            assert(SUCCEEDED(fence->SetEventOnCompletion(fenceValue, fenceEvent)));
            ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(durationMs));
        }
    }

    void FlushCommandQueue(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t &fenceValue, HANDLE fenceEvent)
    {
        uint64_t fenceValueForSignal = SignalCommandQueue(commandQueue, fence, fenceValue);
        WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
    }

    void TransitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList, ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
        commandList->ResourceBarrier(1, &barrier);
    }

    void UpdateBufferResource(ComPtr<ID3D12Device2> device, ComPtr<ID3D12GraphicsCommandList2> commandList, ComPtr<ID3D12Resource> &destinationResource, ComPtr<ID3D12Resource>* intermediateResource, size_t numElements, size_t elementSize, const void *bufferData, D3D12_RESOURCE_FLAGS flags)
    {
        LONG_PTR bufferSize = numElements * elementSize;

        CD3DX12_HEAP_PROPERTIES resourceHeapProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);

        // Create a committed resource for the GPU resource in a default heap.
        assert(SUCCEEDED(device->CreateCommittedResource(
            &resourceHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceBufferDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&destinationResource))));

        // Create an committed resource for the upload.
        if (bufferData)
        {
            assert(intermediateResource);

            CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

            assert(SUCCEEDED(device->CreateCommittedResource(
                &uploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &uploadBufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&(*intermediateResource)))));

            D3D12_SUBRESOURCE_DATA subresourceData[1] = {
                {.pData = bufferData,
                 .RowPitch = bufferSize,
                 .SlicePitch = bufferSize}};

            UpdateSubresources(commandList.Get(),
                               destinationResource.Get(), intermediateResource->Get(),
                               0, 0, 1, subresourceData);
        }
    }
}