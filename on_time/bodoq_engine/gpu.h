#pragma once

#include <stdint.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <math.h>
#include <utility>
#include <functional>

#include "array.h"
#include "debug.h"
#include "window.h"
#include "string.h"
#include "ref_counter.h"

///////////////////////////////////////////////////////////////
// CONFIG 
///////////////////////////////////////////////////////////////

#define GPU_FRAME_NUM 2
#define GPU_BACKBUFFER_NUM 2
#define GPU_RENDERER 1

static_assert(GPU_FRAME_NUM >= GPU_BACKBUFFER_NUM, "Frame numbers must be equal or larger than backbuffer numbers");
static_assert(GPU_BACKBUFFER_NUM >= 2, "Backbuffer numbers numbers must be greater or equeal to 2");

///////////////////////////////////////////////////////////////

#define GPU_D3D_ASSERT(x, ...) if ((x) != S_OK) { DBG_LOG(__VA_ARGS__); }
#define GPU_D3D_RELEASE(obj)  if ((obj)) { (obj)->Release(); obj = nullptr; }
#define COLOR_UINT(color) (((color) & 0xff) << 24) | ((((color) >> 8) & 0xff) << 16) | ((((color) >> 16) & 0xff) << 8) | ((color) >> 24)
#define COLOR_RGBA_UINT(r, g, b, a) ((uint32_t)(r)) | ((uint32_t)(g) << 8) | ((uint32_t)(b) << 16) | ((uint32_t)(a) << 24)
#define COLOR_RGB_UINT(r, g, b) COLOR_RGBA_UINT(r, g, b, 0xff)
#define COLOR_RGBA_FLOAT(r, g, b, a) COLOR_RGBA_UINT((uint8_t)((r) * 255.0f), (uint8_t)((g) * 255.0f), (uint8_t)((b) * 255.0f), (uint8_t)((a) * 255.0f))
#define COLOR_RGB_FLOAT(r, g, b) COLOR_RGBA_FLOAT(r, g, b, 1.0f)

struct GpuRootSignatureDescriptorRange
{
	GpuRootSignatureDescriptorRange() {}
	~GpuRootSignatureDescriptorRange() {}

	GpuRootSignatureDescriptorRange& addRange(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t descriptorNum, uint32_t baseShaderRegister, uint32_t registerSpace);
	const D3D12_DESCRIPTOR_RANGE* operator*() const { return ranges.getData(); }
	uint32_t getNum() const { return ranges.getNum(); }

private:
	TArray<D3D12_DESCRIPTOR_RANGE, uint32_t> ranges;
};

struct GpuRootSignatureBuilder
{
	GpuRootSignatureBuilder() {}
	~GpuRootSignatureBuilder() {}

	void addRootParameterCBV(uint32_t shaderRegister, uint32_t registerSapce, D3D12_SHADER_VISIBILITY shaderVisibility);
	void addRootParameterSRV(uint32_t shaderRegister, uint32_t registerSapce, D3D12_SHADER_VISIBILITY shaderVisibility);
	void addRootParameterUAV(uint32_t shaderRegister, uint32_t registerSapce, D3D12_SHADER_VISIBILITY shaderVisibility);
	void addRootParameterDescriptorTable(const D3D12_DESCRIPTOR_RANGE* ranges, uint32_t rangeNum, D3D12_SHADER_VISIBILITY shaderVisibility);
	void addRootParameterDescriptorTable(const GpuRootSignatureDescriptorRange& ranges, D3D12_SHADER_VISIBILITY shaderVisibility);
	void addRootParameterConstant(uint32_t shaderRegister, uint32_t registerSpace, uint32_t num32BitValues, D3D12_SHADER_VISIBILITY shaderVisibility);
	void addStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addressModeAll, uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility);
	ID3D12RootSignature* build(bool isCompute);

private:
	TArray<D3D12_ROOT_PARAMETER, uint32_t> rootParameters;
	TArray<D3D12_STATIC_SAMPLER_DESC, uint32_t> staticSamplers;
};

