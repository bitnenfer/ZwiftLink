#include "render_graph.h"
#include "memory.h"
#include "pix.h"
#include <algorithm>

const RgResource RgResource::Null = { UINT64_MAX };

RenderGraph::RenderGraph() :
	passAllocator(MEM_MiB(2)),
	passDataAllocator(MEM_MiB(2)),
	descriptorManager(GPU_FRAME_NUM, 1024 * 4, 64)
{

}

RenderGraph::~RenderGraph()
{

}

RgResource RenderGraph::importTexture(const String& name, GpuTexture* texture, bool isRoot)
{
	return registry.addTexture(name, texture, isRoot);
}

RgResource RenderGraph::importBuffer(const String& name, GpuBuffer* buffer, bool isRoot)
{
	return registry.addBuffer(name, buffer, isRoot);
}

RgResource RenderGraph::updateTexture(const String& name, GpuTexture* texture)
{
	return registry.setTexture(name, texture);
}

RgResource RenderGraph::updateBuffer(const String& name, GpuBuffer* buffer)
{
	return registry.setBuffer(name, buffer);
}

RgResource RenderGraph::updateTexture(const String& name, const RgResource& resource, GpuTexture* texture)
{
	return registry.setTexture(name, resource, texture);
}

RgResource RenderGraph::updateBuffer(const String& name, const RgResource& resource, GpuBuffer* buffer)
{
	return registry.setBuffer(name, resource, buffer);
}

RgResource RenderGraph::getResource(const String& name)
{
	return registry.getHandle(name);
}

void RenderGraph::execute(ID3D12GraphicsCommandList* commandList)
{
	TArray<RgPassEntryBase*> builtPasses = buildPasses();
	descriptorManager.beginFrame();
	registry.beginFrame();
	ID3D12DescriptorHeap* descriptorHeap = descriptorManager.getDescriptorHeapGPU();
	commandList->SetDescriptorHeaps(1, &descriptorHeap);
	for (uint64_t index = 0; RgPassEntryBase* entry : builtPasses)
	{
		entry->execute(index++, &registry, commandList);
	}
	passes.clear();
	passAllocator.reset();
	passDataAllocator.reset();
	registry.endFrame();
}

void RenderGraph::processPass(TArray<RgPassEntryBase*>& builtPasses, RgPassEntryBase* passEntry, const TArray<RgResource>& reads)
{
	if (passEntry->shouldRun())
	{
		builtPasses.addUnique(passEntry);
		for (const RgResource& read : reads)
		{
			for (int64_t index2 = (int64_t)passEntry->index - 1; index2 >= 0; --index2)
			{
				RgPassEntryBase* passEntry2 = passes[index2];
				TArray<RgResource> reads2;
				TArray<RgResource> writes2;
				passEntry2->getReadsAndWrites(reads2, writes2);
				for (RgResource& write : writes2)
				{
					if (read == write)
					{
						processPass(builtPasses, passEntry2, reads2);
						break;
					}
				}
			}
		}
	}
}

TArray<RgPassEntryBase*> RenderGraph::buildPasses()
{
	TArray<RgPassEntryBase*> builtPasses;
	TArray<RgResource> rootResources = registry.getRootResources();
	TArray<RgPassEntryBase*> passesThatWriteToRoot;
	for (RgResource& root : rootResources)
	{
		for (uint64_t index = 0; index < passes.getNum(); index++)
		{
			RgPassEntryBase* passEntry = passes[index];
			TArray<RgResource> reads;
			TArray<RgResource> writes;
			passEntry->getReadsAndWrites(reads, writes);
			for (RgResource& res : writes)
			{
				if (root == res)
				{
					passesThatWriteToRoot.addUnique(passEntry);
					break;
				}
			}
		}
	}

	for (uint64_t index = 0; index < passesThatWriteToRoot.getNum(); index++)
	{
		RgPassEntryBase* passEntry = passesThatWriteToRoot[index];
		TArray<RgResource> reads;
		TArray<RgResource> writes;
		passEntry->getReadsAndWrites(reads, writes);
		processPass(builtPasses, passEntry, reads);
	}

	std::sort(builtPasses.begin(), builtPasses.end(), [](auto& a, auto& b) -> bool {
		return a->index < b->index;
	});

	return builtPasses;
}

RgComputePassEntry::RgComputePassEntry(RgComputePassEntry&& other) noexcept :
	pass(std::move(other.pass)),
	execFn(std::move(other.execFn)),
	passData(std::move(other.passData)),
	descriptorManager(other.descriptorManager)

{
	setDesc(other.desc);
	other.passData = nullptr;
	other.execFn = nullptr;
	other.descriptorManager = nullptr;
}

RgComputePassEntry::~RgComputePassEntry()
{
	if (passData) delete passData;
}

RgRasterPassEntry::RgRasterPassEntry(RgRasterPassEntry&& other)  noexcept :
	pass(std::move(other.pass)),
	execFn(std::move(other.execFn)),
	passData(std::move(other.passData)),
	descriptorManager(other.descriptorManager)

{
	setDesc(other.desc);
	other.passData = nullptr;
	other.execFn = nullptr;
	other.descriptorManager = nullptr;
}

RgRasterPassEntry::~RgRasterPassEntry()
{
}

void RgRasterPassEntry::processBarriers(RgResourceRegistry* registry, ID3D12GraphicsCommandList* commandList)
{
	GpuResourceBarrierBatcher barriers;
	ID3D12Device* device = gpu::getDevice();
	for (uint64_t index = 0; index < pass.uses.getNum(); ++index)
	{
		RgResourceUseReference& use = pass.uses[index];
		RgResourceEntry* resourceEntry = registry->get(use.resource);
		barriers.transition(*resourceEntry->resource, use.state);
	}
	barriers.flush(commandList);

}

void RgComputePassEntry::processBarriers(RgResourceRegistry* registry, ID3D12GraphicsCommandList* commandList)
{
	GpuResourceBarrierBatcher barriers;
	ID3D12Device* device = gpu::getDevice();
	for (uint64_t index = 0; index < pass.uses.getNum(); ++index)
	{
		RgResourceUseReference& use = pass.uses[index];
		RgResourceEntry* resourceEntry = registry->get(use.resource);
		barriers.transition(*resourceEntry->resource, use.state);
	}
	barriers.flush(commandList);
}

