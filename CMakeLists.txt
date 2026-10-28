cmake_minimum_required(VERSION 3.20)

if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version")
endif()

project(1080SnowboardingRecompiled LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Small fixes to the runtime submodule (see patches/runtime/). Applied once;
# a patch that is already in place is left alone.
find_package(Git QUIET)
foreach(runtime_patch IN ITEMS
        "lib/N64ModernRuntime|patches/runtime/N64ModernRuntime.patch"
        "lib/RecompFrontend|patches/runtime/RecompFrontend.patch"
        "lib/rt64|patches/runtime/rt64-camera-cuts.patch")
    string(REPLACE "|" ";" runtime_patch_parts "${runtime_patch}")
    list(GET runtime_patch_parts 0 patch_dir)
    list(GET runtime_patch_parts 1 patch_file)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_SOURCE_DIR}/${patch_dir}" apply --reverse --check "${CMAKE_SOURCE_DIR}/${patch_file}"
        RESULT_VARIABLE patch_already_applied OUTPUT_QUIET ERROR_QUIET)
    if (NOT patch_already_applied EQUAL 0)
        execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_SOURCE_DIR}/${patch_dir}" apply "${CMAKE_SOURCE_DIR}/${patch_file}"
            RESULT_VARIABLE patch_result)
        if (NOT patch_result EQUAL 0)
            message(FATAL_ERROR "Could not apply ${patch_file}")
        endif()
    endif()
endforeach()

# Where tools/recompile.sh put the recompiled game. Nothing derived from the
# ROM is checked in, so run that script first.
set(GAME_GENERATED_DIR "${CMAKE_SOURCE_DIR}/build" CACHE PATH "Output folder of tools/recompile.sh")
foreach(required RecompiledFuncs/funcs.h rsp/aspMain.cpp recomp-input/module_table.inl)
    if (NOT EXISTS "${GAME_GENERATED_DIR}/${required}")
        message(FATAL_ERROR "${GAME_GENERATED_DIR}/${required} is missing. Run tools/recompile.sh with your ROM first.")
    endif()
endforeach()

set(GAME_VERSION "1.0.1" CACHE STRING "Version shown in the launcher")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/game_version.h.tmp"
    "#pragma once\ninline constexpr const char* game_version = \"${GAME_VERSION}\";\n")
configure_file("${CMAKE_CURRENT_BINARY_DIR}/generated/game_version.h.tmp"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/game_version.h" COPYONLY)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-everything /W4")
endif()

# Opt out of constexpr mutex constructor on windows to prevent vcredist issues
if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR")
    add_compile_definitions(__PRFCHWINTRIN_H)
endif()

if (APPLE)
    enable_language(OBJC OBJCXX)
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    option(RECOMP_FLATPAK "Configure the build for Flatpak compatibility." OFF)
endif()

# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if (WIN32)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/lib/")
endif()

set(RT64_STATIC TRUE)
set(RT64_SDL_WINDOW_VULKAN TRUE)
add_compile_definitions(HLSL_CPU)

if (RECOMP_FLATPAK)
    add_compile_definitions(RECOMP_FLATPAK)
endif()

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/rt64 ${CMAKE_BINARY_DIR}/rt64)

set(BUILD_SHARED_LIBS OFF)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime)

target_include_directories(rt64 PRIVATE ${CMAKE_BINARY_DIR}/rt64/src)

# RecompiledFuncs - Library containing the primary recompiler output
add_library(RecompiledFuncs STATIC)

target_compile_options(RecompiledFuncs PRIVATE
    # -Wno-unused-but-set-variable
    -fno-strict-aliasing
    -Wno-unused-variable
    -Wno-implicit-function-declaration
)

target_include_directories(RecompiledFuncs PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/ultramodern/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/librecomp/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
)