struct GpuResource
{
	bool isValid() const { return apiResource != nullptr; }
	void setName(const WString& fmt, ...)
	{
		if (isValid()) 
		{
			va_list args;
			va_start(args, &fmt);
			WString name = str::formatv(*fmt, args);
			apiResource->SetName(*name);
			va_end(args);
		}
	}

	ID3D12Resource* apiResource = nullptr;
	D3D12_RESOURCE_STATES state;
};

template<uint32_t NUM_BARRIERS>
struct GpuFixedResourceBarrierBatcher
{
	struct BarrierData
	{
		D3D12_RESOURCE_BARRIER barrier;
		GpuResource* resource1;
		GpuResource* resource2;
	};

	GpuFixedResourceBarrierBatcher()
	{
		memset(barriers, 0, sizeof(barriers));
		barrierNum = 0;
	}

	void transition(GpuResource& resource, D3D12_RESOURCE_STATES afterState)
	{
		if (resource.state != afterState)
		{
			BarrierData data = { {}, &resource, nullptr };
			data.barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			data.barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			data.barrier.Transition.Subresource = 0;
			data.barrier.Transition.pResource = resource.apiResource;
			data.barrier.Transition.StateBefore = resource.state;
			data.barrier.Transition.StateAfter = afterState;
			barriers[barrierNum++] = data;
		}
	}

	void reset()
	{
		barrierNum = 0;
	}

	void flush(ID3D12GraphicsCommandList* commandList)
	{
		if (barrierNum == 0) return;
		D3D12_RESOURCE_BARRIER barrierArray[NUM_BARRIERS] = {};
		for (uint32_t index = 0; index < barrierNum; ++index)
		{
			barrierArray[index] = barriers[index].barrier;
		}
		commandList->ResourceBarrier(barrierNum, barrierArray);
		for (uint32_t index = 0; index < barrierNum; ++index)
		{
			BarrierData& data = barriers[index];
			if (data.barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION) {
				data.resource1->state = data.barrier.Transition.StateAfter;
			}
		}
		reset();
	}

	BarrierData barriers[NUM_BARRIERS];
	uint32_t barrierNum;
};

struct GpuResourceBarrierBatcher
{

	struct BarrierData
	{
		D3D12_RESOURCE_BARRIER barrier;
		GpuResource* resource1;
		GpuResource* resource2;
	};

	GpuResourceBarrierBatcher() = default;

	void transition(GpuResource& resource, D3D12_RESOURCE_STATES afterState)
	{
		if (resource.state != afterState) {
			BarrierData data = { {}, &resource, nullptr };
			data.barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			data.barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			data.barrier.Transition.Subresource = 0;
			data.barrier.Transition.pResource = resource.apiResource;
			data.barrier.Transition.StateBefore = resource.state;
			data.barrier.Transition.StateAfter = afterState;
			barriers.add(data);
		}
	}

	void reset()
	{
		barriers.reset();
	}