void RgRasterPassEntry::execute(uint64_t passIndex, RgResourceRegistry* registry, ID3D12GraphicsCommandList* commandList)
{
	if (!pass.shouldExecute) return;
	pix::beginEvent(commandList, passIndex, "Raster Pass: %s", *desc.name);
	registry->createTransientResources(resourcesToCreate);
	if (desc.pso)
	{
		commandList->SetPipelineState(desc.pso->pso);
		commandList->SetGraphicsRootSignature(desc.pso->rootSignature);
	}
	processBarriers(registry, commandList);
	ctx.beginFrame(commandList, registry, descriptorManager);
	execFn(passData, ctx);
	pix::endEvent(commandList);
}

void RgRasterPassEntry::getReadsAndWrites(TArray<RgResource>& reads, TArray<RgResource>& writes)
{
	pass.getReadsAndWrites(reads, writes);
}

void RgComputePassEntry::execute(uint64_t passIndex, RgResourceRegistry* registry, ID3D12GraphicsCommandList* commandList)
{
	if (!pass.shouldExecute) return;
	pix::beginEvent(commandList, passIndex, "Compute Pass: %s", *desc.name);
	registry->createTransientResources(resourcesToCreate);
	if (desc.pso)
	{
		commandList->SetPipelineState(desc.pso->pso);
		commandList->SetComputeRootSignature(desc.pso->rootSignature);
	}
	processBarriers(registry, commandList);
	ctx.beginFrame(commandList, registry, descriptorManager);
	execFn(passData, ctx);
	pix::endEvent(commandList);
}

void RgComputePassEntry::getReadsAndWrites(TArray<RgResource>& reads, TArray<RgResource>& writes)
{
	pass.getReadsAndWrites(reads, writes);
}

RgPass::RgPass(RgResourceRegistry* registry) :
	registry(registry)
{
}

inline D3D12_RESOURCE_STATES renderGraphResourceToResourceState(RgResourceAccess access)
{
	switch (access)
	{
	case RgResourceAccess::UAV:
		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	case RgResourceAccess::SRV:
		return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	case RgResourceAccess::RTV:
		return D3D12_RESOURCE_STATE_RENDER_TARGET;
	case RgResourceAccess::DSV:
		return D3D12_RESOURCE_STATE_DEPTH_WRITE;
	case RgResourceAccess::CBV:
		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	case RgResourceAccess::COPY_SRC:
		return D3D12_RESOURCE_STATE_COPY_SOURCE;
	case RgResourceAccess::COPY_DST:
		return D3D12_RESOURCE_STATE_COPY_DEST;
	case RgResourceAccess::VBV:
		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	case RgResourceAccess::IBV:
		return D3D12_RESOURCE_STATE_INDEX_BUFFER;
	}
	DBG_PANIC("Invalid access");
	return D3D12_RESOURCE_STATE_COMMON;
}

RgResource RgPass::createTexture(const String& name, uint32_t width, uint32_t height, uint32_t depth, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags)
{
	return registry->createTransientTexture(name, width, height, depth, initialState, dxgiFormat, flags);
}

RgResource RgPass::createBuffer(const String& name, size_t bufferSize, D3D12_RESOURCE_STATES initialState, bool initToZero, D3D12_RESOURCE_FLAGS flags)
{
	return registry->createTransientBuffer(name, bufferSize, initialState, initToZero, flags);
}

RgResource RgPass::createRead(const RgResource& resource, RgResourceAccess access)
{
	if (registry->exists(resource))
	{
		uses.add({ resource, renderGraphResourceToResourceState(access), access, false });
		return resource;
	}
	return RgResource::Null;
}

RgResource RgPass::createWrite(const RgResource& resource, RgResourceAccess access)
{
	if (registry->exists(resource))
	{
		uses.add({ resource, renderGraphResourceToResourceState(access), access, true });
		return resource;
	}
	return RgResource::Null;
}

RgResource RgPass::createRead(const String& resourceName, RgResourceAccess access)
{
	RgResource handle = registry->getHandle(resourceName);
	if (handle != RgResource::Null)
	{
		uses.add({ handle, renderGraphResourceToResourceState(access), access, false });
		return handle;
	}
	return RgResource::Null;
}

RgResource RgPass::createWrite(const String& resourceName, RgResourceAccess access)
{
	RgResource handle = registry->getHandle(resourceName);
	if (handle != RgResource::Null)
	{
		uses.add({ handle, renderGraphResourceToResourceState(access), access, true });
		return handle;
	}
	return RgResource::Null;
}

void RgPass::getReadsAndWrites(TArray<RgResource>& reads, TArray<RgResource>& writes)
{
	for (RgResourceUseReference& use : uses)
	{
		if (use.writes)
		{
			writes.add(use.resource);
		}
		else
		{
			reads.add(use.resource);
		}
	}
}

RgResourceRegistry::RgResourceRegistry()
{
	for (uint32_t index = 0; index < GPU_FRAME_NUM; ++index)
	{
		transientAlloctors.add(new RgTransientResourceAllocator(MEM_MiB(512), D3D12_HEAP_TYPE_DEFAULT));
		frameResourceToRelease.add({});
	}
}

RgResourceRegistry::~RgResourceRegistry()
{
	TArray<ID3D12Resource*> resourcesToDestroy;
	for (uint32_t index = 0; index < GPU_FRAME_NUM; ++index)
	{
		TArray<ReleaseData>& frameResources = frameResourceToRelease[index];
		for (ReleaseData& data : frameResources)
		{
			resourcesToDestroy.addUnique(data.resource);
		}
	}

	for (ID3D12Resource* resource : resourcesToDestroy)
	{
		GPU_D3D_RELEASE(resource);
	}

	for (uint32_t index = 0; index < GPU_FRAME_NUM; ++index)
	{
		delete transientAlloctors[index];
	}
}

RgResource RgResourceRegistry::createTransientTexture(const String& name, uint32_t width, uint32_t height, uint32_t depth, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags)
{
	RgTransientResourceCreation creationInfo = {};
	creationInfo.type = RgResourceType::TEXTURE;
	creationInfo.name = name;
	creationInfo.flags = flags;
	creationInfo.initialState = initialState;
	creationInfo.texture.width = width; 
	creationInfo.texture.height = height;
	creationInfo.texture.depth = depth;
	creationInfo.texture.format = dxgiFormat;

	RgTransientResourceSlot slot;
	RgResource resource = RgResource::Null;
	if (findTransientSlot(slot))
	{
		transientResources[slot.slotIndex].isUsed = true;
		resource = setTexture(name, slot.resource, nullptr, true);
	}
	else
	{
		resource = addTexture(name, nullptr, false, true);
		transientResources.add({ resource, transientResources.getNum(), true });
	}
	creationInfo.resource = resource;
	transientResourceToCreate.add(creationInfo);
	return resource;
}

