#define WIN32_LEAN_AND_MEAN 1
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <Windows.h>
#include <Windowsx.h>
#include <stdio.h>
#include <assert.h>
#include <d3d12.h>
#include <dxgidebug.h>
#include <shlobj.h>
#include <string>
#include <strsafe.h>
#include <new>

#include "input.h"
#include "memory.h"
#include "gpu.h"
#include "pix.h"
#include "render_graph.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define NI_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE  4096
#define NI_UTILS_WINDOWS_LOG_MAX_BUFFER_COUNT 4

namespace gpu
{
    struct RenderApp {

        RenderApp();
        virtual ~RenderApp();

        ID3D12Device5* device = nullptr;
        Window* window = nullptr;
        ID3D12Debug3* debugInterface = nullptr;
        ID3D12InfoQueue* infoQueue = nullptr;
        IDXGIFactory1* factory = nullptr;
        IDXGIAdapter1* adapter = nullptr;
        ID3D12CommandQueue* commandQueue = nullptr;
        IDXGISwapChain1* swapChain = nullptr;
        ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
        ID3D12DescriptorHeap* dsvDescriptorHeap = nullptr;
        GpuFrameData frames[GPU_FRAME_NUM];
        GpuBackbuffer backbuffers[GPU_FRAME_NUM];
        uint64_t presentFenceValue = 0;
        uint64_t presentFrame = 0;
        uint64_t currentFrame = 0;
        uint64_t frameCount = 0;
        TArray<TArray<ID3D12Object*>> objectsToDestroy;
        TArray<std::function<void()>> presentCallbacks;
    };
}

static gpu::RenderApp renderApp = {};

static void destroyObjectForFrame(uint64_t frame, bool immediate = false)
{
    uint32_t frameIndex = frame % GPU_FRAME_NUM;
    TArray<ID3D12Object*>& frameObjects = renderApp.objectsToDestroy[frameIndex];
    for (uint64_t index = 0; index < frameObjects.getNum(); ++index)
    {
        if (!immediate) gpu::getFrameData().objectsToDestroy.add(frameObjects[index]);
        else GPU_D3D_RELEASE(frameObjects[index]);
    }
    frameObjects.reset();
}

static void addObjectToDestroyForCurrentFrame(ID3D12Object* object)
{
    uint64_t frame = gpu::getFrameCount() % GPU_FRAME_NUM;
    renderApp.objectsToDestroy[frame].add(object);
}

GpuRootSignatureDescriptorRange& GpuRootSignatureDescriptorRange::addRange(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t descriptorNum, uint32_t baseShaderRegister, uint32_t registerSpace) {
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = type;
    range.NumDescriptors = descriptorNum;
    range.BaseShaderRegister = baseShaderRegister;
    range.RegisterSpace = registerSpace;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    ranges.add(range);
    return *this;
}

void GpuRootSignatureBuilder::addRootParameterCBV(uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility) {
    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParam.Descriptor.ShaderRegister = shaderRegister;
    rootParam.Descriptor.RegisterSpace = registerSpace;
    rootParam.ShaderVisibility = shaderVisibility;
    rootParameters.add(rootParam);
}

void GpuRootSignatureBuilder::addRootParameterSRV(uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility) {
    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    rootParam.Descriptor.ShaderRegister = shaderRegister;
    rootParam.Descriptor.RegisterSpace = registerSpace;
    rootParam.ShaderVisibility = shaderVisibility;
    rootParameters.add(rootParam);
}

void GpuRootSignatureBuilder::addRootParameterUAV(uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility) {
    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    rootParam.Descriptor.ShaderRegister = shaderRegister;
    rootParam.Descriptor.RegisterSpace = registerSpace;
    rootParam.ShaderVisibility = shaderVisibility;
    rootParameters.add(rootParam);
}

void GpuRootSignatureBuilder::addRootParameterDescriptorTable(const D3D12_DESCRIPTOR_RANGE* ranges, uint32_t rangeNum, D3D12_SHADER_VISIBILITY shaderVisibility) {
    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParam.DescriptorTable.pDescriptorRanges = ranges;
    rootParam.DescriptorTable.NumDescriptorRanges = rangeNum;
    rootParam.ShaderVisibility = shaderVisibility;
    rootParameters.add(rootParam);
}
void GpuRootSignatureBuilder::addRootParameterDescriptorTable(const GpuRootSignatureDescriptorRange& ranges, D3D12_SHADER_VISIBILITY shaderVisibility) {
    addRootParameterDescriptorTable(*ranges, ranges.getNum(), shaderVisibility);
}
void GpuRootSignatureBuilder::addRootParameterConstant(uint32_t shaderRegister, uint32_t registerSpace, uint32_t num32BitValues, D3D12_SHADER_VISIBILITY shaderVisibility) {
    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParam.Constants.ShaderRegister = shaderRegister;
    rootParam.Constants.RegisterSpace = registerSpace;
    rootParam.Constants.Num32BitValues = num32BitValues;
    rootParam.ShaderVisibility = shaderVisibility;
    rootParameters.add(rootParam);
}
void GpuRootSignatureBuilder::addStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addressModeAll, uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility) {
    D3D12_STATIC_SAMPLER_DESC staticSampler = {};
    staticSampler.Filter = filter;
    staticSampler.AddressU = addressModeAll;
    staticSampler.AddressV = addressModeAll;
    staticSampler.AddressW = addressModeAll;
    staticSampler.MipLODBias = 0.0f;
    staticSampler.MaxAnisotropy = 0;
    staticSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    staticSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    staticSampler.MinLOD = 0;
    staticSampler.MaxLOD = 0;
    staticSampler.ShaderRegister = shaderRegister;
    staticSampler.RegisterSpace = registerSpace;
    staticSampler.ShaderVisibility = shaderVisibility;
    staticSamplers.add(staticSampler);
}

ID3D12RootSignature* GpuRootSignatureBuilder::build(bool isCompute) {
    ID3DBlob* rootSignatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = rootParameters.getNum();
    rootSignatureDesc.pParameters = rootParameters.getData();
    rootSignatureDesc.NumStaticSamplers = staticSamplers.getNum();
    rootSignatureDesc.pStaticSamplers = staticSamplers.getData();
    rootSignatureDesc.Flags = isCompute ? D3D12_ROOT_SIGNATURE_FLAG_NONE : D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    if (D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob) != S_OK) {
        DBG_PANIC("Failed to serialize root signature.\n%s", errorBlob->GetBufferPointer());
    }
    ID3D12RootSignature* rootSignature = nullptr;
    GPU_D3D_ASSERT(renderApp.device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)), "Failed to create root signature");
    return rootSignature;
}

void GpuDescriptorAllocator::reset() {
    descriptorAllocated = 0;
}