	void flush(ID3D12GraphicsCommandList* commandList)
	{
		if (barriers.getNum() == 0) return;
		nativeBarriers.reset();
		for (uint32_t index = 0; index < barriers.getNum(); ++index)
		{
			nativeBarriers.add(barriers[index].barrier);
		}
		commandList->ResourceBarrier(nativeBarriers.getNum(), nativeBarriers.getData());
		for (uint32_t index = 0; index < barriers.getNum(); ++index)
		{
			BarrierData& data = barriers[index];
			if (data.barrier.Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
			{
				data.resource1->state = data.barrier.Transition.StateAfter;
			}
		}
		reset();
	}

	TArray<BarrierData, uint32_t> barriers;
	TArray<D3D12_RESOURCE_BARRIER, uint32_t> nativeBarriers;
};

struct GpuDescriptorHandle
{

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
};

struct GpuDescriptorTable
{
	void reset();
	GpuDescriptorHandle allocate();
	void allocUAVBuffer(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint64_t firstElement, uint32_t numElements, uint32_t structureByteStride, uint64_t counterOffsetInBytes, bool rawBuffer = false);
	void allocUAVTex1D(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint32_t mipSlice);
	void allocUAVTex1DArray(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize);
	void allocUAVTex2D(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t planeSlice);
	void allocUAVTex2DArray(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize, uint32_t planeSlice);
	void allocUAVTex3D(GpuResource& resource, GpuResource* counter, DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstWSlice, uint32_t wSize);
	void allocSRVBuffer(GpuResource& resource, DXGI_FORMAT format, uint64_t firstElement, uint32_t numElements, uint32_t structureByteStride, bool rawBuffer = false);
	void allocSRVTex1D(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp);
	void allocSRVTex1DArray(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t firstArraySlice, uint32_t arraySize, float resourceMinLODClamp);
	void allocSRVTex2D(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t planeSlice, float resourceMinLODClamp);
	void allocSRVTex2DArray(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t firstArraySlice, uint32_t arraySize, uint32_t planeSlice, float resourceMinLODClamp);
	void allocSRVTex3D(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp);
	void allocSRVTexCube(GpuResource& resource, DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp);
	void allocRTVTex2D(GpuResource& resource, DXGI_FORMAT format, uint32_t mipSlice, uint32_t planeSlice);
	void allocDSVTex2D(GpuResource& resource, DXGI_FORMAT format, uint32_t mipSlice, D3D12_DSV_FLAGS flags);
	void allocCBVBuffer(GpuResource& resource, size_t sizeInBytes);

	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle(uint64_t index);
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle(uint64_t index);
	D3D12_GPU_DESCRIPTOR_HANDLE gpuBaseHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuBaseHandle;
	uint32_t handleSize;
	uint32_t allocated;
	uint32_t capacity;
};

struct GpuDescriptorAllocator
{

	void reset();
	GpuDescriptorTable allocateDescriptorTable(uint32_t descriptorNum);
	uint32_t remaining() const { return descriptorCapacity - descriptorAllocated; }

	ID3D12DescriptorHeap* descriptorHeap;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuBaseHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuBaseHandle;
	uint32_t descriptorHandleSize;
	uint32_t descriptorAllocated;
	uint32_t descriptorCapacity;
};

enum class GpuFrameState
{
	STARTED,
	EXECUTING,
	FINISHED
};

struct GpuTexture
{
	GpuResource resource;
	GpuResource upload;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	const void* cpuData;
	TAtomicRefCounter<uint64_t>* refCounter;
};

struct GpuBuffer
{
	size_t getSizeInBytes() const { return resource.apiResource->GetDesc().Width; }
	GpuResource resource;
	GpuResource upload;
	size_t sizeInBytes;
	const void* cpuData;
	TAtomicRefCounter<uint64_t>* refCounter;
};

struct GpuFrameData
{
	ID3D12GraphicsCommandList* commandList;
	ID3D12CommandAllocator* commandAllocator;
	ID3D12Fence* fence;
	HANDLE fenceEvent;
	uint64_t frameWaitValue;
	uint64_t frameIndex;
	void* userData;
	TArray<GpuTexture*> texturesToUpload;
	TArray<GpuBuffer*> buffersToUpload;
	TArray<ID3D12Object*> objectsToDestroy;
	GpuFrameState state;
	GpuTexture* backbuffer;
};

struct GpuBackbuffer
{
	GpuTexture texture;
	ID3D12Fence* fence;
	uint64_t fenceValue;
	HANDLE event;
};

struct GpuDescriptorRangeEntry
{

	GpuDescriptorRangeEntry(D3D12_DESCRIPTOR_RANGE_TYPE type, uint32_t descriptorNum, uint32_t baseShaderRegister, uint32_t registerSpace) :
		type(type), descriptorNum(descriptorNum), baseShaderRegister(baseShaderRegister), registerSpace(registerSpace)
	{
	}

	GpuDescriptorRangeEntry() = default;

