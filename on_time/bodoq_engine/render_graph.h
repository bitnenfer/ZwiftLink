#pragma once

#include "gpu.h"
#include "array.h"
#include "memory.h"
#include <functional>

enum class RgResourceType
{
	BUFFER,
	TEXTURE
};

enum class RgResourceAccess
{
	INVALID,
	UAV,
	SRV,
	RTV,
	DSV,
	CBV,
	COPY_SRC,
	COPY_DST,
	VBV,
	IBV
};

enum class RgPassType
{
	NONE,
	COMPUTE,
	RASTER,
	COPY
};

struct RgResource
{
	static const RgResource Null;
	operator uint64_t() const { return index; }

	uint64_t index;
};
inline bool operator==(const RgResource& a, const RgResource& b) { return a.index == b.index; }
inline bool operator!=(const RgResource& a, const RgResource& b) { return a.index != b.index; }

struct RgRenderTarget
{
	RgResource resource;
	D3D12_RENDER_TARGET_VIEW_DESC desc;
};

struct RgDepthStencilTarget
{
	RgResource resource;
	D3D12_DEPTH_STENCIL_VIEW_DESC desc;
};

struct RgView
{
	static D3D12_UNORDERED_ACCESS_VIEW_DESC UAVBuffer(DXGI_FORMAT format, uint64_t firstElement, uint32_t numElements, uint32_t structureByteStride, uint64_t counterOffsetInBytes, bool rawBuffer = false);
	static D3D12_UNORDERED_ACCESS_VIEW_DESC UAVTex1D(DXGI_FORMAT format, uint32_t mipSlice);
	static D3D12_UNORDERED_ACCESS_VIEW_DESC UAVTex1DArray(DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize);
	static D3D12_UNORDERED_ACCESS_VIEW_DESC UAVTex2D(DXGI_FORMAT format, uint32_t mipSlice, uint32_t planeSlice);
	static D3D12_UNORDERED_ACCESS_VIEW_DESC UAVTex2DArray(DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize, uint32_t planeSlice);
	static D3D12_UNORDERED_ACCESS_VIEW_DESC UAVTex3D(DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstWSlice, uint32_t wSize);
	static D3D12_SHADER_RESOURCE_VIEW_DESC SRVBuffer(DXGI_FORMAT format, uint64_t firstElement, uint32_t numElements, uint32_t structureByteStride, bool rawBuffer = false);
	static D3D12_SHADER_RESOURCE_VIEW_DESC SRVTex1D(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp);
	static D3D12_SHADER_RESOURCE_VIEW_DESC SRVTex1DArray(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t firstArraySlice, uint32_t arraySize, float resourceMinLODClamp);
	static D3D12_SHADER_RESOURCE_VIEW_DESC SRVTex2D(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t planeSlice, float resourceMinLODClamp);
	static D3D12_SHADER_RESOURCE_VIEW_DESC SRVTex2DArray(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t firstArraySlice, uint32_t arraySize, uint32_t planeSlice, float resourceMinLODClamp);
	static D3D12_SHADER_RESOURCE_VIEW_DESC SRVTex3D(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp);
	static D3D12_SHADER_RESOURCE_VIEW_DESC SRVTexCube(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp);
	static D3D12_RENDER_TARGET_VIEW_DESC RTVTex2D(DXGI_FORMAT format, uint32_t mipSlice, uint32_t planeSlice);
	static D3D12_DEPTH_STENCIL_VIEW_DESC DSVTex2D(DXGI_FORMAT format, uint32_t mipSlice, D3D12_DSV_FLAGS flags);
	static D3D12_CONSTANT_BUFFER_VIEW_DESC CBVBuffer(size_t sizeInBytes);
	static D3D12_VERTEX_BUFFER_VIEW VBVBuffer(size_t sizeInBytes, size_t strideInBytes);
	static D3D12_INDEX_BUFFER_VIEW IBVBuffer(DXGI_FORMAT format, size_t sizeInBytes);
};

struct RgDescriptorTable
{
	D3D12_CPU_DESCRIPTOR_HANDLE alloc();
	D3D12_CPU_DESCRIPTOR_HANDLE getCPUDescriptor(uint32_t slot) const;

	D3D12_CPU_DESCRIPTOR_HANDLE cpu;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu;