GpuDescriptorTable GpuDescriptorAllocator::allocateDescriptorTable(uint32_t descriptorNum) {
    DBG_ASSERT(descriptorNum > 0, "DescriptorAllocator: cannot allocate 0 descriptors");
    DBG_ASSERT(descriptorAllocated <= descriptorCapacity, "DescriptorAllocator: internal overflow");
    DBG_ASSERT(descriptorCapacity - descriptorAllocated >= descriptorNum,
        "DescriptorAllocator: requested %u descriptors, only %u remaining",
        descriptorNum, remaining());

    const SIZE_T byteOffset = SIZE_T(descriptorAllocated) * SIZE_T(descriptorHandleSize);

    D3D12_GPU_DESCRIPTOR_HANDLE gpu = {};
    if (gpuBaseHandle.ptr != 0) {
        gpu.ptr = gpuBaseHandle.ptr + byteOffset;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu = {};
    cpu.ptr = cpuBaseHandle.ptr + byteOffset;

    GpuDescriptorTable table{};
    table.gpuBaseHandle = gpu;
    table.cpuBaseHandle = cpu;
    table.handleSize = descriptorHandleSize;
    table.allocated = 0;
    table.capacity = descriptorNum;

    descriptorAllocated += descriptorNum;
    return table;
}

static ID3D12RootSignature* buildRootSignature(GpuBindingLayout layout, bool compute) {
    // Fix descriptor tables
    for (uint32_t index = 0; index < layout.rootParameterNum; ++index) {
        if (layout.rootParameters[index].ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            uint64_t rangeIndex = (uint64_t)layout.rootParameters[index].DescriptorTable.pDescriptorRanges;
            layout.rootParameters[index].DescriptorTable.pDescriptorRanges = &layout.descriptorTableRanges[rangeIndex];
        }
    }

    ID3DBlob* rootSignatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = layout.rootParameterNum;
    rootSignatureDesc.pParameters = layout.rootParameters;
    rootSignatureDesc.NumStaticSamplers = layout.staticSamplerNum;
    rootSignatureDesc.pStaticSamplers = layout.staticSamplers;
    rootSignatureDesc.Flags = compute ? D3D12_ROOT_SIGNATURE_FLAG_NONE : D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    if (D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootSignatureBlob, &errorBlob) != S_OK) {
        DBG_ASSERT(0, "Failed to serialize root signature.\n%s", errorBlob->GetBufferPointer());
        return nullptr;
    }
    ID3D12RootSignature* rootSignature = nullptr;
    GPU_D3D_ASSERT(gpu::getDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)), "Failed to create root signature");
    return rootSignature;
}

GpuPipelineState* gpu::buildComputePipelineState(const GpuComputePipelineDesc& desc) {
    if (ID3D12RootSignature* rootSignature = buildRootSignature(desc.layout, true)) {
        D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature;
        psoDesc.CS = desc.shader;
        psoDesc.NodeMask = 0;
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        if (ID3D12PipelineState* pso = gpu::createComputePipelineState(psoDesc)) {
            GpuPipelineState* pipelineState = new GpuPipelineState;
            pipelineState->pso = pso;
            pipelineState->rootSignature = rootSignature;
            return pipelineState;
        }
        GPU_D3D_RELEASE(rootSignature);
    }
    return nullptr;
}

GpuPipelineState* gpu::buildGraphicsPipelineState(GpuGraphicsPipelineDesc& desc) {
    if (ID3D12RootSignature* rootSignature = buildRootSignature(desc.layout, false)) {
        TArray<D3D12_INPUT_ELEMENT_DESC, uint32_t> inputElements;
        for (uint32_t bufferIndex = 0; bufferIndex < desc.vertex.vertexBuffers.getNum(); ++bufferIndex) {
            for (uint32_t attribIndex = 0; attribIndex < desc.vertex.vertexBuffers[bufferIndex].vertexAttributes.getNum(); ++attribIndex) {
                const GpuVertexAttribute& attrib = desc.vertex.vertexBuffers[bufferIndex].vertexAttributes[attribIndex];
                D3D12_INPUT_ELEMENT_DESC elementDesc = {};
                elementDesc.SemanticName = attrib.semanticName;
                elementDesc.SemanticIndex = attrib.semanticIndex;
                elementDesc.Format = attrib.format;
                elementDesc.InputSlot = bufferIndex;
                elementDesc.AlignedByteOffset = attrib.offset;
                elementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
                elementDesc.InstanceDataStepRate = 0;
                inputElements.add(elementDesc);
            }
        }

        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.pRootSignature = rootSignature;
        psoDesc.VS = desc.vertex.shader;
        psoDesc.PS = desc.pixel.shader;
        psoDesc.BlendState.AlphaToCoverageEnable = false;
        psoDesc.BlendState.IndependentBlendEnable = false;

        for (uint32_t index = 0; index < desc.pixel.renderTargets.getNum(); ++index) {
            psoDesc.BlendState.RenderTarget[index].BlendEnable = true;
            psoDesc.BlendState.RenderTarget[index].LogicOpEnable = false;
            psoDesc.BlendState.RenderTarget[index].SrcBlend = D3D12_BLEND_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[index].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[index].BlendOp = D3D12_BLEND_OP_ADD;
            psoDesc.BlendState.RenderTarget[index].SrcBlendAlpha = D3D12_BLEND_ONE;
            psoDesc.BlendState.RenderTarget[index].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
            psoDesc.BlendState.RenderTarget[index].BlendOpAlpha = D3D12_BLEND_OP_ADD;
            psoDesc.BlendState.RenderTarget[index].LogicOp = D3D12_LOGIC_OP_NOOP;
            psoDesc.BlendState.RenderTarget[index].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        }

        psoDesc.SampleMask = UINT_MAX;
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = desc.rasterizer.cullMode;
        psoDesc.RasterizerState.FrontCounterClockwise = false;
        psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        psoDesc.RasterizerState.DepthClipEnable = true;
        psoDesc.RasterizerState.MultisampleEnable = false;
        psoDesc.RasterizerState.AntialiasedLineEnable = false;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        psoDesc.DepthStencilState.DepthEnable = desc.pixel.depthStencilFormat != DXGI_FORMAT_UNKNOWN;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        psoDesc.DepthStencilState.StencilEnable = false;
        psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
        psoDesc.DepthStencilState.FrontFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
        psoDesc.DepthStencilState.BackFace = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
        psoDesc.InputLayout.pInputElementDescs = inputElements.getData();
        psoDesc.InputLayout.NumElements = inputElements.getNum();
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = desc.pixel.renderTargets.getNum();
        for (uint32_t index = 0; index < desc.pixel.renderTargets.getNum(); ++index) {
            psoDesc.RTVFormats[index] = desc.pixel.renderTargets[index];
        }
        psoDesc.DSVFormat = desc.pixel.depthStencilFormat;
        psoDesc.SampleDesc = { 1, 0 };
        psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        if (ID3D12PipelineState* pso = gpu::createGraphicsPipelineState(psoDesc)) {
            GpuPipelineState* pipelineState = new GpuPipelineState;
            pipelineState->pso = pso;
            pipelineState->rootSignature = rootSignature;
            return pipelineState;
        }
        GPU_D3D_RELEASE(rootSignature);
    }
    return nullptr;
}

void gpu::destroyPipelineState(GpuPipelineState*& pipelineState) {
    if (pipelineState == nullptr) return;
    addObjectToDestroyForCurrentFrame(pipelineState->pso);
    addObjectToDestroyForCurrentFrame(pipelineState->rootSignature);
    delete pipelineState;
    pipelineState = nullptr;
}