	D3D12_DESCRIPTOR_RANGE_TYPE type;
	uint32_t descriptorNum;
	uint32_t baseShaderRegister;
	uint32_t registerSpace;
};

struct GpuDescriptorRange
{
	static constexpr uint32_t MAX_DESCRIPTOR_RANGES = 8;

	GpuDescriptorRange() = default;

	template<typename... TArgs>
	GpuDescriptorRange(TArgs... args) : rangeNum(0)
	{
		memset(ranges, 0, sizeof(ranges));
		addRangesImpl(args...);
	}

	template<typename... TArgs>
	GpuDescriptorRange& addRanges(TArgs... args)
	{
		return addRangesImpl(args...);
	}

	const D3D12_DESCRIPTOR_RANGE& operator[](uint32_t index) const { return ranges[index]; }
	uint32_t getRangeNum() const { return rangeNum; }

	GpuDescriptorRange& addRange(const GpuDescriptorRangeEntry& rangeEntry)
	{
		DBG_ASSERT(rangeNum < MAX_DESCRIPTOR_RANGES - 1, "Exceeded the limit of descriptor range %u.", MAX_DESCRIPTOR_RANGES);
		D3D12_DESCRIPTOR_RANGE range = {};
		range.RangeType = rangeEntry.type;
		range.NumDescriptors = rangeEntry.descriptorNum;
		range.BaseShaderRegister = rangeEntry.baseShaderRegister;
		range.RegisterSpace = rangeEntry.registerSpace;
		range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		ranges[rangeNum++] = range;
		return *this;
	}
private:
	GpuDescriptorRange& addRangesImpl() { return *this; }
	template<typename T, typename... TArgs>
	GpuDescriptorRange& addRangesImpl(const T& range, TArgs... args)
	{
		addRange(range);
		return addRangesImpl(args...);
	}


	D3D12_DESCRIPTOR_RANGE ranges[MAX_DESCRIPTOR_RANGES];
	uint32_t rangeNum = 0;
};

struct GpuBindingLayout
{
	static constexpr uint32_t MAX_ROOT_PARAMETERS = 8;
	static constexpr uint32_t MAX_STATIC_SAMPLERS = 4;