RgResource RgResourceRegistry::createTransientBuffer(const String& name, size_t bufferSize, D3D12_RESOURCE_STATES initialState, bool initToZero, D3D12_RESOURCE_FLAGS flags)
{
	RgTransientResourceCreation creationInfo = {};
	creationInfo.type = RgResourceType::BUFFER;
	creationInfo.name = name;
	creationInfo.flags = flags;
	creationInfo.initialState = initialState;
	creationInfo.buffer.bufferSize = bufferSize;
	creationInfo.buffer.initToZero = initToZero;

	RgTransientResourceSlot slot;
	RgResource resource = RgResource::Null;
	if (findTransientSlot(slot))
	{
		transientResources[slot.slotIndex].isUsed = true;
		resource = setBuffer(name, slot.resource, nullptr, true);
	}
	else
	{
		resource = addBuffer(name, nullptr, false, true);
		transientResources.add({ resource, transientResources.getNum(), true });
	}
	creationInfo.resource = resource;
	transientResourceToCreate.add(creationInfo);
	return resource;
}

RgResource RgResourceRegistry::addTexture(const String& name, GpuTexture* texture, bool isRoot, bool transient)
{
	//DBG_ASSERT(!exists(name), "Texture %s already exists", *name);
	if (exists(name))
	{
		return setTexture(name, texture, transient);
	}
	RgResourceEntry entry;
	entry.name = name;
	entry.type = RgResourceType::TEXTURE;
	entry.texture = texture;
	entry.buffer = nullptr;
	entry.resource = texture ? &texture->resource : nullptr;
	entry.index = resources.getNum();
	entry.transient = transient;
	if (texture) texture->resource.setName(str::toStringW(name));

	RgResource handle = { entry.index };
	if (findResourceFromPool(handle))
	{
		resources[handle.index] = entry;
	}
	else
	{
		resources.add(entry);
	}
	if (isRoot)
	{
		rootResources.add(handle);
	}
	return handle;
}

RgResource RgResourceRegistry::addBuffer(const String& name, GpuBuffer* buffer, bool isRoot, bool transient)
{
	//DBG_ASSERT(!exists(name), "Buffer %s already exists", *name);
	if (exists(name))
	{
		return setBuffer(name, buffer, transient);
	}
	RgResourceEntry entry;
	entry.name = name;
	entry.type = RgResourceType::BUFFER;
	entry.texture = nullptr;
	entry.buffer = buffer;
	entry.resource = buffer ? &buffer->resource : nullptr;
	entry.index = resources.getNum();
	entry.transient = transient;
	if (buffer) buffer->resource.setName(str::toStringW(name));
	RgResource handle = { entry.index };
	if (findResourceFromPool(handle))
	{
		resources[handle.index] = entry;
	}
	else
	{
		resources.add(entry);
	}
	if (isRoot)
	{
		rootResources.add(handle);
	}
	return handle;
}

RgResource RgResourceRegistry::setTexture(const String& name, GpuTexture* texture, bool transient)
{
	if (RgResourceEntry* entry = get(name))
	{
		entry->type = RgResourceType::TEXTURE;
		entry->buffer = nullptr;
		entry->texture = texture;
		entry->resource = texture ? &texture->resource : nullptr;
		if (texture) texture->resource.setName(str::toStringW(name));
		entry->transient = transient;
		return { entry->index };
	}
	return RgResource::Null;
}

RgResource RgResourceRegistry::setBuffer(const String& name, GpuBuffer* buffer, bool transient)
{
	if (RgResourceEntry* entry = get(name))
	{
		entry->type = RgResourceType::BUFFER;
		entry->buffer = buffer;
		entry->texture = nullptr;
		entry->resource = buffer ? &buffer->resource : nullptr;
		if (buffer) buffer->resource.setName(str::toStringW(name));
		entry->transient = transient;
		return { entry->index };
	}
	return RgResource::Null;
}

RgResource RgResourceRegistry::setTexture(const String& name, const RgResource& resource, GpuTexture* texture, bool transient)
{
	if (RgResourceEntry* entry = get(resource))
	{
		entry->type = RgResourceType::TEXTURE;
		entry->name = name;
		entry->buffer = nullptr;
		entry->texture = texture;
		entry->resource = texture ? &texture->resource : nullptr;
		if (entry->resource) entry->resource->setName(str::toStringW(name));
		entry->transient = transient;
		return { entry->index };
	}
	return RgResource::Null;
}

RgResource RgResourceRegistry::setBuffer(const String& name, const RgResource& resource, GpuBuffer* buffer, bool transient)
{
	if (RgResourceEntry* entry = get(resource))
	{
		entry->type = RgResourceType::BUFFER;
		entry->name = name;
		entry->buffer = buffer;
		entry->texture = nullptr;
		entry->resource = buffer ? &buffer->resource : nullptr;
		if (entry->resource) entry->resource->setName(str::toStringW(name));
		entry->transient = transient;
		return { entry->index };
	}
	return RgResource::Null;
}

RgResourceEntry* RgResourceRegistry::get(const RgResource& resource)
{
	if (resource == RgResource::Null) return nullptr;
	return &resources[resource.index];
}

RgResourceEntry* RgResourceRegistry::get(const String& name)
{
	for (uint64_t index = 0; index < resources.getNum(); ++index)
	{
		if (resources[index].name == name)
		{
			return &resources[index];
		}
	}
	return nullptr;
}

const RgResourceEntry* RgResourceRegistry::get(const RgResource& resource) const
{
	return &resources[resource.index];
}

const RgResourceEntry* RgResourceRegistry::get(const String& name) const
{
	for (uint64_t index = 0; index < resources.getNum(); ++index)
	{
		if (resources[index].name == name)
		{
			return &resources[index];
		}
	}
	return nullptr;
}

RgResource RgResourceRegistry::getHandle(const String& name) const
{
	for (uint64_t index = 0; index < resources.getNum(); ++index)
	{
		if (resources[index].name == name)
		{
			return { resources[index].index };
		}
	}
	return RgResource::Null;
}

GpuTexture* RgResourceRegistry::getTexture(const RgResource& resource)
{
	RgResourceEntry* entry = get(resource);
	DBG_ASSERT(entry != nullptr, "Invalid resource");
	DBG_ASSERT(entry->type == RgResourceType::TEXTURE, "Resource is not a texture");
	return entry->texture;
}

GpuBuffer* RgResourceRegistry::getBuffer(const RgResource& resource)
{
	RgResourceEntry* entry = get(resource);
	DBG_ASSERT(entry != nullptr, "Invalid resource");
	DBG_ASSERT(entry->type == RgResourceType::BUFFER, "Resource is not a buffer");
	return entry->buffer;
}