void gpu::destroyBuffer(GpuBuffer*& buffer) {
    if (buffer == nullptr) return;
    if (buffer->refCounter->dec())
    {
        addObjectToDestroyForCurrentFrame(buffer->resource.apiResource);
        addObjectToDestroyForCurrentFrame(buffer->upload.apiResource);
        delete buffer->refCounter;
        delete buffer;
        buffer = nullptr;
    }
}

ID3D12CommandQueue* gpu::getCommandQueue()
{
    return renderApp.commandQueue;
}

void gpu::addOnPresentCallback(std::function<void()> callback)
{
    renderApp.presentCallbacks.add(callback);
}

void gpu::init(Window* window, bool enableValidations, bool enablePIX) 
{
    if (!window)
    {
		DBG_PANIC("ni::init: window is null");
        return;
    }

    //memset(&renderApp, 0, sizeof(renderApp));
    new (&renderApp) RenderApp();
    renderApp.frameCount = 0;
    renderApp.window = window;

    UINT factoryFlag = 0;
    renderApp.debugInterface = nullptr;
#if PIX_ENABLE_DEBUG_API
    if (enablePIX)
    {
        DBG_ASSERT(pix::loadPIX(), "Failed to load PIX");
    }
#endif
    if (enableValidations) {
        GPU_D3D_ASSERT(D3D12GetDebugInterface(IID_PPV_ARGS(&renderApp.debugInterface)), "Failed to create debug interface");
        renderApp.debugInterface->EnableDebugLayer();
        renderApp.debugInterface->SetEnableGPUBasedValidation(true);
        renderApp.debugInterface->SetEnableSynchronizedCommandQueueValidation(true);
        factoryFlag = DXGI_CREATE_FACTORY_DEBUG;
    }
    GPU_D3D_ASSERT(CreateDXGIFactory2(factoryFlag, IID_PPV_ARGS(&renderApp.factory)), "Failed to create factory");
    GPU_D3D_ASSERT(renderApp.factory->EnumAdapters(0, (IDXGIAdapter**)(&renderApp.adapter)), "Failed to aquire adapter");
    GPU_D3D_ASSERT(D3D12CreateDevice((IUnknown*)renderApp.adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&renderApp.device)), "Failed to create device");
    renderApp.device->SetName(L"gpu::device");
    renderApp.infoQueue = nullptr;
    if (enableValidations && renderApp.device->QueryInterface(IID_PPV_ARGS(&renderApp.infoQueue)) == S_OK)
    {
        renderApp.infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, false);
        renderApp.infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);

        D3D12_MESSAGE_SEVERITY severities[] = 
        {
            D3D12_MESSAGE_SEVERITY_WARNING
        };
        D3D12_MESSAGE_ID denyIds[] = 
        {
            D3D12_MESSAGE_ID_HEAP_ADDRESS_RANGE_INTERSECTS_MULTIPLE_BUFFERS
        };

        D3D12_INFO_QUEUE_FILTER filter = {};
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = &severities[0];
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = &denyIds[0];
        renderApp.infoQueue->AddStorageFilterEntries(&filter);
    }

    D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
    commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    commandQueueDesc.NodeMask = 0;
    GPU_D3D_ASSERT(renderApp.device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&renderApp.commandQueue)), "Failed to create command queue");
    renderApp.commandQueue->SetName(L"gpu::graphicsCommandQueue");

    for (uint32_t index = 0; index < GPU_FRAME_NUM; ++index) {
        GpuFrameData& frame = renderApp.frames[index];
        GPU_D3D_ASSERT(renderApp.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.commandAllocator)), "Failed to create command allocator");
        GPU_D3D_ASSERT(renderApp.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frame.commandAllocator, nullptr, IID_PPV_ARGS(&frame.commandList)), "Failed to create command list");
        GPU_D3D_ASSERT(renderApp.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frame.fence)), "Failed to create fence");
        frame.fenceEvent = CreateEvent(nullptr, false, false, nullptr);
        GPU_D3D_ASSERT(frame.commandList->Close(), "Failed to close command list");
        frame.commandAllocator->SetName(str::format(L"gpu::frame::commandAllocator %u", index));
        frame.commandList->SetName(str::format(L"gpu::frame::commandList %u", index));
        frame.fence->SetName(str::format(L"gpu::frame::fence %u", index));
        frame.frameIndex = index;
        frame.frameWaitValue = 0;
        frame.state = GpuFrameState::FINISHED;
        renderApp.objectsToDestroy.add(TArray<ID3D12Object*>());
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {
         { 
            window->sizeUint[0],
            window->sizeUint[1],
            { 0,  0},
            DXGI_FORMAT_R8G8B8A8_UNORM,
            DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            DXGI_MODE_SCALING_UNSPECIFIED},
        { 1, 0 },
        DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT,
        GPU_BACKBUFFER_NUM,
        (HWND)window->windowHandle,
        true,
        DXGI_SWAP_EFFECT_FLIP_DISCARD,
        DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH 
    };

    GPU_D3D_ASSERT(renderApp.factory->CreateSwapChain((IUnknown*)renderApp.commandQueue, &swapChainDesc, (IDXGISwapChain**)&renderApp.swapChain), "Failed to create swapchain");
    for (uint32_t index = 0; index < GPU_BACKBUFFER_NUM; ++index) {
        
        renderApp.swapChain->GetBuffer(index, IID_PPV_ARGS(&renderApp.backbuffers[index].texture.resource.apiResource));
        renderApp.backbuffers[index].texture.resource.setName(L"gpu::backbuffer %u", index);
        renderApp.backbuffers[index].texture.resource.state = D3D12_RESOURCE_STATE_PRESENT;
        D3D12_RESOURCE_DESC desc = renderApp.backbuffers[index].texture.resource.apiResource->GetDesc();
        renderApp.backbuffers[index].texture.width = (uint32_t)desc.Width;
        renderApp.backbuffers[index].texture.height = (uint32_t)desc.Height;
        renderApp.backbuffers[index].texture.depth = (uint32_t)desc.DepthOrArraySize;
        GPU_D3D_ASSERT(renderApp.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&renderApp.backbuffers[index].fence)), "Failed to create fence");
        renderApp.backbuffers[index].fence->SetName(str::format(L"gpu::backbuffer::fence %u", index));
        renderApp.backbuffers[index].event = CreateEvent(nullptr, false, false, nullptr);;
        renderApp.backbuffers[index].fenceValue = 0;
    }

    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc = {};
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvDescriptorHeapDesc.NumDescriptors = GPU_BACKBUFFER_NUM;
    rtvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvDescriptorHeapDesc.NodeMask = 0;
    GPU_D3D_ASSERT(renderApp.device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&renderApp.rtvDescriptorHeap)), "Failed to create RTV descriptor heap");
    renderApp.rtvDescriptorHeap->SetName(L"gpu::rtvDescriptorHeap");

    D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc = {};
    dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvDescriptorHeapDesc.NumDescriptors = GPU_BACKBUFFER_NUM;
    dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvDescriptorHeapDesc.NodeMask = 0;
    GPU_D3D_ASSERT(renderApp.device->CreateDescriptorHeap(&dsvDescriptorHeapDesc, IID_PPV_ARGS(&renderApp.dsvDescriptorHeap)), "Failed to create DSV descriptor heap");
    renderApp.dsvDescriptorHeap->SetName(L"gpu::dsvDescriptorHeap");

    /*
            ID3D12Device5* device = nullptr;
        ID3D12Debug3* debugInterface = nullptr;
        ID3D12InfoQueue* infoQueue = nullptr;
        IDXGIFactory1* factory = nullptr;
        IDXGIAdapter1* adapter = nullptr;
        ID3D12CommandQueue* commandQueue = nullptr;
        IDXGISwapChain1* swapChain = nullptr;
        ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
        ID3D12DescriptorHeap* dsvDescriptorHeap = nullptr;
        GPUFrameData frames[GPU_FRAME_NUM];
        GPUBackbuffer backbuffers[GPU_FRAME_NUM];
        uint64_t presentFenceValue = 0;
        uint64_t presentFrame = 0;
        uint64_t currentFrame = 0;
        uint64_t frameCount = 0;
    */
}
void gpu::setFrameUserData(uint32_t frame, void* data) {
    DBG_ASSERT(frame < GPU_FRAME_NUM, "Can't store user data on frame %u because it doesn't exist. The frame count is %u", frame, GPU_FRAME_NUM);
    renderApp.frames[frame].userData = data;
}
void gpu::waitForCurrentFrame() {
    GpuFrameData& frame = renderApp.frames[renderApp.currentFrame];
    if (frame.fence->GetCompletedValue() <  frame.frameWaitValue) {
        frame.fence->SetEventOnCompletion(frame.frameWaitValue, frame.fenceEvent);
        WaitForSingleObject(frame.fenceEvent, INFINITE);
    }
    if (frame.state == GpuFrameState::EXECUTING) {
        frame.state = GpuFrameState::FINISHED;
        for (uint64_t index = 0; index < frame.objectsToDestroy.getNum(); ++index)
        {
            GPU_D3D_RELEASE(frame.objectsToDestroy[index]);
        }
        frame.objectsToDestroy.reset();
    }
}
void gpu::waitForAllFrames() {
    for (uint32_t index = 0; index < GPU_FRAME_NUM; ++index) {
        GpuFrameData& frame = renderApp.frames[index];
        if (frame.fence->GetCompletedValue() < frame.frameWaitValue) {
            frame.fence->SetEventOnCompletion(frame.frameWaitValue, frame.fenceEvent);
            WaitForSingleObject(frame.fenceEvent, INFINITE);
        }
        if (frame.state == GpuFrameState::EXECUTING) {
            frame.state = GpuFrameState::FINISHED;
        }
    }
}
void gpu::destroy() {
    waitForAllFrames();

    if (renderApp.backbuffers[renderApp.presentFrame].fence->GetCompletedValue() < renderApp.backbuffers[renderApp.presentFrame].fenceValue) {
        renderApp.backbuffers[renderApp.presentFrame].fence->SetEventOnCompletion(renderApp.backbuffers[renderApp.presentFrame].fenceValue, renderApp.backbuffers[renderApp.presentFrame].event);
        WaitForSingleObject(renderApp.backbuffers[renderApp.presentFrame].event, INFINITE);
    }


    for (uint32_t index = 0; index < GPU_FRAME_NUM; ++index) {
        GpuFrameData& frame = renderApp.frames[index];
        GPU_D3D_RELEASE(frame.commandList);
        GPU_D3D_RELEASE(frame.commandAllocator);
        GPU_D3D_RELEASE(frame.fence);
        CloseHandle(frame.fenceEvent);
        frame.texturesToUpload.clear();
        frame.buffersToUpload.clear();
        destroyObjectForFrame(index, true);
    }

    for (uint32_t index = 0; index < GPU_BACKBUFFER_NUM; ++index) {
        GPU_D3D_RELEASE(renderApp.backbuffers[index].texture.resource.apiResource);
        GPU_D3D_RELEASE(renderApp.backbuffers[index].fence);
        CloseHandle(renderApp.backbuffers[index].event);
    }
    GPU_D3D_RELEASE(renderApp.dsvDescriptorHeap);
    GPU_D3D_RELEASE(renderApp.rtvDescriptorHeap);
    GPU_D3D_RELEASE(renderApp.swapChain);
    GPU_D3D_RELEASE(renderApp.commandQueue);
    GPU_D3D_RELEASE(renderApp.device);
    GPU_D3D_RELEASE(renderApp.adapter);
    GPU_D3D_RELEASE(renderApp.factory);
    GPU_D3D_RELEASE(renderApp.infoQueue);

#if _DEBUG
    if (renderApp.debugInterface) {
        IDXGIDebug1* dxgiDebug = nullptr;
        if (DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)) == S_OK) {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
            dxgiDebug->Release();
        }
        GPU_D3D_RELEASE(renderApp.debugInterface);
    }
