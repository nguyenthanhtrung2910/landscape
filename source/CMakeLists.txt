##==============================================================================
## General.
##==============================================================================
cmake_minimum_required(VERSION 4.0)
if(NOT CMAKE_HOST_WIN32)
    message(FATAL_ERROR "landscape is configured for Windows only")
endif()
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/UnigineCMakeVariables.cmake)
project(landscape LANGUAGES C CXX CUDA)
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "landscape requires the MSVC C++ toolchain")
endif()

##==============================================================================
## Global cached default variables.
##==============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED TRUE)
set(CMAKE_CUDA_EXTENSIONS FALSE)

##==============================================================================
## Dependencies.
##==============================================================================
# Engine.
set(UNIGINE_DOUBLE 0)
get_filename_component(UNIGINE_SDK_PATH "${PROJECT_SOURCE_DIR}/.." ABSOLUTE)
set(UNIGINE_BIN_DIR "${UNIGINE_SDK_PATH}/bin")
set(UNIGINE_LIB_DIR "${UNIGINE_SDK_PATH}/lib")
set(UNIGINE_INCLUDE_DIR "${UNIGINE_SDK_PATH}/include")
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
find_package(Engine REQUIRED MODULE QUIET)
# Cuda
find_package(CUDAToolkit REQUIRED)
# FFmpeg's Windows distribution provides MSVC import libraries directly.
set(FFMPEG_ROOT "" CACHE PATH "Root of the FFmpeg shared build")
find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h
    PATHS "${FFMPEG_ROOT}/include" NO_DEFAULT_PATH REQUIRED)
foreach(ffmpeg_library IN ITEMS avcodec avformat avutil)
    find_library(FFMPEG_${ffmpeg_library}_LIBRARY NAMES ${ffmpeg_library}
        PATHS "${FFMPEG_ROOT}/lib" NO_DEFAULT_PATH REQUIRED)
    add_library(FFmpeg::${ffmpeg_library} UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::${ffmpeg_library} PROPERTIES
        IMPORTED_LOCATION "${FFMPEG_${ffmpeg_library}_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}")
endforeach()

##==============================================================================
## Target linking.
##==============================================================================
set(target "landscape")
add_executable(${target}
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AppSystemLogic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/AppWorldLogic.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Drone/Drone.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Drone/PathVisualizer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Drone/PropellerAnimation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Input/GamepadInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Input/InputManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Input/InputUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/Input/KeyboardInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/cameramanager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/hwvideoencoder/hwvideoencoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/hwvideoencoder/rtpstream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/hwvideoencoder/cudautils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/hwvideoencoder/cudatexture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/components/hwvideoencoder/rgba_to_nv12.cu
    ${CMAKE_CURRENT_SOURCE_DIR}/components/imagegenerator.cpp
#    ${CMAKE_CURRENT_SOURCE_DIR}/eventsubscribers/helix.cpp
#    ${CMAKE_CURRENT_SOURCE_DIR}/eventsubscribers/udpreceiver.cpp
)
target_include_directories(${target} PRIVATE
    ${CUDAToolkit_INCLUDE_DIRS}
    ${UNIGINE_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${target} PRIVATE
    Unigine::Engine
    CUDA::cudart
    FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil
)
target_link_options(${target} PRIVATE
    "$<$<CONFIG:Release>:$<HOST_LINK:/SUBSYSTEM:WINDOWS>>"
    "$<$<CONFIG:Release>:$<HOST_LINK:/ENTRY:wmainCRTStartup>>")
set(binary_name ${target})
string(APPEND binary_name "_x64")
set_target_properties(${target} PROPERTIES
    DEBUG_POSTFIX "d"
    OUTPUT_NAME ${binary_name}
    CUDA_SEPARABLE_COMPILATION TRUE
    CUDA_ARCHITECTURES 86
)

##==============================================================================
## Compiler definitions and options.
##==============================================================================
target_compile_definitions(${target} PRIVATE
    $<$<CONFIG:Debug>:DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
    _CRT_SECURE_NO_DEPRECATE
)
include(ProcessorCount)
processorcount(proc_count)
target_compile_options(${target} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:/FS>
    $<$<COMPILE_LANGUAGE:CXX>:/fp:fast>
    $<$<COMPILE_LANGUAGE:CXX>:/bigobj>
    $<$<COMPILE_LANGUAGE:CXX>:/EHsc>
    $<$<COMPILE_LANGUAGE:CXX>:/MP${proc_count}>
)
unset(proc_count)

#add_executable(client
#   client.cpp
#)