	GpuBindingLayout() : rootParameterNum(0), staticSamplerNum(0)
	{
		memset(rootParameters, 0, sizeof(rootParameters));
		memset(staticSamplers, 0, sizeof(staticSamplers));
	}
	void addDescriptorCBV(uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility)
	{
		DBG_ASSERT(rootParameterNum + 1 <= MAX_ROOT_PARAMETERS, "Exceeded the limit of root parameters %u.", MAX_ROOT_PARAMETERS);
		D3D12_ROOT_PARAMETER rootParam = {};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParam.Descriptor.RegisterSpace = registerSpace;
		rootParam.Descriptor.ShaderRegister = shaderRegister;
		rootParam.ShaderVisibility = shaderVisibility;
		rootParameters[rootParameterNum++] = rootParam;
	}
	void addDescriptorSRV(uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility)
	{
		DBG_ASSERT(rootParameterNum + 1 <= MAX_ROOT_PARAMETERS, "Exceeded the limit of root parameters %u.", MAX_ROOT_PARAMETERS);
		D3D12_ROOT_PARAMETER rootParam = {};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParam.Descriptor.RegisterSpace = registerSpace;
		rootParam.Descriptor.ShaderRegister = shaderRegister;
		rootParam.ShaderVisibility = shaderVisibility;
		rootParameters[rootParameterNum++] = rootParam;
	}
	void addDescriptorUAV(uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility)
	{
		DBG_ASSERT(rootParameterNum + 1 <= MAX_ROOT_PARAMETERS, "Exceeded the limit of root parameters %u.", MAX_ROOT_PARAMETERS);
		D3D12_ROOT_PARAMETER rootParam = {};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		rootParam.Descriptor.RegisterSpace = registerSpace;
		rootParam.Descriptor.ShaderRegister = shaderRegister;
		rootParam.ShaderVisibility = shaderVisibility;
		rootParameters[rootParameterNum++] = rootParam;
	}
	void addDescriptorTable(const GpuDescriptorRange& descriptorRange, D3D12_SHADER_VISIBILITY shaderVisibility)
	{
		DBG_ASSERT(rootParameterNum + 1 <= MAX_ROOT_PARAMETERS, "Exceeded the limit of root parameters %u.", MAX_ROOT_PARAMETERS);
		D3D12_DESCRIPTOR_RANGE* rangeOffset = (D3D12_DESCRIPTOR_RANGE*)(descriptorTableRanges.getNum());
		for (uint32_t index = 0; index < descriptorRange.getRangeNum(); ++index) {
			const D3D12_DESCRIPTOR_RANGE& range = descriptorRange[index];
			descriptorTableRanges.add(range);
		}

		D3D12_ROOT_PARAMETER rootParam = {};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam.DescriptorTable.pDescriptorRanges = rangeOffset;
		rootParam.DescriptorTable.NumDescriptorRanges = descriptorRange.getRangeNum();
		rootParam.ShaderVisibility = shaderVisibility;
		rootParameters[rootParameterNum++] = rootParam;
	}
	void add32BitConstant(uint32_t shaderRegister, uint32_t registerSpace, uint32_t num32BitValues, D3D12_SHADER_VISIBILITY shaderVisibility)
	{
		DBG_ASSERT(rootParameterNum + 1 <= MAX_ROOT_PARAMETERS, "Exceeded the limit of root parameters %u.", MAX_ROOT_PARAMETERS);
		D3D12_ROOT_PARAMETER rootParam = {};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParam.Constants.Num32BitValues = num32BitValues;
		rootParam.Constants.RegisterSpace = registerSpace;
		rootParam.Constants.ShaderRegister = shaderRegister;
		rootParam.ShaderVisibility = shaderVisibility;
		rootParameters[rootParameterNum++] = rootParam;
	}
	void addStaticSampler(D3D12_FILTER filter, D3D12_TEXTURE_ADDRESS_MODE addressModeAll, uint32_t shaderRegister, uint32_t registerSpace, D3D12_SHADER_VISIBILITY shaderVisibility)
	{
		DBG_ASSERT(staticSamplerNum + 1 <= MAX_STATIC_SAMPLERS, "Exceeded the limit of static samplers %u.", MAX_STATIC_SAMPLERS);
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
		staticSamplers[staticSamplerNum++] = staticSampler;
	}

	D3D12_ROOT_PARAMETER rootParameters[MAX_ROOT_PARAMETERS];
	D3D12_STATIC_SAMPLER_DESC staticSamplers[MAX_STATIC_SAMPLERS];
	TArray<D3D12_DESCRIPTOR_RANGE> descriptorTableRanges;
	uint32_t rootParameterNum;
	uint32_t staticSamplerNum;
};

struct GpuVertexAttribute
{
	DXGI_FORMAT format;
	const char* semanticName;
	uint32_t semanticIndex;
	uint32_t offset;
};

struct GpuVertexBufferLayout
{

	GpuVertexBufferLayout& addVertexAttribute(DXGI_FORMAT format, const char* semanticName, uint32_t semanticIndex, uint32_t offset)
	{
		GpuVertexAttribute& attrib = vertexAttributes.emplace();
		attrib.format = format;
		attrib.semanticName = semanticName;
		attrib.semanticIndex = semanticIndex;
		attrib.offset = offset;
		return *this;
	}

	TFixedArray<GpuVertexAttribute, 8> vertexAttributes;
	uint32_t vertexStride;
};

struct GpuVertexStage
{
	GpuVertexBufferLayout& addVertexBuffer(uint32_t vertexStride)
	{
		GpuVertexBufferLayout& vertexBuffer = vertexBuffers.emplace();
		vertexBuffer.vertexStride = vertexStride;
		return vertexBuffer;
	}