#endif
}

GpuFrameData& gpu::getFrameData() {
    GpuFrameData& frame = renderApp.frames[renderApp.currentFrame];
    return frame;
}

ID3D12Device* gpu::getDevice() {
    return renderApp.device;
}

GpuFrameData* gpu::beginFrame() {
    GpuFrameData& frame = renderApp.frames[renderApp.currentFrame];
    if (frame.state != GpuFrameState::FINISHED) {
        gpu::waitForCurrentFrame();
    }


    destroyObjectForFrame(renderApp.frameCount);

    GPU_D3D_ASSERT(frame.commandAllocator->Reset(), "Failed to reset command allocator");
    GPU_D3D_ASSERT(frame.commandList->Reset(frame.commandAllocator, nullptr), "Failed to reset command list");

    // Upload texture data
    for (uint32_t index = 0; index < frame.texturesToUpload.getNum(); ++index) {
        GpuTexture* image = frame.texturesToUpload[index];
        ID3D12Resource* texture = image->resource.apiResource;
        ID3D12Resource* uploadBuffer = image->upload.apiResource;
        D3D12_RESOURCE_DESC desc = image->resource.apiResource->GetDesc();

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
        uint32_t numRows = 0;
        uint64_t rowSizeInBytes = 0, totalBytes = 0;
        renderApp.device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, &numRows, &rowSizeInBytes, &totalBytes);
        void* mapped = nullptr;
        uploadBuffer->Map(0, nullptr, &mapped);
        size_t pixelSize = gpu::getDXGIFormatBytes(desc.Format);

        for (uint32_t index = 0; index < numRows * layout.Footprint.Depth; ++index) {
            void* dstAddr = mem::offsetPtr(mapped, (intptr_t)(index * layout.Footprint.RowPitch));
            const void* srcAddr = mem::offsetPtr((void*)image->cpuData, (intptr_t)(index * (desc.Width * pixelSize)));
            memcpy(dstAddr, srcAddr, (size_t)rowSizeInBytes);
        }
        uploadBuffer->Unmap(0, nullptr);

        D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.pResource = uploadBuffer;
        srcLocation.PlacedFootprint = layout;
        D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.pResource = texture;
        dstLocation.SubresourceIndex = 0;

        D3D12_RESOURCE_BARRIER textureBufferBarrier[1] = {};
        textureBufferBarrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        textureBufferBarrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        textureBufferBarrier[0].Transition.Subresource = 0;
        textureBufferBarrier[0].Transition.pResource = texture;
        textureBufferBarrier[0].Transition.StateBefore = image->resource.state;
        textureBufferBarrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;

        frame.commandList->ResourceBarrier(1, textureBufferBarrier);
        frame.commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

        textureBufferBarrier[0].Transition.StateAfter = image->resource.state;
        textureBufferBarrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        frame.commandList->ResourceBarrier(1, textureBufferBarrier);

        free((void*)image->cpuData);
        image->cpuData = nullptr;
        addObjectToDestroyForCurrentFrame(image->upload.apiResource);
        image->upload.apiResource = nullptr;
    }
    frame.texturesToUpload.reset();

    // Upload buffer data
    for (uint32_t index = 0; index < frame.buffersToUpload.getNum(); ++index) {
        GpuBuffer* buffer = frame.buffersToUpload[index];
        if (buffer->cpuData == nullptr || buffer->resource.apiResource == nullptr) continue;
        ID3D12Resource* resource = buffer->resource.apiResource;
        ID3D12Resource* uploadResource = buffer->upload.apiResource;
        D3D12_RESOURCE_DESC desc = buffer->resource.apiResource->GetDesc();
        D3D12_RESOURCE_BARRIER barrier[1] = {};

        void* mapped = nullptr;
        if (uploadResource->Map(0, nullptr, &mapped) == S_OK) {
            memcpy(mapped, buffer->cpuData, buffer->sizeInBytes);
            uploadResource->Unmap(0, nullptr);

            barrier[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier[0].Transition.Subresource = 0;
            barrier[0].Transition.pResource = resource;
            barrier[0].Transition.StateBefore = buffer->resource.state;
            barrier[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
            frame.commandList->ResourceBarrier(1, barrier);
            frame.commandList->CopyResource(resource, uploadResource);
            barrier[0].Transition.StateAfter = buffer->resource.state;
            barrier[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
            frame.commandList->ResourceBarrier(1, barrier);
        }

        free((void*)buffer->cpuData);
        buffer->cpuData = nullptr;
        addObjectToDestroyForCurrentFrame(buffer->upload.apiResource);
        buffer->upload.apiResource = nullptr;
    }
    frame.buffersToUpload.reset();

    frame.state = GpuFrameState::STARTED;
    frame.backbuffer = &renderApp.backbuffers[renderApp.presentFrame].texture;

    return &frame;
}

void gpu::endFrame() {
    GpuFrameData& frame = renderApp.frames[renderApp.currentFrame];

    GpuFixedResourceBarrierBatcher<1> presentBarrier;
    presentBarrier.transition(frame.backbuffer->resource, D3D12_RESOURCE_STATE_PRESENT);
    presentBarrier.flush(frame.commandList);

    GPU_D3D_ASSERT(frame.commandList->Close(), "Failed to close command list");
    ID3D12CommandList* commandLists[] = { frame.commandList };
    renderApp.commandQueue->ExecuteCommandLists(1, commandLists);
    GPU_D3D_ASSERT(renderApp.commandQueue->Signal(frame.fence, ++frame.frameWaitValue), "Failed to signal frame fence");
    renderApp.currentFrame = (renderApp.currentFrame + 1) % GPU_FRAME_NUM;
    frame.state = GpuFrameState::EXECUTING;
}

void gpu::present(bool vsync) 
{
    if (win::shouldResize(renderApp.window))
    {
        gpu::waitForAllFrames();

        for (uint32_t index = 0; index < GPU_BACKBUFFER_NUM; ++index)
        {
            if (renderApp.backbuffers[index].fence->GetCompletedValue() < renderApp.backbuffers[index].fenceValue)
            {
                renderApp.backbuffers[index].fence->SetEventOnCompletion(renderApp.backbuffers[index].fenceValue, renderApp.backbuffers[index].event);
                WaitForSingleObject(renderApp.backbuffers[index].event, INFINITE);
            }
        }

        for (uint32_t index = 0; index < GPU_BACKBUFFER_NUM; ++index)
        {
            GpuTexture* texture = &renderApp.backbuffers[index].texture;
            GPU_D3D_RELEASE(renderApp.backbuffers[index].texture.resource.apiResource);
            GPU_D3D_RELEASE(renderApp.backbuffers[index].fence);
            CloseHandle(renderApp.backbuffers[index].event);
        }

        DXGI_SWAP_CHAIN_DESC swapChainDesc{};
        renderApp.swapChain->GetDesc(&swapChainDesc);
        if (renderApp.swapChain->ResizeBuffers(swapChainDesc.BufferCount, renderApp.window->resizeUint[0], renderApp.window->resizeUint[1], swapChainDesc.BufferDesc.Format, swapChainDesc.Flags) == S_OK)
        {
            for (uint32_t index = 0; index < swapChainDesc.BufferCount; ++index)
            {
                renderApp.swapChain->GetBuffer(index, IID_PPV_ARGS(&renderApp.backbuffers[index].texture.resource.apiResource));
                renderApp.backbuffers[index].texture.resource.setName(L"gpu::backbuffer %u", index);
                renderApp.backbuffers[index].texture.resource.state = D3D12_RESOURCE_STATE_PRESENT;
                D3D12_RESOURCE_DESC desc = renderApp.backbuffers[index].texture.resource.apiResource->GetDesc();
                renderApp.backbuffers[index].texture.width = (uint32_t)desc.Width;
                renderApp.backbuffers[index].texture.height = (uint32_t)desc.Height;
                renderApp.backbuffers[index].texture.depth = (uint32_t)desc.DepthOrArraySize;
                GPU_D3D_ASSERT(renderApp.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&renderApp.backbuffers[index].fence)), "Failed to create fence");
                renderApp.backbuffers[index].fence->SetName(str::format(L"gpu::backbuffer::fence %u", index));
                renderApp.backbuffers[index].event = CreateEvent(nullptr, false, false, nullptr);;
                renderApp.backbuffers[index].fenceValue = 0;
            }

            renderApp.window->sizeUint[0] = renderApp.window->resizeUint[0];
            renderApp.window->sizeUint[1] = renderApp.window->resizeUint[1];
            renderApp.window->size = Float2((float)renderApp.window->sizeUint[0], (float)renderApp.window->sizeUint[1]);
            renderApp.window->aspectRatio = renderApp.window->size.x / renderApp.window->size.y;
            renderApp.presentFrame = 0;
        }
        else
        {
            DBG_PANIC("Failed to resize swap chain");
        }

    }

    if (renderApp.backbuffers[renderApp.presentFrame].fence->GetCompletedValue() < renderApp.backbuffers[renderApp.presentFrame].fenceValue) 
    {
        renderApp.backbuffers[renderApp.presentFrame].fence->SetEventOnCompletion(renderApp.backbuffers[renderApp.presentFrame].fenceValue, renderApp.backbuffers[renderApp.presentFrame].event);
        WaitForSingleObject(renderApp.backbuffers[renderApp.presentFrame].event, INFINITE);
    }

    GPU_D3D_ASSERT(renderApp.swapChain->Present(vsync ? 1 : 0, 0), "Failed to present");
    renderApp.presentFrame = (renderApp.presentFrame + 1) % GPU_BACKBUFFER_NUM;
    GPU_D3D_ASSERT(renderApp.commandQueue->Signal(renderApp.backbuffers[renderApp.presentFrame].fence, ++renderApp.backbuffers[renderApp.presentFrame].fenceValue), "Failed to signal present fence");
    for (std::function<void()> callback : renderApp.presentCallbacks)
    {
        callback();
    }
    renderApp.presentCallbacks.clear();
    renderApp.frameCount++;
}