const GpuTexture* RgResourceRegistry::getTexture(const RgResource& resource) const
{
	const RgResourceEntry* entry = get(resource);
	DBG_ASSERT(entry != nullptr, "Invalid resource");
	DBG_ASSERT(entry->type == RgResourceType::TEXTURE, "Resource is not a texture");
	return entry->texture;
}

const GpuBuffer* RgResourceRegistry::getBuffer(const RgResource& resource) const
{
	const RgResourceEntry* entry = get(resource);
	DBG_ASSERT(entry != nullptr, "Invalid resource");
	DBG_ASSERT(entry->type == RgResourceType::BUFFER, "Resource is not a buffer");
	return entry->buffer;
}

bool RgResourceRegistry::exists(const RgResource& resource) const
{
	return resource.index >= 0 && resource.index < resources.getNum();
}

bool RgResourceRegistry::exists(const String& resourceName) const
{
	return getHandle(resourceName) != RgResource::Null;
}

bool RgResourceRegistry::isRootResource(const RgResource& resource) const
{
	for (const RgResource& rootResource : rootResources)
	{
		if (rootResource == resource)
		{
			return true;
		}
	}
	return false;
}

void RgResourceRegistry::createTransientResources(const TArray<RgTransientResourceCreation>& resourcesToCreate)
{
	RgTransientResourceAllocator* allocator = transientAlloctors[gpu::getFrameIndex()];
	allocator->waitForFence();
	for (const RgTransientResourceCreation& info : resourcesToCreate)
	{
		switch (info.type)
		{
		case RgResourceType::BUFFER:
		{
			GpuBuffer* buffer = allocator->createBuffer(info.buffer.bufferSize, info.initialState, info.buffer.initToZero, info.flags);
			setBuffer(info.name, info.resource, buffer, true);
			break;
		}
		case RgResourceType::TEXTURE:
		{	
			GpuTexture* texture = allocator->createTexture(info.texture.width, info.texture.height, info.texture.depth, info.initialState, info.texture.format, info.flags);
			setTexture(info.name, info.resource, texture, true);
			break;
		}
		}
	}
}

TArray<RgTransientResourceCreation> RgResourceRegistry::getPassResourcesToCreate()
{
	TArray<RgTransientResourceCreation> data(std::move(transientResourceToCreate));
	return data;
}

bool RgResourceRegistry::findResourceFromPool(RgResource& outResource)
{
	if (resourcePool.getNum() > 0)
	{
		outResource = resourcePool[resourcePool.getNum() - 1];
		resourcePool.pop();
		return true;
	}
	return false;
}

inline bool operator==(const RgResourceRegistry::ReleaseData& a, const RgResourceRegistry::ReleaseData& b)
{
	return a.resource == b.resource;
}

void RgResourceRegistry::releaseFrameResources(uint32_t frame)
{
	TArray<ReleaseData>& frameResources = frameResourceToRelease[frame];
	for (ReleaseData& data : frameResources)
	{
		GPU_D3D_RELEASE(data.resource);
	}
	frameResources.reset();
}

void RgResourceRegistry::endFrame()
{
	TArray<ReleaseData>& frameResources = frameResourceToRelease[gpu::getFrameIndex()];
	for (RgTransientResourceSlot& slot : transientResources)
	{
		RgResourceEntry* entry = get(slot.resource);
		if (entry->resource)
		{
			frameResources.addUnique({ entry->name, entry->resource->apiResource });
		}
		slot.isUsed = false;
	}
	gpu::addOnPresentCallback([&]()
	{
		transientAlloctors[gpu::getFrameIndex()]->signalFence();
	});
}

void RgResourceRegistry::beginFrame()
{
	transientAlloctors[gpu::getFrameIndex()]->beginFrame();
}

bool RgResourceRegistry::findTransientSlot(RgTransientResourceSlot& outSlot)
{
	for (RgTransientResourceSlot& slot : transientResources)
	{
		if (!slot.isUsed)
		{
			outSlot = slot;
			return true;
		}
	}
	return false;
}

ID3D12DescriptorHeap* RgDescriptorManager::createDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_DESC desc)
{
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	GPU_D3D_ASSERT(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)), "Failed to create descriptor heap");
	return descriptorHeap;
}

