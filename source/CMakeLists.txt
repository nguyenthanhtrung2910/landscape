##==============================================================================
## General.
##==============================================================================
cmake_minimum_required(VERSION 4.0)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/UnigineCMakeVariables.cmake)
project(landscape LANGUAGES C CXX CUDA)

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
set(UNIGINE_SDK_PATH ${PROJECT_SOURCE_DIR}/../)
set(UNIGINE_BIN_DIR ${UNIGINE_SDK_PATH}bin)
set(UNIGINE_LIB_DIR ${UNIGINE_SDK_PATH}lib)
set(UNIGINE_INCLUDE_DIR ${UNIGINE_SDK_PATH}include)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
find_package(Engine REQUIRED MODULE QUIET)
# JsonCpp
find_package(jsoncpp REQUIRED)
# Cuda
find_package(CUDAToolkit REQUIRED)
# FFmpeg
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
)

##==============================================================================
## Target linking.
##==============================================================================
set(target "landscape")
if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE
        "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:wmainCRTStartup")
endif()
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
    ${CMAKE_CURRENT_SOURCE_DIR}/eventsubscribers/helix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/eventsubscribers/udpreceiver.cpp
)
target_include_directories(${target} PRIVATE
    ${CUDAToolkit_INCLUDE_DIRS}
    ${UNIGINE_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(${target} PRIVATE
    Unigine::Engine
    CUDA::cudart
    JsonCpp::JsonCpp
    PkgConfig::FFMPEG
)
set_property(TARGET ${target} APPEND PROPERTY
    BUILD_RPATH ${CMAKE_CURRENT_SOURCE_DIR}/../bin
)
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
    $<$<BOOL:${UNIX}>:_LINUX>
    $<$<CONFIG:Debug>:DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(UNIGINE_COMPILER_IS_MSVC TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(UNIGINE_COMPILER_IS_CLANG TRUE)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(UNIGINE_COMPILER_IS_GNU TRUE)
endif()
if(UNIGINE_COMPILER_IS_MSVC)
    target_compile_definitions(${target} PRIVATE
        _CRT_SECURE_NO_DEPRECATE
    )
    include(ProcessorCount)
    processorcount(proc_count)
    target_compile_options(${target} PRIVATE
        /TP # Specifies all source files are C++.
        /FS # Forces writes to the program database (PDB) file to be serialized through MSPDBSRV.EXE.
        /fp:fast # Specifies floating-point behavior.
        /bigobj # Increases the number of addressable sections in an .obj file.
        /EHsc # Specifies the kind of exception handling used by the compiler.
        /MP${proc_count} # Build with Multiple Processes.
    )
    unset(proc_count)
    target_link_options(${target} INTERFACE "/FIXED:NO")
elseif(UNIGINE_COMPILER_IS_GNU OR UNIGINE_COMPILER_IS_CLANG)
    target_compile_options(${target} PRIVATE
        -m64
        -march=athlon64
        -msse4.2
        # Optimizations.
        -ffast-math
        -fno-strict-aliasing
        $<$<NOT:$<CONFIG:Debug>>:-fomit-frame-pointer>
        # Warnings.
        $<$<CONFIG:Debug>:-Wall>
        $<$<CONFIG:Debug>:-Wno-unknown-pragmas>
        $<$<CONFIG:Debug>:-Wno-unused-parameter>
    )
endif()

add_executable(client
    client.cpp
)