GpuDescriptorAllocator* gpu::createDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorNum, D3D12_DESCRIPTOR_HEAP_FLAGS flags) {
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    descriptorHeapDesc.Type = type;
    descriptorHeapDesc.NumDescriptors = descriptorNum;
    descriptorHeapDesc.Flags = flags;
    descriptorHeapDesc.NodeMask = 0;
    ID3D12DescriptorHeap* descriptorHeap = nullptr;
    GpuDescriptorAllocator* descriptorAllocator = new GpuDescriptorAllocator();
    GPU_D3D_ASSERT(renderApp.device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorAllocator->descriptorHeap)), "Failed to create descriptor heap");
    descriptorAllocator->descriptorAllocated = 0;
    descriptorAllocator->gpuBaseHandle = descriptorAllocator->descriptorHeap->GetGPUDescriptorHandleForHeapStart();
    descriptorAllocator->cpuBaseHandle = descriptorAllocator->descriptorHeap->GetCPUDescriptorHandleForHeapStart();
    descriptorAllocator->descriptorHandleSize = renderApp.device->GetDescriptorHandleIncrementSize(type);
    descriptorAllocator->descriptorCapacity = descriptorNum;
    return descriptorAllocator;
}

void gpu::destroyDescriptorAllocator(GpuDescriptorAllocator* descriptorAllocator) {
    addObjectToDestroyForCurrentFrame(descriptorAllocator->descriptorHeap);
    delete descriptorAllocator;
}