RgDescriptorManager::RgDescriptorManager(uint32_t frameCount, uint32_t gpuCapacity, uint32_t rtvAndDsvCapacity) :
	gpuCapacity(gpuCapacity),
	rtvAndDsvCapacity(rtvAndDsvCapacity),
	maxFrames(frameCount)
{
	ID3D12Device* device = gpu::getDevice();
	rtvDescriptorHeap = createDescriptorHeap(device, { .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV, .NumDescriptors = rtvAndDsvCapacity, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, .NodeMask = 0 });
	dsvDescriptorHeap = createDescriptorHeap(device, { .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV, .NumDescriptors = rtvAndDsvCapacity, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, .NodeMask = 0 });
	gpuStagingDescriptorHeap = createDescriptorHeap(device, { .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, .NumDescriptors = gpuCapacity,.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE, .NodeMask = 0 });
	for (uint32_t index = 0; index < maxFrames; ++index)
	{
		gpuDescriptorHeaps.add(createDescriptorHeap(device, {.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, .NumDescriptors = gpuCapacity, .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, .NodeMask = 0}));
	}
	rtvIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	dsvIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	shaderVisibleIncrement = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

RgDescriptorManager::~RgDescriptorManager()
{
	for (uint32_t index = 0; index < maxFrames; ++index) GPU_D3D_RELEASE(gpuDescriptorHeaps[index]);
	GPU_D3D_RELEASE(gpuStagingDescriptorHeap);
	GPU_D3D_RELEASE(dsvDescriptorHeap);
	GPU_D3D_RELEASE(rtvDescriptorHeap);
}

D3D12_CPU_DESCRIPTOR_HANDLE RgDescriptorManager::allocRTV()
{
	DBG_ASSERT(rtvUsed < rtvAndDsvCapacity, "RTV heap exhausted");
	return { .ptr = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + (size_t)(rtvUsed++ * rtvIncrement) };
}

D3D12_CPU_DESCRIPTOR_HANDLE RgDescriptorManager::allocDSV()
{
	DBG_ASSERT(dsvUsed < rtvAndDsvCapacity, "DSV heap exhausted");
	return { .ptr = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + (size_t)(dsvUsed++ * dsvIncrement) };
}

D3D12_CPU_DESCRIPTOR_HANDLE RgDescriptorManager::allocCPU()
{
	DBG_ASSERT(cpuUsed < gpuCapacity, "CPU heap exhausted");
	return { .ptr = gpuStagingDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + (size_t)(cpuUsed++ * shaderVisibleIncrement) };
}

RgDescriptorTable RgDescriptorManager::allocGPU(uint32_t count)
{
	DBG_ASSERT(gpuUsed + count <= gpuCapacity, "GPU heap exhausted");
	RgDescriptorTable desc =
	{
		{ gpuDescriptorHeaps[frame]->GetCPUDescriptorHandleForHeapStart().ptr + (size_t)(gpuUsed * shaderVisibleIncrement)},
		{ gpuDescriptorHeaps[frame]->GetGPUDescriptorHandleForHeapStart().ptr + (uint64_t)(gpuUsed * shaderVisibleIncrement) },
		shaderVisibleIncrement,
		0, count
	};
	gpuUsed += count;
	return desc;
}

void RgDescriptorManager::beginFrame() 
{ 
	gpuUsed = 0; 
	rtvUsed = 0;
	dsvUsed = 0;
	frame = (frame + 1) % maxFrames; 
}

D3D12_CPU_DESCRIPTOR_HANDLE RgDescriptorTable::alloc()
{
	DBG_ASSERT(allocated + 1 <= capacity, "Exceeded capacity of descriptors allocated");
	D3D12_CPU_DESCRIPTOR_HANDLE handle = cpu;
	handle.ptr += ((size_t)allocated * handleSize);
	allocated += 1;
	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE RgDescriptorTable::getCPUDescriptor(uint32_t slot) const
{
	DBG_ASSERT(slot < capacity, "Slot %u out of bounds", slot);
	return D3D12_CPU_DESCRIPTOR_HANDLE{ .ptr = cpu.ptr + (handleSize * slot) };
}

RgCopyPass::RgCopyPass(RgResourceRegistry* registry) :
	registry(registry)
{
}

void RgCopyPass::copy(const String& dest, const String& source)
{
	RgResource destHandle = registry->getHandle(dest);
	RgResource sourceHandle = registry->getHandle(source);
	copy(destHandle, sourceHandle);
}

void RgCopyPass::copy(const RgResource& dest, const RgResource& source)
{
	if (registry->exists(dest) && registry->exists(source))
	{
		copyResources.add({
			.dest = { dest, renderGraphResourceToResourceState(RgResourceAccess::COPY_DST), RgResourceAccess::COPY_DST, true },
			.source = { source, renderGraphResourceToResourceState(RgResourceAccess::COPY_SRC), RgResourceAccess::COPY_SRC, false }
			});
	}
}

RgResource RgCopyPass::createTexture(const String& name, uint32_t width, uint32_t height, uint32_t depth, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags)
{
	return registry->createTransientTexture(name, width, height, depth, initialState, dxgiFormat, flags);
}

RgResource RgCopyPass::createBuffer(const String& name, size_t bufferSize, D3D12_RESOURCE_STATES initialState, bool initToZero, D3D12_RESOURCE_FLAGS flags)
{
	return registry->createTransientBuffer(name, bufferSize, initialState, initToZero, flags);
}

void RgCopyPass::getReadsAndWrites(TArray<RgResource>& reads, TArray<RgResource>& writes)
{
	for (RgCopyPassData& copyData : copyResources)
	{
		writes.add(copyData.dest.resource);
		reads.add(copyData.source.resource);
	}
}

RgCopyPassEntry::RgCopyPassEntry(const String& name, RgResourceRegistry* registry, std::function<void(RgCopyPass&)> setupFn) :
	name(name),
	pass(registry)
{
	setupFn(pass);
	setDesc({ name, nullptr });
}

void RgCopyPassEntry::execute(uint64_t passIndex, RgResourceRegistry* registry, ID3D12GraphicsCommandList* commandList)
{
	if (!pass.shouldExecute) return;
	pix::beginEvent(commandList, passIndex, "Copy Pass: %s", *name);
	GpuResourceBarrierBatcher barriers;
	for (RgCopyPassData& copyData : pass.copyResources)
	{
		RgResourceEntry* dest = registry->get(copyData.dest.resource);
		RgResourceEntry* source = registry->get(copyData.source.resource);
		barriers.transition(*dest->resource, copyData.dest.state);
		if (source->resource->state != D3D12_RESOURCE_STATE_GENERIC_READ)
		{
			barriers.transition(*source->resource, copyData.source.state);
		}
	}
	barriers.flush(commandList);

	for (RgCopyPassData& copyData : pass.copyResources)
	{
		RgResourceEntry* dest = registry->get(copyData.dest.resource);
		RgResourceEntry* source = registry->get(copyData.source.resource);
		commandList->CopyResource(dest->resource->apiResource, source->resource->apiResource);
	}

	pix::endEvent(commandList);
}

void RgCopyPassEntry::getReadsAndWrites(TArray<RgResource>& reads, TArray<RgResource>& writes)
{
	pass.getReadsAndWrites(reads, writes);
}

D3D12_UNORDERED_ACCESS_VIEW_DESC RgView::UAVBuffer(DXGI_FORMAT format, uint64_t firstElement, uint32_t numElements, uint32_t structureByteStride, uint64_t counterOffsetInBytes, bool rawBuffer)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = firstElement;
	uavDesc.Buffer.NumElements = numElements;
	uavDesc.Buffer.StructureByteStride = structureByteStride;
	uavDesc.Buffer.CounterOffsetInBytes = counterOffsetInBytes;
	uavDesc.Buffer.Flags = rawBuffer ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;

	if (rawBuffer) 
	{
		DBG_ASSERT(format == DXGI_FORMAT_R32_TYPELESS && structureByteStride == 0, "RAW UAV requires R32_TYPELESS, stride=0");
		uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	}
	else if (structureByteStride) 
	{
		DBG_ASSERT(format == DXGI_FORMAT_UNKNOWN, "Structured UAV must use UNKNOWN format");
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
		// counter valid only for structured
	}
	else 
	{
		DBG_ASSERT(format != DXGI_FORMAT_UNKNOWN, "Typed UAV must specify a typed DXGI_FORMAT");
		uavDesc.Format = format;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
	}
	return uavDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC RgView::UAVTex1D(DXGI_FORMAT format, uint32_t mipSlice)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
	uavDesc.Texture1D.MipSlice = mipSlice;
	return uavDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC RgView::UAVTex1DArray(DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
	uavDesc.Texture1DArray.MipSlice = mipSlice;
	uavDesc.Texture1DArray.FirstArraySlice = firstArraySlice;
	uavDesc.Texture1DArray.ArraySize = arraySize;
	return uavDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC RgView::UAVTex2D(DXGI_FORMAT format, uint32_t mipSlice, uint32_t planeSlice)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = mipSlice;
	uavDesc.Texture2D.PlaneSlice = planeSlice;
	return uavDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC RgView::UAVTex2DArray(DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstArraySlice, uint32_t arraySize, uint32_t planeSlice)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	uavDesc.Texture2DArray.MipSlice = mipSlice;
	uavDesc.Texture2DArray.PlaneSlice = planeSlice;
	uavDesc.Texture2DArray.FirstArraySlice = firstArraySlice;
	uavDesc.Texture2DArray.ArraySize = arraySize;
	return uavDesc;
}

D3D12_UNORDERED_ACCESS_VIEW_DESC RgView::UAVTex3D(DXGI_FORMAT format, uint32_t mipSlice, uint32_t firstWSlice, uint32_t wSize)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
	uavDesc.Texture3D.MipSlice = mipSlice;
	uavDesc.Texture3D.FirstWSlice = firstWSlice;
	uavDesc.Texture3D.WSize = wSize;
	return uavDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC RgView::SRVBuffer(DXGI_FORMAT format, uint64_t firstElement, uint32_t numElements, uint32_t structureByteStride, bool rawBuffer)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = firstElement;
	srvDesc.Buffer.NumElements = numElements;
	srvDesc.Buffer.StructureByteStride = structureByteStride;
	srvDesc.Buffer.Flags = rawBuffer ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;

	if (rawBuffer) 
	{
		DBG_ASSERT(format == DXGI_FORMAT_R32_TYPELESS && structureByteStride == 0, "RAW SRV requires R32_TYPELESS, stride=0");
		srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	}
	else if (structureByteStride) 
	{
		DBG_ASSERT(format == DXGI_FORMAT_UNKNOWN, "Structured SRV must use UNKNOWN format");
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	}
	else 
	{
		DBG_ASSERT(format != DXGI_FORMAT_UNKNOWN, "Typed SRV must specify a typed DXGI_FORMAT");
		srvDesc.Format = format;
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	}
	return srvDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC RgView::SRVTex1D(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture1D.MostDetailedMip = mostDetailedMip;
	srvDesc.Texture1D.MipLevels = mipLevels;
	srvDesc.Texture1D.ResourceMinLODClamp = resourceMinLODClamp;
	return srvDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC RgView::SRVTex1DArray(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t firstArraySlice, uint32_t arraySize, float resourceMinLODClamp)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture1DArray.MostDetailedMip = mostDetailedMip;
	srvDesc.Texture1DArray.MipLevels = mipLevels;
	srvDesc.Texture1DArray.FirstArraySlice = firstArraySlice;
	srvDesc.Texture1DArray.ArraySize = arraySize;
	srvDesc.Texture1DArray.ResourceMinLODClamp = resourceMinLODClamp;
	return srvDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC RgView::SRVTex2D(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t planeSlice, float resourceMinLODClamp)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;
	srvDesc.Texture2D.MipLevels = mipLevels;
	srvDesc.Texture2D.PlaneSlice = planeSlice;
	srvDesc.Texture2D.ResourceMinLODClamp = resourceMinLODClamp;
	return srvDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC RgView::SRVTex2DArray(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, uint32_t firstArraySlice, uint32_t arraySize, uint32_t planeSlice, float resourceMinLODClamp)
{
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
	return srvDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC RgView::SRVTex3D(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture3D.MostDetailedMip = mostDetailedMip;
	srvDesc.Texture3D.MipLevels = mipLevels;
	srvDesc.Texture3D.ResourceMinLODClamp = resourceMinLODClamp;
	return srvDesc;
}

D3D12_SHADER_RESOURCE_VIEW_DESC RgView::SRVTexCube(DXGI_FORMAT format, uint32_t mostDetailedMip, uint32_t mipLevels, float resourceMinLODClamp)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.TextureCube.MostDetailedMip = mostDetailedMip;
	srvDesc.TextureCube.MipLevels = mipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = resourceMinLODClamp;
	return srvDesc;
}

D3D12_RENDER_TARGET_VIEW_DESC RgView::RTVTex2D(DXGI_FORMAT format, uint32_t mipSlice, uint32_t planeSlice)
{
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	rtvDesc.Format = format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtvDesc.Texture2D.MipSlice = mipSlice;
	rtvDesc.Texture2D.PlaneSlice = planeSlice;
	return rtvDesc;
}

D3D12_DEPTH_STENCIL_VIEW_DESC RgView::DSVTex2D(DXGI_FORMAT format, uint32_t mipSlice, D3D12_DSV_FLAGS flags)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = format;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = flags;
	dsvDesc.Texture2D.MipSlice = mipSlice;
	return dsvDesc;
}

D3D12_CONSTANT_BUFFER_VIEW_DESC RgView::CBVBuffer(size_t sizeInBytes)
{
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = 0;
	cbvDesc.SizeInBytes = UINT((sizeInBytes + 255) & ~255u); // align up to 256
	DBG_ASSERT(cbvDesc.SizeInBytes <= 65536, "CBV view must be <= 64KB");
	return cbvDesc;
}

D3D12_VERTEX_BUFFER_VIEW RgView::VBVBuffer(size_t sizeInBytes, size_t strideInBytes)
{
	return D3D12_VERTEX_BUFFER_VIEW();
}

D3D12_INDEX_BUFFER_VIEW RgView::IBVBuffer(DXGI_FORMAT format, size_t sizeInBytes)
{
	return D3D12_INDEX_BUFFER_VIEW();
}

void RgPassComputeContext::bindUAV(const RgDescriptorTable& descriptorTable, uint32_t slot, const RgResource& resource, const RgResource* counter, D3D12_UNORDERED_ACCESS_VIEW_DESC desc)
{
	if (RgResourceEntry* entry = reg->get(resource))
	{
		gpu::getDevice()->CreateUnorderedAccessView(
			entry->resource->apiResource,
			counter ? reg->get(*counter)->resource->apiResource : nullptr,
			&desc,
			descriptorTable.getCPUDescriptor(slot)
		);
	}
}

void RgPassComputeContext::bindSRV(const RgDescriptorTable& descriptorTable, uint32_t slot, const RgResource& resource, D3D12_SHADER_RESOURCE_VIEW_DESC desc)
{
	if (RgResourceEntry* entry = reg->get(resource))
	{
		gpu::getDevice()->CreateShaderResourceView(
			entry->resource->apiResource,
			&desc,
			descriptorTable.getCPUDescriptor(slot)
		);

	}
}

void RgPassComputeContext::bindCBV(const RgDescriptorTable& descriptorTable, uint32_t slot, const RgResource& resource, D3D12_CONSTANT_BUFFER_VIEW_DESC desc)
{
	if (RgResourceEntry* entry = reg->get(resource))
	{
		desc.BufferLocation = entry->resource->apiResource->GetGPUVirtualAddress();
		desc.SizeInBytes = (UINT)entry->buffer->getSizeInBytes();
		gpu::getDevice()->CreateConstantBufferView(&desc, descriptorTable.getCPUDescriptor(slot));
	}
}

void RgPassComputeContext::set32BitConstants(uint32_t rootParameter, uint32_t numValues, const void* data, uint32_t destOffset)
{
	cmd->SetComputeRoot32BitConstants(rootParameter, numValues, data, destOffset);
}

void RgPassComputeContext::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
	cmd->Dispatch(x, y, z);
}

size_t RgPassComputeContext::getBufferSizeInBytes(const RgResource& resource)
{
	if (RgResourceEntry* entry = reg->get(resource))
	{
		if (entry->buffer)
		{
			return entry->buffer->getSizeInBytes();
		}
	}
	return 0;
}

RgResource RgPassComputeContext::setTexture(const String& name, const RgResource& resource, GpuTexture* texture)
{
	return reg->setTexture(name, resource, texture);
}
RgResource RgPassComputeContext::setBuffer(const String& name, const RgResource& resource, GpuBuffer* buffer)
{
	return reg->setBuffer(name, resource, buffer);
}

void RgPassRasterContext::setScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
	D3D12_RECT scissor = { left, top, right, bottom };
	cmd->RSSetScissorRects(1, &scissor);
}

