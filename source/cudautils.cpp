#include "cudautils.h"

#include <UnigineSystemInfo.h>
#include <UnigineRender.h>

using namespace Unigine;

void importResourceExternalMemory(Unigine::ResourceExternalMemoryPtr resource_external_memory, cudaExternalMemory_t *external_memory)
{
	cudaExternalMemoryHandleDesc desc{};
	switch (Render::getAPI())
	{
		case Render::API_DIRECT3D12: desc.type = cudaExternalMemoryHandleTypeD3D12Resource; break;
		case Render::API_VULKAN:
			#ifdef _WIN32
				desc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
			#else
				desc.type = cudaExternalMemoryHandleTypeOpaqueFd;
			#endif
			break;
	}

	#ifdef _WIN32
		desc.handle.win32.handle = resource_external_memory->getWin32Handle();
	#else
		desc.handle.fd = resource_external_memory->getFdHandle();
	#endif

	desc.size = resource_external_memory->getSize();

	// Required since all Graphics APIs internally allocate dedicated memory for shared resources
	desc.flags = cudaExternalMemoryDedicated;

	ck(cudaImportExternalMemory(external_memory, &desc));
}

void importFenceExternal(Unigine::ResourceFencePtr fence, cudaExternalSemaphore_t *external_semaphore)
{
	cudaExternalSemaphoreHandleDesc desc{};
	#ifdef _WIN32
		switch (Render::getAPI())
		{
			case Render::API_DIRECT3D12: desc.type = cudaExternalSemaphoreHandleTypeD3D12Fence; break;
			case Render::API_VULKAN: desc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreWin32; break;
		}

		desc.handle.win32.handle = fence->getWin32Handle();
	#else
		desc.type = cudaExternalSemaphoreHandleTypeTimelineSemaphoreFd;
		desc.handle.fd = fence->getFdHandle();
	#endif

	ck(cudaImportExternalSemaphore(external_semaphore, &desc));
}