ID3D12PipelineState* gpu::createGraphicsPipelineState(const wchar_t* name, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc) {
    ID3D12PipelineState* pso = nullptr;
    GPU_D3D_ASSERT(renderApp.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)), "Failed to create graphics pipeline state");
    pso->SetName(name);
    return pso;
}

ID3D12PipelineState* gpu::createComputePipelineState(const wchar_t* name, const D3D12_COMPUTE_PIPELINE_STATE_DESC& psoDesc) {
    ID3D12PipelineState* pso = nullptr;
    GPU_D3D_ASSERT(renderApp.device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)), "Failed to create compute pipeline state");
    pso->SetName(name);
    return pso;
}

ID3D12PipelineState* gpu::createGraphicsPipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc) {
    ID3D12PipelineState* pso = nullptr;
    GPU_D3D_ASSERT(renderApp.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso)), "Failed to create graphics pipeline state");
    return pso;
}

ID3D12PipelineState* gpu::createComputePipelineState(const D3D12_COMPUTE_PIPELINE_STATE_DESC& psoDesc) {
    ID3D12PipelineState* pso = nullptr;
    GPU_D3D_ASSERT(renderApp.device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso)), "Failed to create compute pipeline state");
    return pso;
}

GpuResource gpu::createResource(size_t sizeInBytes, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState, bool initToZero, D3D12_RESOURCE_FLAGS flags) {
    D3D12_RESOURCE_DESC resourceDesc = {
        D3D12_RESOURCE_DIMENSION_BUFFER,
        D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        mem::alignSize(sizeInBytes, 256),
        1,
        1,
        1,
        DXGI_FORMAT_UNKNOWN,
        { 1, 0},
        D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        flags
    };
    D3D12_HEAP_PROPERTIES heapProps = {
        heapType,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0
    };
    D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
    if (!initToZero) {
        heapFlags |= D3D12_HEAP_FLAG_CREATE_NOT_ZEROED;
    }
    ID3D12Resource* resource = nullptr;
    GPU_D3D_ASSERT(renderApp.device->CreateCommittedResource(
        &heapProps,
        heapFlags,
        &resourceDesc, initialState, nullptr,
        IID_PPV_ARGS(&resource)),
        "Failed to create buffer resource");

    return { resource, initialState };
}

GpuBuffer* gpu::createBuffer(const wchar_t* name, size_t bufferSize, D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType, const void* initialData, bool initToZero, D3D12_RESOURCE_FLAGS flags) {
    GpuBuffer* buffer = createBuffer(bufferSize, initialState, heapType, initialData, initToZero, flags);
    buffer->resource.apiResource->SetName(name);
    return buffer;
}

GpuBuffer* gpu::createBuffer(size_t bufferSize, D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType, const void* initialData, bool initToZero, D3D12_RESOURCE_FLAGS flags) {
    GpuBuffer* buffer = new GpuBuffer();
    buffer->resource = createResource(bufferSize, heapType, initialState, initToZero, flags);
    buffer->sizeInBytes = buffer->resource.apiResource->GetDesc().Width;
    buffer->cpuData = nullptr;
    if (initialData) {
        buffer->upload = createResource(bufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, false, D3D12_RESOURCE_FLAG_NONE);
        buffer->cpuData = malloc(bufferSize);
        memcpy((void*)buffer->cpuData, initialData, bufferSize);
        getFrameData().buffersToUpload.add(buffer);
    } else {
        buffer->upload.apiResource = nullptr;
    }
    buffer->refCounter = new TAtomicRefCounter<uint64_t>(1);
    return buffer;
}

