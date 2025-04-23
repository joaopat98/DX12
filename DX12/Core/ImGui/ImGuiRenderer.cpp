#include "ImGuiRenderer.h"

#include "DX12/Core/CommandQueue.h"
#include "DX12/Core/Window.h"

#include "DX12/Dependencies/ImGui/imgui.h"
#include "DX12/Dependencies/ImGui/imgui_impl_win32.h"
#include "DX12/Dependencies/ImGui/imgui_impl_dx12.h"

#include <cassert>

using namespace Microsoft::WRL;

class DescriptorHeapAllocator
{
public:
    void Init()
    {
        ComPtr<ID3D12Device2> device = Engine::Get().GetDevice();
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 64;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        assert(SUCCEEDED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_descriptorHeap))));

        m_heapStartCpu = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_heapStartGpu = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
        m_heapHandleIncrement = device->GetDescriptorHandleIncrementSize(desc.Type);
        m_freeIndexes.reserve(desc.NumDescriptors);
        for (int n = desc.NumDescriptors; n > 0; n--)
        {
            m_freeIndexes.push_back(n - 1);
        }
    }

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() { return m_descriptorHeap; }

    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE *out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE *out_gpu_handle)
    {
        assert(m_freeIndexes.size() > 0);
        size_t idx = m_freeIndexes.back();
        m_freeIndexes.pop_back();
        out_cpu_handle->ptr = m_heapStartCpu.ptr + (idx * m_heapHandleIncrement);
        out_gpu_handle->ptr = m_heapStartGpu.ptr + (idx * m_heapHandleIncrement);
    }

    void Free(D3D12_CPU_DESCRIPTOR_HANDLE outCpuDescHandle, D3D12_GPU_DESCRIPTOR_HANDLE outGpuDescHandle)
    {
        size_t cpuIdx = (int)((outCpuDescHandle.ptr - m_heapStartCpu.ptr) / m_heapHandleIncrement);
        size_t gpuIdx = (int)((outGpuDescHandle.ptr - m_heapStartGpu.ptr) / m_heapHandleIncrement);
        assert(cpuIdx == gpuIdx);
        m_freeIndexes.push_back(cpuIdx);
    }

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptorHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_heapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE m_heapStartGpu;
    uint32_t m_heapHandleIncrement;
    std::vector<size_t> m_freeIndexes;
};

static DescriptorHeapAllocator g_descriptorHeapAllocator;

ImGuiRenderer::ImGuiRenderer(std::shared_ptr<Window> window) : m_window(window)
{
    static bool init = InitImGui();

    ImGui_ImplWin32_Init(m_window->GetWindowHandle());
}

void ImGuiRenderer::Render(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList)
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow(&m_showingDemoWindow);

    ImGui::Begin("Hello, world!");
    ImGui::End();

    ImGui::Render();

    auto heap = g_descriptorHeapAllocator.GetDescriptorHeap().Get();
    commandList->SetDescriptorHeaps(1, &heap);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}

bool ImGuiRenderer::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    g_descriptorHeapAllocator.Init();

    ComPtr<ID3D12Device2> device = Engine::Get().GetDevice();

    ImGui_ImplDX12_InitInfo initInfo = {};
    initInfo.Device = device.Get();
    initInfo.CommandQueue = Engine::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)->GetD3D12CommandQueue().Get();
    initInfo.NumFramesInFlight = 3;
    initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    initInfo.DSVFormat = DXGI_FORMAT_UNKNOWN;

    initInfo.SrvDescriptorHeap = g_descriptorHeapAllocator.GetDescriptorHeap().Get();

    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo *, D3D12_CPU_DESCRIPTOR_HANDLE *outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE *outGpuHandle)
    {
        g_descriptorHeapAllocator.Alloc(outCpuHandle, outGpuHandle);
    };
    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo *, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
    {
        g_descriptorHeapAllocator.Free(cpuHandle, gpuHandle);
    };
    ImGui_ImplDX12_Init(&initInfo);

    return true;
}