	uint32_t handleSize;
	uint32_t allocated;
	uint32_t capacity;
};

struct RgDescriptorManager
{
	static ID3D12DescriptorHeap* createDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_DESC desc);

	RgDescriptorManager(uint32_t frameCount, uint32_t gpuCapacity, uint32_t rtvAndDsvCapacity);
	~RgDescriptorManager();
	D3D12_CPU_DESCRIPTOR_HANDLE allocRTV();
	D3D12_CPU_DESCRIPTOR_HANDLE allocDSV();
	D3D12_CPU_DESCRIPTOR_HANDLE allocCPU();
	RgDescriptorTable allocGPU(uint32_t count = 1);
	ID3D12DescriptorHeap* getDescriptorHeapGPU() { return gpuDescriptorHeaps[frame]; }
	void beginFrame();
private:
	friend struct RgPassRasterContext;
	friend struct RgPassComputeContext;
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* dsvDescriptorHeap = nullptr;
	ID3D12DescriptorHeap* gpuStagingDescriptorHeap = nullptr;
	TArray<ID3D12DescriptorHeap*> gpuDescriptorHeaps;
	uint32_t gpuCapacity = 0;
	uint32_t rtvAndDsvCapacity = 0;
	uint32_t rtvIncrement = 0;
	uint32_t dsvIncrement = 0;
	uint32_t shaderVisibleIncrement = 0;
	uint32_t rtvUsed = 0;
	uint32_t dsvUsed = 0;
	uint32_t cpuUsed = 0;
	uint32_t gpuUsed = 0;
	uint32_t frame = 0;
	uint32_t maxFrames = 0;
};

struct RgResourceEntry
{
	String name;
	RgResourceType type;
	GpuTexture* texture;
	GpuBuffer* buffer;
	GpuResource* resource;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	uint64_t index;
	bool transient;
};

