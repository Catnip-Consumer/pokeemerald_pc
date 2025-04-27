cmake_minimum_required(VERSION 3.18)

project(PokeEmerald
	VERSION 0.0.0
	LANGUAGES C CXX
)

# find some tools to help make scripts
cmake_path(GET CMAKE_C_COMPILER PARENT_PATH C_COMPILER_BASE_PATH)
set(CPP ${C_COMPILER_BASE_PATH}/cpp${CMAKE_EXECUTABLE_SUFFIX})
set(GCC ${C_COMPILER_BASE_PATH}/gcc${CMAKE_EXECUTABLE_SUFFIX})
set(AS ${C_COMPILER_BASE_PATH}/as${CMAKE_EXECUTABLE_SUFFIX})
set(LD ${C_COMPILER_BASE_PATH}/ld${CMAKE_EXECUTABLE_SUFFIX})
set(OBJCOPY ${C_COMPILER_BASE_PATH}/objcopy${CMAKE_EXECUTABLE_SUFFIX})

# Determine the null device based on the platform
if(WIN32)
	set(NULL_DEVICE "NUL")
else()
	set(NULL_DEVICE "/dev/null")
endif()

# SDL2
# https://github.com/libsdl-org/SDL/issues/1481
# On 2014-06-22 17:15:50 +0000, Sam Lantinga wrote:
#   If you link SDL statically, you also need to define HAVE_LIBC so it builds with the C runtime that your application uses.
#   This should probably go in a FAQ.
set(SDL_LIBC ON CACHE BOOL "Tell SDL that we want it to use our C runtime (required for proper static linking)" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SYSTEM_SDL_MIN_VER 2.32.0)

#add_definitions(-DSDL_MAIN_HANDLED)
add_subdirectory(${CMAKE_SOURCE_DIR}/extern/SDL2 EXCLUDE_FROM_ALL)

# ImGui
include_directories(${CMAKE_SOURCE_DIR}/extern/imgui)

# OpenGL
find_package(OpenGL REQUIRED)

include("./tools/aif2pcm/make.cmake")
include("./tools/bin2c/make.cmake")
include("./tools/gbafix/make.cmake")
include("./tools/gbagfx/make.cmake")
include("./tools/jsonproc/make.cmake")
include("./tools/mapjson/make.cmake")
include("./tools/mid2agb/make.cmake")
include("./tools/preproc/make.cmake")
include("./tools/ramscrgen/make.cmake")
include("./tools/rsfont/make.cmake")
include("./tools/scaninc/make.cmake")

include("./compile/sound-cvt.cmake")
include("./compile/dll.cmake")
include("./compile/sdl2.cmake")
include("./compile/ai.cmake")