void RgPassRasterContext::setViewport(float topLeftX, float topLeftY, float width, float height, float minDepth, float maxDepth)
{
	D3D12_VIEWPORT viewport = { topLeftX, topLeftY, width, height, minDepth, maxDepth };
	cmd->RSSetViewports(1, &viewport);
}

void RgPassRasterContext::setPrimitiveTopology(RgPrimitiveTopology topology)
{
	D3D12_PRIMITIVE_TOPOLOGY topo = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	switch (topology)
	{
	case RgPrimitiveTopology::TRIANGLE_LIST:
		topo = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		break;
	}
	cmd->IASetPrimitiveTopology(topo);
}

void RgPassRasterContext::drawInstanced(uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation)
{
	cmd->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void RgPassRasterContext::drawIndexedInstanced(uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation)
{
	cmd->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void RgPassRasterContext::setVertexBuffer(const RgResource& vertexBuffer, uint32_t slot, size_t sizeInBytes, size_t strideInBytes)
{
	if (RgResourceEntry* entry = reg->get(vertexBuffer))
	{
		DBG_ASSERT(entry->resource != nullptr, "Resource is NULL");
		D3D12_VERTEX_BUFFER_VIEW vbv = {};
		vbv.BufferLocation = entry->resource->apiResource->GetGPUVirtualAddress();
		vbv.SizeInBytes = (UINT)sizeInBytes;
		vbv.StrideInBytes = (UINT)strideInBytes;
		cmd->IASetVertexBuffers(slot, 1, &vbv);
	}
	else
	{
		DBG_PANIC("Invalid vertex buffer");
	}
}

void RgPassRasterContext::setVertexBuffer(const RgResource& vertexBuffer, uint32_t slot, size_t strideInBytes)
{
	if (RgResourceEntry* entry = reg->get(vertexBuffer))
	{
		DBG_ASSERT(entry->resource != nullptr, "Resource is NULL");
		D3D12_VERTEX_BUFFER_VIEW vbv = {};
		vbv.BufferLocation = entry->resource->apiResource->GetGPUVirtualAddress();
		vbv.SizeInBytes = (UINT)entry->buffer->getSizeInBytes();
		vbv.StrideInBytes = (UINT)strideInBytes;
		cmd->IASetVertexBuffers(slot, 1, &vbv);
	}
	else
	{
		DBG_PANIC("Invalid vertex buffer");
	}
}

void RgPassRasterContext::setIndexBuffer(const RgResource& indexBuffer, size_t sizeInBytes, DXGI_FORMAT indexFormat)
{
	if (RgResourceEntry* entry = reg->get(indexBuffer))
	{
		DBG_ASSERT(entry->resource != nullptr, "Resource is NULL");
		D3D12_INDEX_BUFFER_VIEW ibv = {};
		ibv.BufferLocation = entry->resource->apiResource->GetGPUVirtualAddress();
		ibv.SizeInBytes = (UINT)sizeInBytes;
		ibv.Format = indexFormat;
		cmd->IASetIndexBuffer(&ibv);
	}
	else
	{
		DBG_PANIC("Invalid index buffer");
	}
}

void RgPassRasterContext::setRenderTargets(const TArray<RgRenderTarget>& renderTargets, RgDepthStencilTarget* depthStencilTarget, RgClearFlag clearFlag)
{
	TArray< D3D12_CPU_DESCRIPTOR_HANDLE> rtvDescriptors;
	for (const RgRenderTarget& rt : renderTargets)
	{
		if (RgResourceEntry* entry = reg->get(rt.resource))
		{
			ID3D12Device* dev = gpu::getDevice();
			ID3D12DescriptorHeap* heap = descMgr->rtvDescriptorHeap;
			ID3D12Resource* res = entry->resource->apiResource;
			uint32_t idx = descMgr->rtvUsed;

			// Validate objects
			DBG_ASSERT(dev , "device is null");
			DBG_ASSERT(heap , "rtv heap is null");
			DBG_ASSERT(res , "resource is null");

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = heap->GetDesc();
			DBG_ASSERT(heapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_RTV , "heap type must be RTV");
			DBG_ASSERT((heapDesc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) == 0 , "RTV heap cannot be shader-visible");
			DBG_ASSERT(idx < heapDesc.NumDescriptors , "rtv index out of range");

			// Compute handle
			const UINT inc = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			DBG_ASSERT(inc > 0 , "RTV increment size is zero?");
			D3D12_CPU_DESCRIPTOR_HANDLE base = heap->GetCPUDescriptorHandleForHeapStart();
			DBG_ASSERT(base.ptr != 0 , "RTV heap CPU start is null");
			D3D12_CPU_DESCRIPTOR_HANDLE handle{ base.ptr + SIZE_T(idx) * inc };
			DBG_ASSERT(handle.ptr != 0 , "Computed RTV handle is null");

			// Optional: sanity for desc if provided
			{
				DBG_ASSERT(rt.desc.Format != DXGI_FORMAT_UNKNOWN, "RTV format must be concrete (non-typeless)");
				DBG_ASSERT(rt.desc.ViewDimension != D3D12_RTV_DIMENSION_UNKNOWN, "");
			}

			DBG_ASSERT(entry->resource != nullptr, "Resource is NULL");
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = descMgr->allocRTV();

			gpu::getDevice()->CreateRenderTargetView(entry->resource->apiResource, &rt.desc, rtvHandle);
			rtvDescriptors.add(rtvHandle);
		}
	}
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = { 0 };
	D3D12_CPU_DESCRIPTOR_HANDLE* dsvHandlePtr = nullptr;
	if (depthStencilTarget)
	{
		if (RgResourceEntry* entry = reg->get(depthStencilTarget->resource))
		{
			DBG_ASSERT(entry->resource != nullptr, "Resource is NULL");
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = descMgr->allocDSV();
			gpu::getDevice()->CreateDepthStencilView(entry->resource->apiResource, &depthStencilTarget->desc, dsvHandle);
			dsvHandlePtr = &dsvHandle;
		}
	}
	cmd->OMSetRenderTargets((UINT)rtvDescriptors.getNum(), *rtvDescriptors, true, dsvHandlePtr);

	if (renderGraphClearHasFlag(clearFlag, RgClearFlag::CLEAR_COLOR))
	{
		for (D3D12_CPU_DESCRIPTOR_HANDLE rtv : rtvDescriptors)
		{
			cmd->ClearRenderTargetView(rtv, rtClearColor.components, 0, nullptr);
		}
	}
	if (dsvHandlePtr)
	{
		if (renderGraphClearHasFlag(clearFlag, RgClearFlag::CLEAR_DEPTH_STENCIL))
		{
			cmd->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, dsClearDepth, dsClearStencil, 0, nullptr);
		}
		else if (renderGraphClearHasFlag(clearFlag, RgClearFlag::CLEAR_DEPTH))
		{
			cmd->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, dsClearDepth, dsClearStencil, 0, nullptr);
		}
		else if (renderGraphClearHasFlag(clearFlag, RgClearFlag::CLEAR_STENCIL))
		{
			cmd->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_STENCIL, dsClearDepth, dsClearStencil, 0, nullptr);
		}
	}
}

