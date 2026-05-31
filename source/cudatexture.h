#pragma once

#include <cuda_runtime.h>

#include <UnigineTextures.h>
#include <UnigineResourceFence.h>
#include <UnigineResourceExternalMemory.h>
#include <UnigineEvent.h>

class CUDATexture
{
public:
	CUDATexture()
	{
	}
	~CUDATexture()
	{
		clear();
	}

	bool exportToCuda(const Unigine::TexturePtr &src);
	void clear();

	cudaSurfaceObject_t getSurface() const { return surface; }
	cudaExternalMemory_t getExternalMemory() const { return external_memory; }
	cudaMipmappedArray_t getMipArray() const { return mip_array; }
	cudaArray_t getMipLevel(int mip) const;
	const Unigine::TexturePtr &getTexture() const { return texture; }
	bool isInitialized() const { return initialized; }
	int getWidth() const { return texture->getWidth(); }
	int getHeight() const { return texture->getHeight(); }

private:
	cudaChannelFormatDesc convert_texture_format() const;

private:
	bool initialized{};
	Unigine::TexturePtr texture;
	cudaExternalMemory_t external_memory{};
	cudaSurfaceObject_t surface{};
	cudaMipmappedArray_t mip_array{};
};
