#include "CommandQueue.h"

#include <cassert>

CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
    : m_CommandListType(type), m_d3d12Device(device), m_fenceValue(0)
{
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    assert(SUCCEEDED(m_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_d3d12CommandQueue))));
    assert(SUCCEEDED(m_d3d12Device->CreateFence(m_fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence))));

    m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(m_FenceEvent && "Failed to create fence event handle.");
}

CommandQueue::~CommandQueue()
{
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    assert(SUCCEEDED(m_d3d12Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&commandAllocator))));

    return commandAllocator;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator)
{
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;
    assert(SUCCEEDED(m_d3d12Device->CreateCommandList(0, m_CommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList))));

    return commandList;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::GetCommandList()
{
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList;

    if (!m_CommandAllocatorQueue.empty() && IsFenceComplete(m_CommandAllocatorQueue.front().fenceValue))
    {
        commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
        m_CommandAllocatorQueue.pop();

        assert(SUCCEEDED(commandAllocator->Reset()));
    }
    else
    {
        commandAllocator = CreateCommandAllocator();
    }

    if (!m_CommandListQueue.empty())
    {
        commandList = m_CommandListQueue.front();
        m_CommandListQueue.pop();

        assert(SUCCEEDED(commandList->Reset(commandAllocator.Get(), nullptr)));
    }
    else
    {
        commandList = CreateCommandList(commandAllocator);
    }

    // Associate the command allocator with the command list so that it can be
    // retrieved when the command list is executed.
    assert(SUCCEEDED(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get())));

    return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList)
{
    commandList->Close();

    ID3D12CommandAllocator *commandAllocator;
    UINT dataSize = sizeof(commandAllocator);
    assert(SUCCEEDED(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator)));

    ID3D12CommandList *const ppCommandLists[] = {commandList.Get()};

    m_d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
    uint64_t fenceValue = Signal();

    m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{fenceValue, commandAllocator});
    m_CommandListQueue.push(commandList);

    // The ownership of the command allocator has been transferred to the ComPtr
    // in the command allocator queue. It is safe to release the reference
    // in this temporary COM pointer here.
    commandAllocator->Release();

    return fenceValue;
}

uint64_t CommandQueue::Signal()
{
    uint64_t fenceValueForSignal = ++m_fenceValue;
    assert(SUCCEEDED(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), fenceValueForSignal)));

    return fenceValueForSignal;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
    return m_d3d12Fence->GetCompletedValue() >= fenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
    if (!IsFenceComplete(fenceValue))
    {
        m_d3d12Fence->SetEventOnCompletion(fenceValue, m_FenceEvent);
        ::WaitForSingleObject(m_FenceEvent, DWORD_MAX);
    }
}

void CommandQueue::Flush()
{
    WaitForFenceValue(Signal());
}
