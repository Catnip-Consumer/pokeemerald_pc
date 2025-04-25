cmake_minimum_required(VERSION 3.18)

project(PokeEmerald-sdl2
	VERSION 0.0.0
	LANGUAGES C
)

# requires at least C11-compatible compiler to compile the code
set(CMAKE_C_STANDARD 11)

cmake_policy(SET CMP0065 NEW)

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
	target_compile_options(emerald-sdl2 PRIVATE -Wformat -Wformat-security -fomit-frame-pointer -msse3)
	target_compile_options(emerald-sdl2 PRIVATE -Wno-trigraphs -Wimplicit -Wno-int-conversion -Wparentheses -Wunused -Wno-unused-function)

	if(WIN32)
		# handle windows-specific options
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

# Compiler toggle flags
target_compile_definitions(emerald-sdl2 PRIVATE RENDERER_EASY_DRAW)
target_compile_definitions(emerald-sdl2 PRIVATE NONMATCHING)
target_compile_definitions(emerald-sdl2 PRIVATE MODERN=1)
target_compile_definitions(emerald-sdl2 PRIVATE PORTABLE=1)
target_compile_definitions(emerald-sdl2 PRIVATE UBFIX=1)
target_compile_definitions(emerald-sdl2 PRIVATE VER_64BIT=1)

# Add `DEBUG` macro definition if compiling under Debug or RelWithDebInfo
# configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(emerald-sdl2 PRIVATE DEBUG)
endif()