void RgPassRasterContext::setRenderTarget(const RgRenderTarget& renderTarget, RgDepthStencilTarget* depthStencilTarget, RgClearFlag clearFlag)
{
	setRenderTargets({ renderTarget }, depthStencilTarget, clearFlag);
}

void RgPassRasterContext::set32BitConstants(uint32_t rootParameter, uint32_t numValues, const void* data, uint32_t destOffset)
{
	cmd->SetGraphicsRoot32BitConstants(rootParameter, numValues, data, destOffset);
}

RgTransientResourceAllocator::RgTransientResourceAllocator(size_t sizeInBytes, D3D12_HEAP_TYPE heapType) :
	objAllocator(MEM_MiB(2))
{
	D3D12_HEAP_DESC desc = {};
	desc.Alignment = 0;
	desc.SizeInBytes = sizeInBytes;
	desc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
	desc.Properties.Type = heapType;
	desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	desc.Properties.CreationNodeMask = 0;
	desc.Properties.VisibleNodeMask = 0;
	GPU_D3D_ASSERT(gpu::getDevice()->CreateHeap(&desc, IID_PPV_ARGS(&heap)), "Failed to create heap for transient resources");
	capacity = sizeInBytes;
	GPU_D3D_ASSERT(gpu::getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "Failed to create transient resource allocator fence");
	fenceEvent = CreateEventA(nullptr, false, false, nullptr);
	heap->SetName(L"RgTransientResourceAllocator::heap");
}