	D3D12_SHADER_BYTECODE shader;
	TFixedArray<GpuVertexBufferLayout, 4> vertexBuffers;
};

struct GpuPixelStage
{
	D3D12_SHADER_BYTECODE shader;
	TArray<DXGI_FORMAT, uint32_t> renderTargets;
	DXGI_FORMAT depthStencilFormat = DXGI_FORMAT_UNKNOWN;
};

struct GpuRasterizerStage
{
	D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_NONE;
};

struct GpuComputePipelineDesc
{
	GpuComputePipelineDesc() = default;
	GpuBindingLayout layout;
	D3D12_SHADER_BYTECODE shader;
};

struct GpuGraphicsPipelineDesc
{
	GpuGraphicsPipelineDesc() = default;
	GpuBindingLayout layout;
	GpuVertexStage vertex;
	GpuPixelStage pixel;
	GpuRasterizerStage rasterizer;
};

struct GpuPipelineState
{
	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* pso;
};

namespace gpu 
{
	void init(Window* window, bool enableValidations = false, bool enablePix = false);
	void setFrameUserData(uint32_t frame, void* data);
	void waitForCurrentFrame();
	void waitForAllFrames();
	void destroy();
	GpuFrameData& getFrameData();
	ID3D12Device* getDevice();
	GpuFrameData* beginFrame();
	void endFrame();
	void present(bool vsync = true);
	GpuDescriptorAllocator* createDescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t descriptorNum, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
	void destroyDescriptorAllocator(GpuDescriptorAllocator* descriptorAllocator);
	ID3D12PipelineState* createGraphicsPipelineState(const wchar_t* name, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
	ID3D12PipelineState* createComputePipelineState(const wchar_t* name, const D3D12_COMPUTE_PIPELINE_STATE_DESC& psoDesc);
	ID3D12PipelineState* createGraphicsPipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& psoDesc);
	ID3D12PipelineState* createComputePipelineState(const D3D12_COMPUTE_PIPELINE_STATE_DESC& psoDesc);
	GpuResource createResource(size_t sizeInBytes, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState, bool initToZero, D3D12_RESOURCE_FLAGS flags);
	GpuBuffer* createBuffer(const wchar_t* name, size_t bufferSize, D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType, const void* initialData = nullptr, bool initToZero = false, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	GpuBuffer* createBuffer(size_t bufferSize, D3D12_RESOURCE_STATES initialState, D3D12_HEAP_TYPE heapType, const void* initialData = nullptr, bool initToZero = false, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	D3D12_CPU_DESCRIPTOR_HANDLE getRenderTargetViewCPUHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE getRenderTargetViewGPUHandle();
	D3D12_CPU_DESCRIPTOR_HANDLE getDepthStencilViewCPUHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE getDepthStencilViewGPUHandle();
	template<typename T> uint32_t calcNumUint32FromSize() { return sizeof(T) / sizeof(uint32_t); }
	uint32_t calcNumUint32FromSize(size_t size);
	size_t getDXGIFormatBits(DXGI_FORMAT format);
	size_t getDXGIFormatBytes(DXGI_FORMAT format);
	GpuTexture* createTexture(const wchar_t* name, uint32_t width, uint32_t height, uint32_t depth, const void* pixels, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	GpuTexture* createTexture(uint32_t width, uint32_t height, uint32_t depth, const void* pixels, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	void destroyTexture(GpuTexture*& image);
	uint64_t getFrameCount();
	uint64_t getFrameIndex(int64_t offset = 0);
	GpuPipelineState* buildComputePipelineState(const GpuComputePipelineDesc& desc);
	GpuPipelineState* buildGraphicsPipelineState(GpuGraphicsPipelineDesc& desc);
	void destroyPipelineState(GpuPipelineState*& pipelineState);
	void destroyBuffer(GpuBuffer*& buffer);
	ID3D12CommandQueue* getCommandQueue();
	void addOnPresentCallback(std::function<void()> callback);
}
