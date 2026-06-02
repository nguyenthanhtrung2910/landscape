#include "cudatexture.h"

#include <UnigineRender.h>

#include "cudautils.h"

using namespace Unigine;

bool CUDATexture::exportToCuda(const TexturePtr &src)
{
	clear();
	if ((src->getFormatFlags() & Texture::FORMAT_USAGE_SHARED) == 0)
		return false;
	texture = src;
	// nullptr is returned if no SHARED flag
	ResourceExternalMemoryPtr resource_external_memory{texture->getResourceExternalMemory()};
	if (resource_external_memory == nullptr)
		return false;
	// eternal memory
	importResourceExternalMemory(resource_external_memory, &external_memory);
	// mipmap
	cudaExternalMemoryMipmappedArrayDesc mip_mapped_array_desc{};
	mip_mapped_array_desc.extent = make_cudaExtent(texture->getWidth(), texture->getHeight(), 0);
	switch (texture->getType())
	{
	case Texture::TEXTURE_2D_ARRAY:
		mip_mapped_array_desc.extent.depth = texture->getNumLayers();
		mip_mapped_array_desc.flags |= cudaArrayLayered;
		break;
	case Texture::TEXTURE_CUBE:
		mip_mapped_array_desc.extent.depth = texture->getNumFaces();
		mip_mapped_array_desc.flags |= cudaArrayCubemap;
		break;
	case Texture::TEXTURE_CUBE_ARRAY:
		mip_mapped_array_desc.extent.depth = texture->getNumLayers() * texture->getNumFaces();
		mip_mapped_array_desc.flags |= cudaArrayLayered | cudaArrayCubemap;
		break;
	case Texture::TEXTURE_3D:
		mip_mapped_array_desc.extent.depth = texture->getDepth();
		break;
	}
	mip_mapped_array_desc.formatDesc = convert_texture_format();
	mip_mapped_array_desc.numLevels = texture->getNumMipmaps();
	mip_mapped_array_desc.flags = cudaArraySurfaceLoadStore; // We need ask for RW access for CUDA kernels
	if (texture->getFormatFlags() & Texture::FORMAT_USAGE_RENDER)
		mip_mapped_array_desc.flags |= cudaArrayColorAttachment;
	switch (texture->getType())
	{
	case Texture::TEXTURE_2D_ARRAY:
		mip_mapped_array_desc.flags |= cudaArrayLayered;
		break;
	case Texture::TEXTURE_CUBE:
		mip_mapped_array_desc.flags |= cudaArrayCubemap;
		break;
	case Texture::TEXTURE_CUBE_ARRAY:
		mip_mapped_array_desc.flags |= cudaArrayLayered | cudaArrayCubemap;
		break;
	}
	ck(cudaExternalMemoryGetMappedMipmappedArray(&mip_array, external_memory, &mip_mapped_array_desc));
	// surface mip0
	cudaArray_t cu_array{};
	ck(cudaGetMipmappedArrayLevel(&cu_array, mip_array, 0));
	cudaResourceDesc resource_desc{};
	resource_desc.resType = cudaResourceTypeArray;
	resource_desc.res.array.array = cu_array;
	ck(cudaCreateSurfaceObject(&surface, &resource_desc));

	resource_external_memory->closeHandle();
	initialized = true;
	return true;
}

void CUDATexture::clear()
{
	if (initialized == false)
		return;
	cudaDestroySurfaceObject(surface);
	cudaFreeMipmappedArray(mip_array);
	cudaDestroyExternalMemory(external_memory);
	texture = nullptr;
	initialized = false;
}

cudaArray_t CUDATexture::getMipLevel(int mip) const
{
	mip = Math::clamp(mip, 0, texture->getNumMipmaps());
	cudaArray_t cuda_array;
	cudaGetMipmappedArrayLevel(&cuda_array, mip_array, mip);
	return cuda_array;
}

cudaChannelFormatDesc CUDATexture::convert_texture_format() const
{
	switch (texture->getFormat())
	{
	case Texture::FORMAT_R8:
		return {8, 0, 0, 0, cudaChannelFormatKindUnsigned};
	case Texture::FORMAT_RG8:
		return {8, 8, 0, 0, cudaChannelFormatKindUnsigned};
	case Texture::FORMAT_RGBA8:
		return {8, 8, 8, 8, cudaChannelFormatKindUnsigned};

	case Texture::FORMAT_R16:
		return {16, 0, 0, 0, cudaChannelFormatKindSignedNormalized8X1};
	case Texture::FORMAT_RG16:
		return {16, 16, 0, 0, cudaChannelFormatKindSignedNormalized8X2};
	case Texture::FORMAT_RGBA16:
		return {16, 16, 16, 16, cudaChannelFormatKindSignedNormalized8X4};

	case Texture::FORMAT_R16U:
		return {16, 0, 0, 0, cudaChannelFormatKindUnsignedNormalized8X1};
	case Texture::FORMAT_RG16U:
		return {16, 16, 0, 0, cudaChannelFormatKindUnsignedNormalized8X2};
	case Texture::FORMAT_RGBA16U:
		return {16, 16, 16, 16, cudaChannelFormatKindUnsignedNormalized8X4};

	case Texture::FORMAT_R32U:
		return {32, 0, 0, 0, cudaChannelFormatKindUnsignedNormalized8X1};
	case Texture::FORMAT_RG32U:
		return {32, 32, 0, 0, cudaChannelFormatKindUnsignedNormalized8X2};
	case Texture::FORMAT_RGBA32U:
		return {32, 32, 32, 32, cudaChannelFormatKindUnsignedNormalized8X4};

	case Texture::FORMAT_R16F:
		return {16, 0, 0, 0, cudaChannelFormatKindFloat};
	case Texture::FORMAT_RG16F:
		return {16, 16, 0, 0, cudaChannelFormatKindFloat};
	case Texture::FORMAT_RGBA16F:
		return {16, 16, 16, 16, cudaChannelFormatKindFloat};

	case Texture::FORMAT_R32F:
		return {32, 0, 0, 0, cudaChannelFormatKindFloat};
	case Texture::FORMAT_RG32F:
		return {32, 32, 0, 0, cudaChannelFormatKindFloat};
	case Texture::FORMAT_RGBA32F:
		return {32, 32, 32, 32, cudaChannelFormatKindFloat};

	case Texture::FORMAT_RGB565:
		return {5, 6, 5, 0, cudaChannelFormatKindUnsigned};
	case Texture::FORMAT_RGBA4:
		return {4, 4, 4, 4, cudaChannelFormatKindUnsigned};
	case Texture::FORMAT_RGB5A1:
		return {5, 5, 5, 1, cudaChannelFormatKindUnsigned};
	case Texture::FORMAT_RGB10A2:
		return {10, 10, 10, 2, cudaChannelFormatKindUnsigned};
	case Texture::FORMAT_RG11B10F:
		return {11, 11, 10, 0, cudaChannelFormatKindFloat};
	}

	return {};
}