struct RgTransientResourceAllocator
{
	RgTransientResourceAllocator(size_t sizeInBytes, D3D12_HEAP_TYPE heapType);
	~RgTransientResourceAllocator();
	void beginFrame();
	ID3D12Resource* createResource(const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue = nullptr);
	GpuTexture* createTexture(uint32_t width, uint32_t height, uint32_t depth, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	GpuBuffer* createBuffer(size_t bufferSize, D3D12_RESOURCE_STATES initialState, bool initToZero = false, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	void waitForFence();
	void signalFence();

private:
	ID3D12Heap* heap = nullptr;
	ID3D12Fence* fence = nullptr;
	uint64_t fenceValue = 0;
	HANDLE fenceEvent = 0;
	LinearAllocator objAllocator;
	int64_t capacity = 0;
	int64_t offset = 0;
};

struct RgTransientResourceSlot
{
	RgResource resource;
	uint64_t slotIndex;
	bool isUsed;
};

struct RgTransientResourceCreation
{
	RgResource resource;
	RgResourceType type;
	String name;
	D3D12_RESOURCE_FLAGS flags;
	D3D12_RESOURCE_STATES initialState;
	union
	{
		struct
		{
			uint32_t width;
			uint32_t height;
			uint32_t depth;
			DXGI_FORMAT format;
		} texture;
		struct
		{
			size_t bufferSize;
			bool initToZero;
		} buffer;
	};
};

struct RgResourceRegistry
{
	RgResourceRegistry();
	~RgResourceRegistry();
	RgResource createTransientTexture(const String& name, uint32_t width, uint32_t height, uint32_t depth, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	RgResource createTransientBuffer(const String& name, size_t bufferSize, D3D12_RESOURCE_STATES initialState, bool initToZero = false, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	RgResource addTexture(const String& name, GpuTexture* texture, bool isRoot, bool transient = false);
	RgResource addBuffer(const String& name, GpuBuffer* buffer, bool isRoot, bool transient = false);
	RgResource setTexture(const String& name, GpuTexture* texture, bool transient = false);
	RgResource setBuffer(const String& name, GpuBuffer* buffer, bool transient = false);
	RgResource setTexture(const String& name, const RgResource& resource, GpuTexture* texture, bool transient = false);
	RgResource setBuffer(const String& name, const RgResource& resource, GpuBuffer* buffer, bool transient = false);
	RgResourceEntry* get(const RgResource& resource);
	RgResourceEntry* get(const String& name);
	const RgResourceEntry* get(const RgResource& resource)const;
	const RgResourceEntry* get(const String& name) const;
	RgResource getHandle(const String& name) const;
	GpuTexture* getTexture(const RgResource& resource);
	GpuBuffer* getBuffer(const RgResource& resource);
	const GpuTexture* getTexture(const RgResource& resource) const;
	const GpuBuffer* getBuffer(const RgResource& resource) const;
	bool exists(const RgResource& resource) const;
	bool exists(const String& resourceName) const;
	const TArray<RgResource>& getRootResources() const { return rootResources; }
	bool isRootResource(const RgResource& resource) const;
	void createTransientResources(const TArray<RgTransientResourceCreation>& resourcesToCreate);
	TArray<RgTransientResourceCreation> getPassResourcesToCreate();
	struct ReleaseData
	{
		String name;
		ID3D12Resource* resource;
	};
private:
	bool findResourceFromPool(RgResource& outResource);
	void releaseFrameResources(uint32_t frame);
	void endFrame();
	void beginFrame();
	bool findTransientSlot(RgTransientResourceSlot& outSlot);

	friend struct RenderGraph;
	TArray<RgTransientResourceCreation> transientResourceToCreate;
	TArray<RgResourceEntry> resources;
	TArray<RgResource> rootResources;
	TArray<RgResource> resourcePool;
	TArray<RgTransientResourceAllocator*> transientAlloctors;
	TArray<RgTransientResourceSlot> transientResources;
	
	TArray<TArray<ReleaseData>> frameResourceToRelease;
};

struct RgResourceUseReference
{
	RgResource resource;
	D3D12_RESOURCE_STATES state;
	RgResourceAccess access;
	bool writes;
};

struct RgPassDesc
{
	String name;
	GpuPipelineState* pso;
};

struct RgPassEntryBase
{
	virtual RgPassType getType() const { return RgPassType::NONE; }
	virtual void execute(uint64_t passIndex, RgResourceRegistry* registry, ID3D12GraphicsCommandList* commandList) = 0;
	virtual void getReadsAndWrites(TArray<RgResource>& reads, TArray<RgResource>& writes) = 0;
	virtual bool shouldRun() = 0;
	void setDesc(const RgPassDesc& inDesc) { desc = inDesc; }
	RgPassDesc desc;
	uint64_t index = 0;
};

enum class RgBindingType
{
	UAV,
	SRV,
	CBV
};

struct RgBinding
{
	static RgBinding UAV(const RgResource& resource, const RgResource* counter, D3D12_UNORDERED_ACCESS_VIEW_DESC desc)
	{
		RgBinding bind = {};
		bind.type = RgBindingType::UAV;
		bind.resource = resource;
		bind.uav.counter = counter;
		bind.uav.desc = desc;
		return bind;
	}

	static RgBinding SRV(const RgResource& resource, D3D12_SHADER_RESOURCE_VIEW_DESC desc)
	{
		RgBinding bind = {};
		bind.type = RgBindingType::SRV;
		bind.resource = resource;
		bind.srv.desc = desc;
		return bind;
	}

	static RgBinding CBV(const RgResource& resource, D3D12_CONSTANT_BUFFER_VIEW_DESC desc)
	{
		RgBinding bind = {};
		bind.type = RgBindingType::CBV;
		bind.resource = resource;
		bind.cbv.desc = desc;
		return bind;
	}

	RgBindingType type;
	RgResource resource;
	union
	{
		struct
		{
			const RgResource* counter;
			D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
		} uav;
		struct
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC desc;
		} srv;
		struct
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
		} cbv;
	};
};

struct RgPassComputeContext
{
	void bindDescriptorToSlot(const RgDescriptorTable& descriptorTable, uint32_t slot, const RgBinding& binding)
	{
		switch (binding.type)
		{
		case RgBindingType::UAV:
			bindUAV(descriptorTable, slot, binding.resource, binding.uav.counter, binding.uav.desc);
			break;
		case RgBindingType::SRV:
			bindSRV(descriptorTable, slot, binding.resource, binding.srv.desc);
			break;
		case RgBindingType::CBV:
			bindCBV(descriptorTable, slot, binding.resource, binding.cbv.desc);
			break;
		}
	}
	void bindResources(uint32_t rootParameterIndex, const TArray<RgBinding>& bindings)
	{
		RgDescriptorTable descTable = descMgr->allocGPU((uint32_t)bindings.getNum());
		for (uint32_t slot = 0; const RgBinding& binding : bindings)
		{
			bindDescriptorToSlot(descTable, slot++, binding);
		}
		setRootDescriptorTable(rootParameterIndex, descTable);
	}
	void set32BitConstants(uint32_t rootParameter, uint32_t numValues, const void* data, uint32_t destOffset);
	template<typename T>
	void set32BitConstants(uint32_t rootParameter, const T& data, uint32_t destOffset)
	{
		set32BitConstants(rootParameter, gpu::calcNumUint32FromSize<T>(), &data, destOffset);
	}
	void dispatch(uint32_t x, uint32_t y, uint32_t z);

	size_t getBufferSizeInBytes(const RgResource& resource);
	RgResource setTexture(const String& name, const RgResource& resource, GpuTexture* texture);
	RgResource setBuffer(const String& name, const RgResource& resource, GpuBuffer* buffer);

	ID3D12GraphicsCommandList* cmd;

protected:
	virtual void setRootDescriptorTable(uint32_t rootParameterIndex, const RgDescriptorTable& descTable)
	{
		cmd->SetComputeRootDescriptorTable(rootParameterIndex, descTable.gpu);
	}
	void bindUAV(const RgDescriptorTable& descriptorTable, uint32_t slot, const RgResource& resource, const RgResource* counter, D3D12_UNORDERED_ACCESS_VIEW_DESC desc);
	void bindSRV(const RgDescriptorTable& descriptorTable, uint32_t slot, const RgResource& resource, D3D12_SHADER_RESOURCE_VIEW_DESC desc);
	void bindCBV(const RgDescriptorTable& descriptorTable, uint32_t slot, const RgResource& resource, D3D12_CONSTANT_BUFFER_VIEW_DESC desc);

	friend struct RgComputePassEntry;
	virtual void beginFrame(ID3D12GraphicsCommandList* commandList, RgResourceRegistry* registry, RgDescriptorManager* descriptorManager)
	{
		cmd = commandList;
		reg = registry;
		descMgr = descriptorManager;
	}

	RgResourceRegistry* reg;
	RgDescriptorManager* descMgr;
};

enum class RgPrimitiveTopology
{
	TRIANGLE_LIST
};

enum class RgClearFlag : uint8_t
{
	NONE = 0,
	CLEAR_COLOR = 1 << 0,
	CLEAR_DEPTH = 1 << 1,
	CLEAR_STENCIL = 1 << 2,
	CLEAR_DEPTH_STENCIL = (1 << 1) | (1 << 2),
	CLEAR_ALL = (1 << 1) | (1 << 2) | (1 << 0)
};

inline RgClearFlag operator|(RgClearFlag a, RgClearFlag b)
{
	return (RgClearFlag)((uint8_t)a | (uint8_t)b);
}

inline bool renderGraphClearHasFlag(RgClearFlag value, RgClearFlag test)
{
	return ((uint8_t)value & (uint8_t)test) != 0;
}

struct RgPassRasterContext : public RgPassComputeContext
{
	void setScissorRect(
		int32_t left,
		int32_t top,
		int32_t right,
		int32_t bottom
	);

	void setViewport(
		float topLeftX,
		float topLeftY,
		float width,
		float height,
		float minDepth,
		float maxDepth
	);
	
	void setPrimitiveTopology(RgPrimitiveTopology topology);
	
	void drawInstanced(
		uint32_t vertexCountPerInstance, 
		uint32_t instanceCount, 
		uint32_t startVertexLocation, 
		uint32_t startInstanceLocation);

	void drawIndexedInstanced(
		uint32_t indexCountPerInstance,
		uint32_t instanceCount,
		uint32_t startIndexLocation,
		int32_t baseVertexLocation,
		uint32_t startInstanceLocation
	);

	void setVertexBuffer(const RgResource& vertexBuffer, uint32_t slot, size_t sizeInBytes, size_t strideInBytes);
	void setVertexBuffer(const RgResource& vertexBuffer, uint32_t slot, size_t strideInBytes);
	void setIndexBuffer(const RgResource& indexBuffer, size_t sizeInBytes, DXGI_FORMAT indexFormat);
	void setRenderTargets(const TArray<RgRenderTarget>& renderTargets, RgDepthStencilTarget* depthStencilTarget, RgClearFlag clearFlag = RgClearFlag::NONE);
	void setRenderTarget(const RgRenderTarget& renderTarget, RgDepthStencilTarget* depthStencilTarget, RgClearFlag clearFlag = RgClearFlag::NONE);
	void set32BitConstants(uint32_t rootParameter, uint32_t numValues, const void* data, uint32_t destOffset);
	inline void setRenderTargetClear(Float4 colorClear, float depthClear, uint8_t stencilClear)
	{
		rtClearColor = colorClear;
		dsClearDepth = depthClear;
		dsClearStencil = stencilClear;
	}
	template<typename T>
	void set32BitConstants(uint32_t rootParameter, const T& data, uint32_t destOffset)
	{
		set32BitConstants(rootParameter, gpu::calcNumUint32FromSize<T>(), &data, destOffset);
	}

protected:
	virtual void setRootDescriptorTable(uint32_t rootParameterIndex, const RgDescriptorTable& descTable) override
	{
		cmd->SetGraphicsRootDescriptorTable(rootParameterIndex, descTable.gpu);
	}

	friend struct RgRasterPassEntry;
	virtual void beginFrame(ID3D12GraphicsCommandList* commandList, RgResourceRegistry* registry, RgDescriptorManager* descriptorManager)
	{
		RgPassComputeContext::beginFrame(commandList, registry, descriptorManager);
	}

	Float4 rtClearColor;
	float dsClearDepth;
	uint8_t dsClearStencil;
};

struct RgPass
{
	RgPass(RgResourceRegistry* registry);
	inline RgResource createUAV(const RgResource& resource)
	{
		return createWrite(resource, RgResourceAccess::UAV);
	}
	inline RgResource createSRV(const RgResource& resource)
	{
		return createRead(resource, RgResourceAccess::SRV);
	}
	inline RgResource createCBV(const RgResource& resource)
	{
		return createRead(resource, RgResourceAccess::CBV);
	}
	// Resource Name
	inline RgResource createUAV(const String& resource)
	{
		return createWrite(resource, RgResourceAccess::UAV);
	}
	inline RgResource createSRV(const String& resource)
	{
		return createRead(resource, RgResourceAccess::SRV);
	}
	inline RgResource createCBV(const String& resource)
	{
		return createRead(resource, RgResourceAccess::CBV);
	}
	RgResource createTexture(const String& name, uint32_t width, uint32_t height, uint32_t depth, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	RgResource createBuffer(const String& name, size_t bufferSize, D3D12_RESOURCE_STATES initialState, bool initToZero = false, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void setExecute(bool exec) { shouldExecute = exec; }
	bool getExecute() const { return shouldExecute; }

protected:
	RgResource createRead(const RgResource& resource, RgResourceAccess access);
	RgResource createWrite(const RgResource& resource, RgResourceAccess access);
	RgResource createRead(const String& resourceName, RgResourceAccess access);
	RgResource createWrite(const String& resourceName, RgResourceAccess access);

	void getReadsAndWrites(TArray<RgResource>& reads, TArray<RgResource>& writes);

	friend struct RenderGraph;
	friend struct RgComputePassEntry;
	friend struct RgRasterPassEntry;
	RgResourceRegistry* registry;
	TArray<RgResourceUseReference> uses;
	bool shouldExecute = true;
};

struct RgComputePassEntry : public RgPassEntryBase
{
	template<typename TPassData>
	RgComputePassEntry(
		LinearAllocator& passDataAllocator,
		const RgPassDesc& desc,
		RgResourceRegistry* registry, 
		RgDescriptorManager* descriptorManager, 
		std::function<void(RgPass&, TPassData&)> setupFn, 
		std::function<void(const TPassData&, RgPassComputeContext&)> inExecFn) :
		pass(registry),
		descriptorManager(descriptorManager)
	{
		setDesc(desc);
		passData = new (passDataAllocator.allocate(sizeof(TPassData))) TPassData();
		execFn = [=](void* passData, RgPassComputeContext& ctx)
		{
			inExecFn(*static_cast<TPassData*>(passData), ctx);
		};
		setupFn(pass, *static_cast<TPassData*>(passData));
		resourcesToCreate = registry->getPassResourcesToCreate();
	}
	RgComputePassEntry(RgComputePassEntry&& other) noexcept;
	~RgComputePassEntry();
	void processBarriers(RgResourceRegistry* registry, ID3D12GraphicsCommandList* commandList);
	virtual void execute(uint64_t passIndex, RgResourceRegistry* registry, ID3D12GraphicsCommandList* commandList) override;
	virtual void getReadsAndWrites(TArray<RgResource>& reads, TArray<RgResource>& writes) override;
	virtual RgPassType getType() const { return RgPassType::COMPUTE; }
	virtual bool shouldRun() override { return pass.shouldExecute; }

private:
	friend struct RenderGraph;
	RgPass pass;
	std::function<void(void* passData, RgPassComputeContext&)> execFn = nullptr;
	void* passData = nullptr;
	RgPassComputeContext ctx;
	RgDescriptorManager* descriptorManager = nullptr;
	TArray<RgTransientResourceCreation> resourcesToCreate;
};

struct RgRasterPass : public RgPass
{
	RgRasterPass(RgResourceRegistry* registry) :
		RgPass(registry)
	{}

	inline RgResource createRTV(const RgResource& resource)
	{
		return createWrite(resource, RgResourceAccess::RTV);
	}
	inline RgResource createDSV(const RgResource& resource)
	{
		return createWrite(resource, RgResourceAccess::DSV);
	}
	inline RgResource createVBV(const RgResource& resource)
	{
		return createRead(resource, RgResourceAccess::VBV);
	}
	inline RgResource createIBV(const RgResource& resource)
	{
		return createRead(resource, RgResourceAccess::IBV);
	}
	inline RgResource createRTV(const String& resource)
	{
		return createWrite(resource, RgResourceAccess::RTV);
	}
	inline RgResource createDSV(const String& resource)
	{
		return createWrite(resource, RgResourceAccess::DSV);
	}
	inline RgResource createVBV(const String& resource)
	{
		return createRead(resource, RgResourceAccess::VBV);
	}
	inline RgResource createIBV(const String& resource)
	{
		return createRead(resource, RgResourceAccess::IBV);
	}

private:
	friend struct RgRasterPassEntry;
};

struct RgRasterPassEntry : public RgPassEntryBase
{
	template<typename TPassData>
	RgRasterPassEntry(LinearAllocator& passDataAllocator, const RgPassDesc& desc, RgResourceRegistry* registry, RgDescriptorManager* descriptorManager, std::function<void(RgRasterPass&, TPassData&)> setupFn, std::function<void(const TPassData&, RgPassRasterContext&)> inExecFn) :
		pass(registry),
		descriptorManager(descriptorManager)
	{
		setDesc(desc);
		passData = new (passDataAllocator.allocate(sizeof(TPassData))) TPassData();
		execFn = [=](void* passData, RgPassRasterContext& ctx)
		{
			inExecFn(*static_cast<TPassData*>(passData), ctx);
		};
		setupFn(pass, *static_cast<TPassData*>(passData));
		resourcesToCreate = registry->getPassResourcesToCreate();
	}
	RgRasterPassEntry(RgRasterPassEntry&& other) noexcept;
	~RgRasterPassEntry();
	void processBarriers(RgResourceRegistry* registry, ID3D12GraphicsCommandList* commandList);
	virtual void execute(uint64_t passIndex, RgResourceRegistry* registry, ID3D12GraphicsCommandList* commandList) override;
	virtual void getReadsAndWrites(TArray<RgResource>& reads, TArray<RgResource>& writes) override;
	virtual RgPassType getType() const { return RgPassType::RASTER; }
	virtual bool shouldRun() override { return pass.shouldExecute; }

private:

	friend struct RenderGraph;
	RgRasterPass pass;
	std::function<void(void* passData, RgPassRasterContext&)> execFn = nullptr;
	void* passData = nullptr;
	RgPassRasterContext ctx;
	RgDescriptorManager* descriptorManager = nullptr;
	TArray<RgTransientResourceCreation> resourcesToCreate;
};

struct RgCopyPassData
{
	RgResourceUseReference dest;
	RgResourceUseReference source;
};

struct RgCopyPass
{
	RgCopyPass(RgResourceRegistry* registry);
	void copy(const String& dest, const String& source);
	void copy(const RgResource& dest, const RgResource& source);
	void setExecute(bool exec) { shouldExecute = exec; }
	bool getExecute() const { return shouldExecute; }
	RgResource createTexture(const String& name, uint32_t width, uint32_t height, uint32_t depth, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	RgResource createBuffer(const String& name, size_t bufferSize, D3D12_RESOURCE_STATES initialState, bool initToZero = false, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

private:
	void getReadsAndWrites(TArray<RgResource>& reads, TArray<RgResource>& writes);

	friend struct RgCopyPassEntry;
	TArray<RgCopyPassData> copyResources;
	RgResourceRegistry* registry = nullptr;
	bool shouldExecute = true;
};

struct RgCopyPassEntry : public RgPassEntryBase
{
	RgCopyPassEntry(const String& name, RgResourceRegistry* registry, std::function<void(RgCopyPass&)> setupFn);
	virtual void execute(uint64_t passIndex, RgResourceRegistry* registry, ID3D12GraphicsCommandList* commandList) override;
	virtual void getReadsAndWrites(TArray<RgResource>& reads, TArray<RgResource>& writes) override;
	virtual RgPassType getType() const { return RgPassType::COPY; }
	virtual bool shouldRun() override { return pass.shouldExecute; }
private:

	String name;
	RgCopyPass pass;
};

struct RenderGraph
{
	RenderGraph();
	~RenderGraph();
	template<typename TPassData>
	TPassData addComputePass(const RgPassDesc& desc, std::function<void(RgPass&, TPassData&)> setupFn, std::function<void(const TPassData&, RgPassComputeContext&)> execFn)
	{
		RgComputePassEntry* entry = new(passAllocator.allocate(sizeof(RgComputePassEntry))) RgComputePassEntry(passDataAllocator, desc, &registry, &descriptorManager, setupFn, execFn);
		entry->index = passes.getNum();
		passes.add(entry);
		return *static_cast<TPassData*>(entry->passData);
	}
	template<typename TPassData>
	TPassData addRasterPass(const RgPassDesc& desc, std::function<void(RgRasterPass&, TPassData&)> setupFn, std::function<void(const TPassData&, RgPassRasterContext&)> execFn)
	{
		RgRasterPassEntry* entry = new(passAllocator.allocate(sizeof(RgRasterPassEntry))) RgRasterPassEntry(passDataAllocator, desc, &registry, &descriptorManager, setupFn, execFn);
		entry->index = passes.getNum();
		passes.add(entry);
		return *static_cast<TPassData*>(entry->passData);
	}
	void addCopyPass(const String& name, std::function<void(RgCopyPass&)> setupFn)
	{
		RgCopyPassEntry* entry = new(passAllocator.allocate(sizeof(RgCopyPassEntry))) RgCopyPassEntry(name, &registry, setupFn);
		entry->index = passes.getNum();
		passes.add(entry);
	}

	RgResource importTexture(const String& name, GpuTexture* texture, bool isRoot = false);
	RgResource importBuffer(const String& name, GpuBuffer* buffer, bool isRoot = false);
	RgResource updateTexture(const String& name, GpuTexture* texture);
	RgResource updateBuffer(const String& name, GpuBuffer* buffer);
	RgResource updateTexture(const String& name, const RgResource& resource, GpuTexture* texture);
	RgResource updateBuffer(const String& name, const RgResource& resource, GpuBuffer* buffer);
	RgResource getResource(const String& name);

	void execute(ID3D12GraphicsCommandList* commandList);

private:
	
	void processPass(TArray<RgPassEntryBase*>& builtPasses, RgPassEntryBase* passEntry, const TArray<RgResource>& reads);
	TArray<RgPassEntryBase*> buildPasses();

	LinearAllocator passAllocator;
	LinearAllocator passDataAllocator;
	TArray<RgPassEntryBase*> passes;
	RgResourceRegistry registry;
	RgDescriptorManager descriptorManager;

	friend struct RenderGraphViewer;
};