RgTransientResourceAllocator::~RgTransientResourceAllocator()
{
	waitForFence();
	GPU_D3D_RELEASE(heap);
	GPU_D3D_RELEASE(fence);
	CloseHandle(fenceEvent);
}

void RgTransientResourceAllocator::beginFrame()
{
	waitForFence();
	objAllocator.reset();
	offset = 0;
	InterlockedExchange64(&offset, 0);
}

ID3D12Resource* RgTransientResourceAllocator::createResource(const D3D12_RESOURCE_DESC& desc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue)
{
	D3D12_RESOURCE_ALLOCATION_INFO allocInfo = gpu::getDevice()->GetResourceAllocationInfo(0, 1, &desc);
	offset = mem::alignSize(offset, allocInfo.Alignment);
	DBG_ASSERT(offset + (int64_t)allocInfo.SizeInBytes <= capacity, "Exhausted transient resource heap");
	ID3D12Resource* resource = nullptr;
	GPU_D3D_ASSERT(gpu::getDevice()->CreatePlacedResource(
		heap,
		offset,
		&desc,
		initialState,
		clearValue,
		IID_PPV_ARGS(&resource)), "Failed to create transient resource");
	InterlockedAdd64(&offset, (LONG64)allocInfo.SizeInBytes);
	return resource;
}

GpuTexture* RgTransientResourceAllocator::createTexture(uint32_t width, uint32_t height, uint32_t depth, D3D12_RESOURCE_STATES initialState, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS flags)
{
	GpuTexture* texture = new(objAllocator.allocate(sizeof(GpuTexture))) GpuTexture();
	D3D12_RESOURCE_DIMENSION dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	if (width > 1 && height > 1 && depth > 1) 
	{
		dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	}
	else if (width >= 1 && height >= 1 && depth == 1) 
	{
		dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	}
	else if (width >= 1 && height == 1 && depth == 1) 
	{
		dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	}
	D3D12_RESOURCE_DESC resourceDesc = 
	{
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

	texture->resource.apiResource = createResource(resourceDesc, initialState, clearValuePtr);
	texture->resource.state = initialState;
	texture->width = width;
	texture->height = height;
	texture->depth = depth;

	return texture;
}

GpuBuffer* RgTransientResourceAllocator::createBuffer(size_t bufferSize, D3D12_RESOURCE_STATES initialState, bool initToZero, D3D12_RESOURCE_FLAGS flags)
{
	GpuBuffer* buffer = new(objAllocator.allocate(sizeof(GpuBuffer))) GpuBuffer();
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Width = mem::alignSize(bufferSize, 256);
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc = { 1, 0 };
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = flags;
	buffer->resource.apiResource = createResource(desc, initialState, nullptr);
	buffer->resource.state = initialState;
	buffer->sizeInBytes = desc.Width;
	buffer->cpuData = nullptr;
	buffer->upload.apiResource = nullptr;
	return buffer;
}

void RgTransientResourceAllocator::waitForFence()
{
	if (fence->GetCompletedValue() < fenceValue)
	{
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
	}
}

void RgTransientResourceAllocator::signalFence()
{
	GPU_D3D_ASSERT(gpu::getCommandQueue()->Signal(fence, ++fenceValue), "Failed to signal transient resource allocator fence");
}
