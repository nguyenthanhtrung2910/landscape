#pragma once

#include <cuda_runtime.h>

#include <UnigineResourceExternalMemory.h>
#include <UnigineResourceFence.h>

#ifdef __GNUC__
	#define __UNIGINE_FUNCTION__ __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
	#define __UNIGINE_FUNCTION__ __FUNCTION__
#endif

#ifndef ck
	#define ck(x) \
	{ \
		cudaError_t res = x; \
		if (res != cudaSuccess) \
			Log::fatal("%s: %s Failed with error '%s' (%d)", __UNIGINE_FUNCTION__, #x, cudaGetErrorString(res), res); \
	}
#endif

void importResourceExternalMemory(Unigine::ResourceExternalMemoryPtr resource_external_memory, cudaExternalMemory_t *external_memory);
void importFenceExternal(Unigine::ResourceFencePtr fence, cudaExternalSemaphore_t *external_semaphore);
