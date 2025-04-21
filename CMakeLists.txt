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