D3D12_CPU_DESCRIPTOR_HANDLE gpu::getRenderTargetViewCPUHandle() {
    return renderApp.rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

D3D12_GPU_DESCRIPTOR_HANDLE gpu::getRenderTargetViewGPUHandle() {
    return renderApp.rtvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

D3D12_CPU_DESCRIPTOR_HANDLE gpu::getDepthStencilViewCPUHandle() {
    return renderApp.dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}
D3D12_GPU_DESCRIPTOR_HANDLE gpu::getDepthStencilViewGPUHandle() {
    return renderApp.dsvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

uint32_t gpu::calcNumUint32FromSize(size_t size) {
    return (uint32_t)(size / sizeof(uint32_t));
}

size_t gpu::getDXGIFormatBits(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
    case DXGI_FORMAT_R32G32B32A32_UINT:
    case DXGI_FORMAT_R32G32B32A32_SINT:
        return 128;

    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_FLOAT:
    case DXGI_FORMAT_R32G32B32_UINT:
    case DXGI_FORMAT_R32G32B32_SINT:
        return 96;

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
    case DXGI_FORMAT_R16G16B16A16_UNORM:
    case DXGI_FORMAT_R16G16B16A16_UINT:
    case DXGI_FORMAT_R16G16B16A16_SNORM:
    case DXGI_FORMAT_R16G16B16A16_SINT:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R32G32_FLOAT:
    case DXGI_FORMAT_R32G32_UINT:
    case DXGI_FORMAT_R32G32_SINT:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_Y416:
    case DXGI_FORMAT_Y210:
    case DXGI_FORMAT_Y216:
        return 64;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_UNORM:
    case DXGI_FORMAT_R10G10B10A2_UINT:
    case DXGI_FORMAT_R11G11B10_FLOAT:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R16G16_FLOAT:
    case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
    case DXGI_FORMAT_R16G16_SNORM:
    case DXGI_FORMAT_R16G16_SINT:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
    case DXGI_FORMAT_R32_UINT:
    case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    case DXGI_FORMAT_AYUV:
    case DXGI_FORMAT_Y410:
    case DXGI_FORMAT_YUY2:
        return 32;

    case DXGI_FORMAT_P010:
    case DXGI_FORMAT_P016:
        return 24;

    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R8G8_UNORM:
    case DXGI_FORMAT_R8G8_UINT:
    case DXGI_FORMAT_R8G8_SNORM:
    case DXGI_FORMAT_R8G8_SINT:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R16_FLOAT:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
    case DXGI_FORMAT_R16_UINT:
    case DXGI_FORMAT_R16_SNORM:
    case DXGI_FORMAT_R16_SINT:
    case DXGI_FORMAT_B5G6R5_UNORM:
    case DXGI_FORMAT_B5G5R5A1_UNORM:
    case DXGI_FORMAT_A8P8:
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 16;

    case DXGI_FORMAT_NV12:
    case DXGI_FORMAT_420_OPAQUE:
    case DXGI_FORMAT_NV11:
        return 12;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
    case DXGI_FORMAT_BC2_TYPELESS:
    case DXGI_FORMAT_BC2_UNORM:
    case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_TYPELESS:
    case DXGI_FORMAT_BC3_UNORM:
    case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC5_TYPELESS:
    case DXGI_FORMAT_BC5_UNORM:
    case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_TYPELESS:
    case DXGI_FORMAT_BC6H_UF16:
    case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_TYPELESS:
    case DXGI_FORMAT_BC7_UNORM:
    case DXGI_FORMAT_BC7_UNORM_SRGB:
    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
        return 8;

    case DXGI_FORMAT_R1_UNORM:
        return 1;

    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        return 4;

    default:
        return 0;
    }
}

size_t gpu::getDXGIFormatBytes(DXGI_FORMAT format) {
    return getDXGIFormatBits(format) / 8;
}

GpuTexture* gpu::createTexture(const wchar_t* name, uint32_t width, uint32_t height, uint32_t depth, const void* pixels, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags) {
    GpuTexture* texture = createTexture(width, height, depth, pixels, initialState, dxgiFormat, flags);
    texture->resource.apiResource->SetName(name);
    return texture;
}

GpuTexture* gpu::createTexture(uint32_t width, uint32_t height, uint32_t depth, const void* pixels, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags) {
    GpuTexture* texture = new GpuTexture();
    D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    if (width > 1 && height > 1 && depth > 1) {
        dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    }
    else if (width >= 1 && height >= 1 && depth == 1) {
        dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    }
    else if (width >= 1 && height == 1 && depth == 1) {
        dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
    }

    D3D12_RESOURCE_DESC resourceDesc = {
        dimension,
        D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
        (uint64_t)width,
        height,
        (uint16_t)depth,
        1,
        dxgiFormat,
        { 1,  0},
        D3D12_TEXTURE_LAYOUT_UNKNOWN,
        flags
    };
    D3D12_HEAP_PROPERTIES heapProps = {
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        D3D12_MEMORY_POOL_UNKNOWN,
        0,
        0
    };

    D3D12_CLEAR_VALUE* clearValuePtr = nullptr;
    D3D12_CLEAR_VALUE clearValue = {};

    if ((flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) > 0) {
        clearValue.Format = dxgiFormat;
        clearValue.DepthStencil.Depth = 1;
        clearValue.DepthStencil.Stencil = 0;
        clearValuePtr = &clearValue;
    }

    if ((flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) > 0) {
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 1.0f;
        clearValue.Format = dxgiFormat;
        clearValuePtr = &clearValue;
    }

    GPU_D3D_ASSERT(gpu::getDevice()->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES, &resourceDesc, initialState, clearValuePtr, IID_PPV_ARGS(&texture->resource.apiResource)), "Failed to create image resource");
    texture->resource.state = initialState;
    texture->width = width;
    texture->height = height;
    texture->depth = depth;

    if (pixels != nullptr) {
        size_t pixelSize = gpu::getDXGIFormatBytes(dxgiFormat);
        uint64_t alignedWidth = mem::alignSize(width, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
        uint64_t dataSize = alignedWidth * height * depth * pixelSize;
        uint64_t bufferSize = dataSize < 256 ? 256 : dataSize;
        texture->upload = gpu::createResource((size_t)bufferSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, false, D3D12_RESOURCE_FLAG_NONE);
        texture->cpuData = malloc(pixelSize * width * height * depth);
        DBG_ASSERT(texture->cpuData != nullptr, "Failed to allocate cpu data for uploading to texture memory");
        if (texture->cpuData != nullptr) {
            memcpy((void*)texture->cpuData, pixels, pixelSize * width * height * depth);
        }
        getFrameData().texturesToUpload.add(texture);
    }
    texture->refCounter = new TAtomicRefCounter<uint64_t>(1);
    return texture;
}

void gpu::destroyTexture(GpuTexture*& texture) {
    if (texture == nullptr) return;
    if (texture->refCounter && !texture->refCounter->dec()) return;

    addObjectToDestroyForCurrentFrame(texture->resource.apiResource);
    addObjectToDestroyForCurrentFrame(texture->upload.apiResource);
    delete texture->refCounter;
    if (texture->refCounter) delete texture;
    texture = nullptr;
}

uint64_t gpu::getFrameCount() {
    return renderApp.frameCount;
}

uint64_t gpu::getFrameIndex(int64_t offset)
{
    int64_t wrapped = (((int64_t)renderApp.frameCount + offset) % GPU_FRAME_NUM + GPU_FRAME_NUM) % GPU_FRAME_NUM;
    return (uint64_t)wrapped;
}

void GpuDescriptorTable::reset() {
    allocated = 0;
}

GpuDescriptorHandle GpuDescriptorTable::allocate() {
    DBG_ASSERT(allocated + 1 <= capacity, "Exceeded capacity of descriptors allocated");
    GpuDescriptorHandle handle = { gpuHandle(allocated), cpuHandle(allocated) };
    allocated += 1;
    return handle;
}

void GpuDescriptorTable::allocUAVBuffer(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint64_t firstElement, uint32_t numElements, uint32_t structureByteStride, uint64_t counterOffsetInBytes, bool rawBuffer) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = firstElement;
    uavDesc.Buffer.NumElements = numElements;
    uavDesc.Buffer.StructureByteStride = structureByteStride;
    uavDesc.Buffer.CounterOffsetInBytes = counterOffsetInBytes;
    uavDesc.Buffer.Flags = rawBuffer ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

    if (rawBuffer) {
        DBG_ASSERT(format == DXGI_FORMAT_R32_TYPELESS && structureByteStride == 0, "RAW UAV requires R32_TYPELESS, stride=0");
        uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        DBG_ASSERT(counter == nullptr, "RAW UAV cannot have a counter");
    }
    else if (structureByteStride) {
        DBG_ASSERT(format == DXGI_FORMAT_UNKNOWN, "Structured UAV must use UNKNOWN format");
        uavDesc.Format = DXGI_FORMAT_UNKNOWN;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        // counter valid only for structured
    }
    else {
        DBG_ASSERT(format != DXGI_FORMAT_UNKNOWN, "Typed UAV must specify a typed DXGI_FORMAT");
        uavDesc.Format = format;
        uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
        DBG_ASSERT(counter == nullptr, "Typed UAV cannot have a counter");
    }

    gpu::getDevice()->CreateUnorderedAccessView(resource.apiResource, counter ? counter->apiResource : nullptr, &uavDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocUAVTex1D(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint32_t mipSlice) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
    uavDesc.Texture1D.MipSlice = mipSlice;
    gpu::getDevice()->CreateUnorderedAccessView(resource.apiResource, counter ? counter->apiResource : nullptr, &uavDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocUAVTex1DArray(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
    uavDesc.Texture1DArray.MipSlice = mipSlice;
    uavDesc.Texture1DArray.FirstArraySlice = firstArraySlice;
    uavDesc.Texture1DArray.ArraySize = arraySize;
    gpu::getDevice()->CreateUnorderedAccessView(resource.apiResource, counter ? counter->apiResource : nullptr, &uavDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocUAVTex2D(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t planeSlice) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = mipSlice;
    uavDesc.Texture2D.PlaneSlice = planeSlice;
    gpu::getDevice()->CreateUnorderedAccessView(resource.apiResource, counter ? counter->apiResource : nullptr, &uavDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocUAVTex2DArray(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize, uint32_t planeSlice) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    uavDesc.Texture2DArray.MipSlice = mipSlice;
    uavDesc.Texture2DArray.PlaneSlice = planeSlice;
    uavDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
    uavDesc.Texture2DArray.ArraySize = arraySize;
    gpu::getDevice()->CreateUnorderedAccessView(resource.apiResource, counter ? counter->apiResource : nullptr, &uavDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocUAVTex3D(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstWSlice, uint32_t wSize) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = format;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
    uavDesc.Texture3D.MipSlice = mipSlice;
    uavDesc.Texture3D.FirstWSlice = firstWSlice;
    uavDesc.Texture3D.WSize = wSize;
    gpu::getDevice()->CreateUnorderedAccessView(resource.apiResource, counter ? counter->apiResource : nullptr, &uavDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocSRVBuffer(GpuResource& resource, DXGI_FORMAT format, uint64_t firstElement, uint32_t numElements, uint32_t structureByteStride, bool rawBuffer) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Buffer.FirstElement = firstElement;
    srvDesc.Buffer.NumElements = numElements;
    srvDesc.Buffer.StructureByteStride = structureByteStride;
    srvDesc.Buffer.Flags = rawBuffer ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;

    if (rawBuffer) {
        DBG_ASSERT(format == DXGI_FORMAT_R32_TYPELESS && structureByteStride == 0, "RAW SRV requires R32_TYPELESS, stride=0");
        srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    }
    else if (structureByteStride) {
        DBG_ASSERT(format == DXGI_FORMAT_UNKNOWN, "Structured SRV must use UNKNOWN format");
        srvDesc.Format = DXGI_FORMAT_UNKNOWN;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    }
    else {
        DBG_ASSERT(format != DXGI_FORMAT_UNKNOWN, "Typed SRV must specify a typed DXGI_FORMAT");
        srvDesc.Format = format;
        srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    }

    gpu::getDevice()->CreateShaderResourceView(resource.apiResource, &srvDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocSRVTex1D(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture1D.MostDetailedMip = mostDetailedMip;
    srvDesc.Texture1D.MipLevels = mipLevels;
    srvDesc.Texture1D.ResourceMinLODClamp = resourceMinLODClamp;
    gpu::getDevice()->CreateShaderResourceView(resource.apiResource, &srvDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocSRVTex1DArray(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t firstArraySlice, uint32_t arraySize, float resourceMinLODClamp) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture1DArray.MostDetailedMip = mostDetailedMip;
    srvDesc.Texture1DArray.MipLevels = mipLevels;
    srvDesc.Texture1DArray.FirstArraySlice = firstArraySlice;
    srvDesc.Texture1DArray.ArraySize = arraySize;
    srvDesc.Texture1DArray.ResourceMinLODClamp = resourceMinLODClamp;
    gpu::getDevice()->CreateShaderResourceView(resource.apiResource, &srvDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocSRVTex2D(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t planeSlice, float resourceMinLODClamp) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;
    srvDesc.Texture2D.MipLevels = mipLevels;
    srvDesc.Texture2D.PlaneSlice = planeSlice;
    srvDesc.Texture2D.ResourceMinLODClamp = resourceMinLODClamp;
    gpu::getDevice()->CreateShaderResourceView(resource.apiResource, &srvDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocSRVTex2DArray(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t firstArraySlice, uint32_t arraySize, uint32_t planeSlice, float resourceMinLODClamp) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2DArray.MostDetailedMip = mostDetailedMip;
    srvDesc.Texture2DArray.MipLevels = mipLevels;
    srvDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
    srvDesc.Texture2DArray.ArraySize = arraySize;
    srvDesc.Texture2DArray.PlaneSlice = planeSlice;
    srvDesc.Texture2DArray.ResourceMinLODClamp = resourceMinLODClamp;
    gpu::getDevice()->CreateShaderResourceView(resource.apiResource, &srvDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocSRVTex3D(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture3D.MostDetailedMip = mostDetailedMip;
    srvDesc.Texture3D.MipLevels = mipLevels;
    srvDesc.Texture3D.ResourceMinLODClamp = resourceMinLODClamp;
    gpu::getDevice()->CreateShaderResourceView(resource.apiResource, &srvDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocSRVTexCube(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.TextureCube.MostDetailedMip = mostDetailedMip;
    srvDesc.TextureCube.MipLevels = mipLevels;
    srvDesc.TextureCube.ResourceMinLODClamp = resourceMinLODClamp;
    gpu::getDevice()->CreateShaderResourceView(resource.apiResource, &srvDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocRTVTex2D(GpuResource& resource, DXGI_FORMAT format, uint32_t mipSlice, uint32_t planeSlice) {
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = mipSlice;
    rtvDesc.Texture2D.PlaneSlice = planeSlice;
    gpu::getDevice()->CreateRenderTargetView(resource.apiResource, &rtvDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocDSVTex2D(GpuResource& resource, DXGI_FORMAT format, uint32_t mipSlice, D3D12_DSV_FLAGS flags) {
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = format;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Flags = flags;
    dsvDesc.Texture2D.MipSlice = mipSlice;
    gpu::getDevice()->CreateDepthStencilView(resource.apiResource, &dsvDesc, allocate().cpuHandle);
}

void GpuDescriptorTable::allocCBVBuffer(GpuResource& resource, size_t sizeInBytes)
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = resource.apiResource->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = UINT((sizeInBytes + 255) & ~255u); // align up to 256
    DBG_ASSERT(cbvDesc.SizeInBytes <= 65536, "CBV view must be <= 64KB");
    gpu::getDevice()->CreateConstantBufferView(&cbvDesc, allocate().cpuHandle);
}

D3D12_GPU_DESCRIPTOR_HANDLE GpuDescriptorTable::gpuHandle(uint64_t index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handle = gpuBaseHandle;
    handle.ptr += ((size_t)index * handleSize);
    return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE GpuDescriptorTable::cpuHandle(uint64_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = cpuBaseHandle;
    handle.ptr += ((size_t)index * handleSize);
    return handle;
}


namespace gpu {
    RenderApp::RenderApp() {
    }
    RenderApp::~RenderApp() {
    }
}
