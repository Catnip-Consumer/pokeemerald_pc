cmake_minimum_required(VERSION 3.18)

project(PokeEmerald-sdl2
	VERSION 0.0.0
	LANGUAGES C CXX
)

# requires at least C11/C++17-compatible compiler to compile the code
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

cmake_policy(SET CMP0065 NEW)

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

# Source files (data/ directory)
set(PokeEmerald_SDL2_SOURCES
	"src/platform/gba_easy_draw.c"
#	"src/platform/gba_fast_draw.c"
	"src/platform/sdl2.c"
)

# source files
if(WIN32)
	list(APPEND PokeEmerald_SDL2_SOURCES
	#	"src/platform/win32.c"
	)
else()
	list(APPEND PokeEmerald_SDL2_SOURCES

	)
endif()

# DLL target
add_executable(emerald-sdl2
	${PokeEmerald_SDL2_SOURCES}
)

set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")
set_target_properties(emerald-sdl2 PROPERTIES LINKER_LANGUAGE C)
target_link_libraries(emerald-sdl2 PRIVATE emerald SDL2main SDL2-static xinput)

# Build flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(emerald-sdl2 PRIVATE -Wformat -Wformat-security -fomit-frame-pointer)
	target_compile_options(emerald-sdl2 PRIVATE -Wno-trigraphs -Wimplicit -Wno-int-conversion -Wparentheses -Wunused)
	target_compile_options(emerald-sdl2 PRIVATE -fleading-underscore -fno-dce -fno-builtin -Wno-unused-function)
	target_compile_options(emerald-sdl2 PRIVATE -mmmx -msse -msse2 -mfxsr -m32)

	if(WIN32)
		# handle windows-specific options
		link_libraries(-static gcc stdc++ winpthread)

		if(CMAKE_BUILD_TYPE STREQUAL "Debug")
			target_compile_options(emerald-sdl2 PRIVATE -mconsole)
		endif()
	endif()

	# strip all symbol info on gnu on release builds
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		target_compile_options(emerald-sdl2 PRIVATE -fdata-sections -ffunction-sections -flto)
		target_link_options(emerald-sdl2 PRIVATE -Wl,--gc-sections -Wl,-flto -Wl,--strip-all)
	endif()
endif()

# Include dirs
target_include_directories(emerald-sdl2 PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(emerald-sdl2 PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(emerald-sdl2 PRIVATE ${CMAKE_SOURCE_DIR}/extern/SDL/include)

# Compiler options
target_compile_options(emerald-sdl2 PRIVATE
	-Wformat -Wformat-security -fomit-frame-pointer
	-Wno-trigraphs -Wimplicit -Wparentheses -Wunused
	-fleading-underscore -fno-dce -fno-builtin
	-mmmx -msse -msse2 -mfxsr
	-m32 -std=gnu99 -O3
)

# Compiler toggle flags
target_compile_definitions(emerald-sdl2 PRIVATE RENDERER_EASY_DRAW)
target_compile_definitions(emerald-sdl2 PRIVATE NONMATCHING)
target_compile_definitions(emerald-sdl2 PRIVATE MODERN=1)
target_compile_definitions(emerald-sdl2 PRIVATE PORTABLE=1)
target_compile_definitions(emerald-sdl2 PRIVATE UBFIX=1)

# Add `DEBUG` macro definition if compiling under Debug or RelWithDebInfo
# configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(emerald-sdl2 PRIVATE DEBUG)
endif()
