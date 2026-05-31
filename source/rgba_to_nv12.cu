#include "rgba_to_nv12.h"

#include <cuda_runtime.h>

namespace
{

__device__ __forceinline__ unsigned char clampToByte(int value)
{
	return static_cast<unsigned char>(min(max(value, 0), 255));
}

__device__ __forceinline__ unsigned char rgbToY(const uchar4 rgba)
{
	const int r = static_cast<int>(rgba.x);
	const int g = static_cast<int>(rgba.y);
	const int b = static_cast<int>(rgba.z);

	// BT.709 limited-range luma, suitable for HD video encoders expecting NV12.
	return clampToByte((47 * r + 157 * g + 16 * b + 4096) >> 8);
}

__device__ __forceinline__ int rgbToUAccum(const uchar4 rgba)
{
	const int r = static_cast<int>(rgba.x);
	const int g = static_cast<int>(rgba.y);
	const int b = static_cast<int>(rgba.z);

	return -26 * r - 87 * g + 112 * b;
}

__device__ __forceinline__ int rgbToVAccum(const uchar4 rgba)
{
	const int r = static_cast<int>(rgba.x);
	const int g = static_cast<int>(rgba.y);
	const int b = static_cast<int>(rgba.z);

	return 112 * r - 102 * g - 10 * b;
}

__global__ void rgba8ToNV12Kernel(cudaSurfaceObject_t rgba_surface, unsigned char *nv12,
	size_t pitch, int width, int height)
{
	const int block_x = blockIdx.x * blockDim.x + threadIdx.x;
	const int block_y = blockIdx.y * blockDim.y + threadIdx.y;
	const int x = block_x * 2;
	const int y = block_y * 2;

	if (x >= width || y >= height)
		return;

	const uchar4 p00 = surf2Dread<uchar4>(rgba_surface, x * static_cast<int>(sizeof(uchar4)), y);
	const uchar4 p10 = surf2Dread<uchar4>(rgba_surface, (x + 1) * static_cast<int>(sizeof(uchar4)), y);
	const uchar4 p01 = surf2Dread<uchar4>(rgba_surface, x * static_cast<int>(sizeof(uchar4)), y + 1);
	const uchar4 p11 = surf2Dread<uchar4>(rgba_surface, (x + 1) * static_cast<int>(sizeof(uchar4)), y + 1);

	unsigned char *y0 = nv12 + y * pitch + x;
	unsigned char *y1 = y0 + pitch;

	y0[0] = rgbToY(p00);
	y0[1] = rgbToY(p10);
	y1[0] = rgbToY(p01);
	y1[1] = rgbToY(p11);

	const int u_accum = rgbToUAccum(p00) + rgbToUAccum(p10) + rgbToUAccum(p01) + rgbToUAccum(p11);
	const int v_accum = rgbToVAccum(p00) + rgbToVAccum(p10) + rgbToVAccum(p01) + rgbToVAccum(p11);

	unsigned char *uv = nv12 + height * pitch + block_y * pitch + x;
	uv[0] = clampToByte(((u_accum + 512) >> 10) + 128);
	uv[1] = clampToByte(((v_accum + 512) >> 10) + 128);
}

} // namespace

cudaError_t rgba8SurfaceToNV12(cudaSurfaceObject_t rgba_surface, std::uint8_t *nv12, std::size_t pitch,
	int width, int height, cudaStream_t stream)
{
	if (rgba_surface == 0 || nv12 == nullptr || width <= 0 || height <= 0)
		return cudaErrorInvalidValue;
	if ((width & 1) != 0 || (height & 1) != 0)
		return cudaErrorInvalidValue;
	if (pitch < static_cast<std::size_t>(width))
		return cudaErrorInvalidPitchValue;

	const dim3 block(16, 16);
	const dim3 grid((width / 2 + block.x - 1) / block.x, (height / 2 + block.y - 1) / block.y);
	rgba8ToNV12Kernel<<<grid, block, 0, stream>>>(rgba_surface, nv12, pitch, width, height);
	return cudaGetLastError();
}
