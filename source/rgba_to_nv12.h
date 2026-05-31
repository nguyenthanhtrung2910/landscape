#pragma once

#include <cuda_runtime.h>

#include <cstddef>
#include <cstdint>

// Converts an RGBA8 CUDA surface to a pitched NV12 device buffer.
// nv12 layout:
//   Y  plane: height rows, width bytes per row, pitch bytes between rows
//   UV plane: height / 2 rows, width bytes per row, pitch bytes between rows
//
// Width and height must be even. The source surface must contain packed RGBA8 texels.
cudaError_t rgba8SurfaceToNV12(cudaSurfaceObject_t rgba_surface, std::uint8_t *nv12, std::size_t pitch,
	int width, int height, cudaStream_t stream = nullptr);