file(GLOB FUNC_C_SOURCES ${GAME_GENERATED_DIR}/RecompiledFuncs/*.c)
file(GLOB FUNC_CXX_SOURCES ${GAME_GENERATED_DIR}/RecompiledFuncs/*.cpp)

target_sources(RecompiledFuncs PRIVATE ${FUNC_C_SOURCES} ${FUNC_CXX_SOURCES})

# Main executable
add_executable(1080SnowboardingRecompiled)

set (SOURCES
    ${CMAKE_SOURCE_DIR}/src/main/main.cpp
    ${CMAKE_SOURCE_DIR}/src/main/crash_log.cpp
    ${CMAKE_SOURCE_DIR}/src/main/theme.cpp
    ${CMAKE_SOURCE_DIR}/src/main/launcher.cpp
    ${CMAKE_SOURCE_DIR}/src/main/register_overlays.cpp

    ${CMAKE_SOURCE_DIR}/src/game/config.cpp
    ${CMAKE_SOURCE_DIR}/src/game/libultra_extras.cpp
    ${CMAKE_SOURCE_DIR}/src/game/boot_residue.cpp
    ${CMAKE_SOURCE_DIR}/src/game/draw_distance.cpp
    ${CMAKE_SOURCE_DIR}/src/game/shadows.cpp
    ${CMAKE_SOURCE_DIR}/src/game/camera.cpp
    ${CMAKE_SOURCE_DIR}/src/game/crt_filter.cpp
    ${CMAKE_SOURCE_DIR}/src/game/vsync.cpp
    ${CMAKE_SOURCE_DIR}/src/game/hd_textures.cpp
    ${CMAKE_SOURCE_DIR}/src/game/boards.cpp
    ${CMAKE_SOURCE_DIR}/src/game/achievements.cpp

    ${GAME_GENERATED_DIR}/rsp/aspMain.cpp
)

target_include_directories(1080SnowboardingRecompiled PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${GAME_GENERATED_DIR}/RecompiledFuncs
    ${GAME_GENERATED_DIR}/recomp-input
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/N64Recomp/include
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/thirdparty
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/thirdparty/concurrentqueue
    ${CMAKE_SOURCE_DIR}/lib/RecompFrontend/recompinput/include
    ${CMAKE_SOURCE_DIR}/lib/RecompFrontend/recompui/include
    ${CMAKE_SOURCE_DIR}/lib/RecompFrontend/recompui/src
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/hlslpp/include
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/dxc/inc
    ${CMAKE_SOURCE_DIR}/lib/rt64/src
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/rhi
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/render
    ${CMAKE_SOURCE_DIR}/lib/freetype-windows-binaries/include
    ${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/nativefiledialog-extended/src/include
    ${CMAKE_BINARY_DIR}/shaders
    ${CMAKE_CURRENT_BINARY_DIR}/generated
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
    target_compile_options(1080SnowboardingRecompiled PRIVATE
        -march=nehalem
        # -march=core2
        # -msse4.1 # added for core2
        -fno-strict-aliasing
        -fms-extensions
    )
else()
    target_compile_options(1080SnowboardingRecompiled PRIVATE
        -fno-strict-aliasing
        -fms-extensions
    )
endif()

if (MSVC)
    # Disable identical code folding, since this breaks mod function patching as multiple functions can get merged into one.
    target_link_options(1080SnowboardingRecompiled PRIVATE /OPT:NOICF)
endif()

if (WIN32)
    include(FetchContent)

    if (DEFINED ENV{SDL2_VERSION})
        set(SDL2_VERSION $ENV{SDL2_VERSION})
    else()
        set(SDL2_VERSION "2.30.3")
    endif()

    # Fetch SDL2 on windows
    FetchContent_Declare(
        sdl2
        URL https://github.com/libsdl-org/SDL/releases/download/release-${SDL2_VERSION}/SDL2-devel-${SDL2_VERSION}-VC.zip
    )
    FetchContent_MakeAvailable(sdl2)
    target_include_directories(1080SnowboardingRecompiled PRIVATE
        ${sdl2_SOURCE_DIR}/include
    )
    target_link_directories(1080SnowboardingRecompiled PRIVATE
        ${sdl2_SOURCE_DIR}/lib/x64
    )

    # Copy SDL2 and dxc DLLs to output folder as post build step
    add_custom_command(TARGET 1080SnowboardingRecompiled POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${sdl2_SOURCE_DIR}/lib/x64/SDL2.dll"
            "${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxil.dll"
            "${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxcompiler.dll"
            $<TARGET_FILE_DIR:1080SnowboardingRecompiled>)

    if (MINGW)
        # Cross-compiled with llvm-mingw (tools/build-windows.sh).
        target_link_options(1080SnowboardingRecompiled PRIVATE $<$<NOT:$<CONFIG:Debug>>:-mwindows>)
        target_sources(1080SnowboardingRecompiled PRIVATE ${CMAKE_SOURCE_DIR}/src/main/mingw_uuids.cpp)
        target_include_directories(1080SnowboardingRecompiled PRIVATE ${CMAKE_SOURCE_DIR}/lib/rt64/src/contrib/dxc/inc)
        target_link_libraries(1080SnowboardingRecompiled PRIVATE dwmapi d3dcompiler)
    else()
        set_target_properties(
            1080SnowboardingRecompiled
            PROPERTIES
                LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
                LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
                LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
                LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
        )
    endif()

    target_sources(1080SnowboardingRecompiled PRIVATE ${CMAKE_SOURCE_DIR}/icons/app.rc)
    target_link_libraries(1080SnowboardingRecompiled PRIVATE SDL2 winmm)
endif()

if (APPLE)
    find_package(SDL2 REQUIRED)
    target_include_directories(1080SnowboardingRecompiled PRIVATE ${SDL2_INCLUDE_DIRS})

    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    target_link_libraries(1080SnowboardingRecompiled PRIVATE ${CMAKE_DL_LIBS} Threads::Threads SDL2::SDL2)

endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    find_package(SDL2 REQUIRED)
    find_package(X11 REQUIRED)

    add_compile_definitions(
        "PLUME_SDL_VULKAN_ENABLED"
        "RT64_SDL_WINDOW_VULKAN"
    )

    # Generate icon_bytes.c from the app icon PNG.
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/icon_bytes.c ${CMAKE_CURRENT_BINARY_DIR}/icon_bytes.h
        COMMAND file_to_c ${CMAKE_SOURCE_DIR}/icons/512.png icon_bytes ${CMAKE_CURRENT_BINARY_DIR}/icon_bytes.c ${CMAKE_CURRENT_BINARY_DIR}/icon_bytes.h
        DEPENDS ${CMAKE_SOURCE_DIR}/icons/512.png
    )
    target_sources(1080SnowboardingRecompiled PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/icon_bytes.c)

    message(STATUS "SDL2_FOUND = ${SDL2_FOUND}")
    message(STATUS "SDL2_INCLUDE_DIRS = ${SDL2_INCLUDE_DIRS}")

    target_include_directories(1080SnowboardingRecompiled PRIVATE ${SDL2_INCLUDE_DIRS})

    message(STATUS "X11_FOUND = ${X11_FOUND}")
    message(STATUS "X11_Xrandr_FOUND = ${X11_Xrandr_FOUND}")
    message(STATUS "X11_INCLUDE_DIR = ${X11_INCLUDE_DIR}")
    message(STATUS "X11_LIBRARIES = ${X11_LIBRARIES}")

    target_include_directories(1080SnowboardingRecompiled PRIVATE ${X11_INCLUDE_DIR} ${X11_Xrandr_INCLUDE_PATH})
    target_link_libraries(1080SnowboardingRecompiled PRIVATE ${X11_LIBRARIES} ${X11_Xrandr_LIB})

    find_package(Freetype REQUIRED)

    message(STATUS "FREETYPE_FOUND = ${FREETYPE_FOUND}")
    message(STATUS "FREETYPE_INCLUDE_DIRS = ${FREETYPE_INCLUDE_DIRS}")
    message(STATUS "FREETYPE_LIBRARIES = ${FREETYPE_LIBRARIES}")

    include_directories(${FREETYPE_LIBRARIES})
    target_link_libraries(1080SnowboardingRecompiled PRIVATE ${FREETYPE_LIBRARIES} SDL2::SDL2)

    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    set(THREADS_PREFER_PTHREAD_FLAG TRUE)
    find_package(Threads REQUIRED)

    target_link_libraries(1080SnowboardingRecompiled PRIVATE "-latomic -static-libstdc++" ${CMAKE_DL_LIBS} Threads::Threads)
endif()

# TODO fix the rt64 CMake script so that this doesn't need to be duplicated here
# For DXC
set (DXC_COMMON_OPTS "-I${PROJECT_SOURCE_DIR}/src")
set (DXC_DXIL_OPTS "-Wno-ignored-attributes")
set (DXC_SPV_OPTS "-spirv" "-fspv-target-env=vulkan1.0" "-fvk-use-dx-layout")
set (DXC_PS_OPTS "${DXC_COMMON_OPTS}" "-E" "PSMain" "-T ps_6_0" "-D DYNAMIC_RENDER_PARAMS")
set (DXC_VS_OPTS "${DXC_COMMON_OPTS}" "-E" "VSMain" "-T vs_6_0" "-D DYNAMIC_RENDER_PARAMS" "-fvk-invert-y")
set (DXC_CS_OPTS "${DXC_COMMON_OPTS}" "-E" "CSMain" "-T cs_6_0")
set (DXC_GS_OPTS "${DXC_COMMON_OPTS}" "-E" "GSMain" "-T gs_6_0")
set (DXC_RT_OPTS "${DXC_COMMON_OPTS}" "-D" "RT_SHADER" "-T" "lib_6_3" "-fspv-target-env=vulkan1.1spirv1.4" "-fspv-extension=SPV_KHR_ray_tracing" "-fspv-extension=SPV_EXT_descriptor_indexing")

if (${WIN32})
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        # Cross-compiling from Linux (see tools/build-windows.sh).
        set (DXC "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/lib/x64" "${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxc-linux")
    else()
        set (DXC "${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxc.exe")
    endif()
    add_compile_definitions(NOMINMAX)
else()
    if (APPLE)
        # Apple's binary is universal, so it'll work on both x86_64 and arm64
        set (DXC "DYLD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/lib/arm64" "${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/arm64/dxc-macos")
        if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
            set(SPIRVCROSS "DYLD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/spirv-cross/lib/x64" "${PROJECT_SOURCE_DIR}/lib/rt64//src/contrib/spirv-cross/bin/x64/spirv-cross")
        else()
            set(SPIRVCROSS "DYLD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/spirv-cross/lib/arm64" "${PROJECT_SOURCE_DIR}/lib/rt64//src/contrib/spirv-cross/bin/x64/spirv-cross")
        endif()
    else()
        if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
            set (DXC "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/lib/x64" "${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/x64/dxc-linux")
        else()
            set (DXC "LD_LIBRARY_PATH=${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/lib/arm64" "${PROJECT_SOURCE_DIR}/lib/rt64/src/contrib/dxc/bin/arm64/dxc-linux")
        endif()
    endif()
endif()

set(RECOMP_FRONTEND_N64MODERNRUNTIME_PATH ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime)
set(RECOMP_FRONTEND_RT64_PATH ${CMAKE_SOURCE_DIR}/lib/rt64)

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/RecompFrontend)

target_include_directories(recompui BEFORE PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/thirdparty
)
target_include_directories(recompinput BEFORE PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/thirdparty
)

# RetroAchievements client library (MIT).
file(GLOB RCHEEVOS_SOURCES
    ${CMAKE_SOURCE_DIR}/lib/rcheevos/src/rapi/*.c
    ${CMAKE_SOURCE_DIR}/lib/rcheevos/src/rcheevos/*.c
)
add_library(rcheevos STATIC
    ${RCHEEVOS_SOURCES}
    ${CMAKE_SOURCE_DIR}/lib/rcheevos/src/rc_client.c
    ${CMAKE_SOURCE_DIR}/lib/rcheevos/src/rc_compat.c
    ${CMAKE_SOURCE_DIR}/lib/rcheevos/src/rc_util.c
    ${CMAKE_SOURCE_DIR}/lib/rcheevos/src/rc_version.c
    ${CMAKE_SOURCE_DIR}/lib/rcheevos/src/rhash/md5.c
)
target_include_directories(rcheevos PUBLIC
    ${CMAKE_SOURCE_DIR}/lib/rcheevos/include
    ${CMAKE_SOURCE_DIR}/lib/rcheevos/src
)
target_compile_definitions(rcheevos PUBLIC RC_DISABLE_LUA)
if (WIN32)
    target_link_libraries(1080SnowboardingRecompiled PRIVATE winhttp)
endif()

target_link_libraries(1080SnowboardingRecompiled PRIVATE
    rcheevos
    RecompiledFuncs
    recompui
    recompinput
    librecomp
    ultramodern
    rt64
    nfd
)

target_sources(1080SnowboardingRecompiled PRIVATE ${SOURCES})

if (WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Refresh runtime assets even when the executable does not need to relink.
    add_custom_target(StageRuntimeAssets
        COMMAND ${CMAKE_COMMAND}
            "-DOUTPUT_DIR=$<TARGET_FILE_DIR:1080SnowboardingRecompiled>/assets"
            -P "${CMAKE_SOURCE_DIR}/cmake/stage-assets.cmake"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/recompcontrollerdb.txt"
            "$<TARGET_FILE_DIR:1080SnowboardingRecompiled>/recompcontrollerdb.txt"
        VERBATIM
    )
    add_dependencies(1080SnowboardingRecompiled StageRuntimeAssets)
endif()

if (WIN32)
    set_property(TARGET 1080SnowboardingRecompiled PROPERTY
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:1080SnowboardingRecompiled>")
else()
    set_property(TARGET 1080SnowboardingRecompiled PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
endif()
