#pragma once

#include <stdint.h>
#include <d3d12.h>

#if _DEBUG
#define PIX_ENABLE_DEBUG_API 0
#else
#define PIX_ENABLE_DEBUG_API 0
#endif

namespace pix
{
	bool loadPIX();
	void beginEvent(ID3D12GraphicsCommandList* ctx, uint64_t color, const char* fmt, ...);
	void endEvent(ID3D12GraphicsCommandList* ctx);
	void beginEvent(ID3D12CommandQueue* ctx, uint64_t color, const char* fmt, ...);
	void endEvent(ID3D12CommandQueue* ctx);
	bool beginCapture(const char* name);
	bool endCapture(bool openCapture = false);
	bool isPixEnabled();
}
