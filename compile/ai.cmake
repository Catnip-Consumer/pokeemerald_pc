cmake_minimum_required(VERSION 3.18)

project(PokeEmerald-ai
	VERSION 0.0.0
	LANGUAGES C CXX
)

# requires at least C11/C++20-compatible compiler to compile the code
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

cmake_policy(SET CMP0065 NEW)

# Source files (data/ directory)
set(PokeEmerald_AI_SOURCES
	"src/platform/gba_easy_draw.c"
	"src/ai/sdl2.cpp"
	"src/ai/main.cpp"
)

# DLL target
add_executable(emerald-ai
	${PokeEmerald_AI_SOURCES}
)

set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")
target_link_libraries(emerald-ai PRIVATE emerald SDL2main SDL2-static xinput)

# Build flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(emerald-ai PRIVATE -Wall -Wformat -Wformat-security -fomit-frame-pointer)
	target_compile_options(emerald-ai PRIVATE -Wno-trigraphs -Wparentheses -Wunused)
	target_compile_options(emerald-ai PRIVATE -fleading-underscore -fno-dce -fno-builtin)

	if(WIN32)
		# handle windows-specific options
		link_libraries(-static gcc stdc++ winpthread)

		if(CMAKE_BUILD_TYPE STREQUAL "Debug")
			target_compile_options(emerald-ai PRIVATE -mconsole)
		endif()
	endif()

	# strip all symbol info on gnu on release builds
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		target_compile_options(emerald-ai PRIVATE -fdata-sections -ffunction-sections -flto)
		target_link_options(emerald-ai PRIVATE -Wl,--gc-sections -Wl,-flto -Wl,--strip-all)
	endif()
endif()

# Include dirs
target_include_directories(emerald-ai PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(emerald-ai PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_include_directories(emerald-ai PRIVATE ${CMAKE_SOURCE_DIR}/extern/SDL/include)

# Compiler toggle flags
target_compile_definitions(emerald-ai PRIVATE RENDERER_EASY_DRAW)
target_compile_definitions(emerald-ai PRIVATE NONMATCHING)
target_compile_definitions(emerald-ai PRIVATE MODERN=1)
target_compile_definitions(emerald-ai PRIVATE PORTABLE=1)
target_compile_definitions(emerald-ai PRIVATE UBFIX=1)

# Add `DEBUG` macro definition if compiling under Debug or RelWithDebInfo
# configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(emerald-ai PRIVATE DEBUG)
endif()

# Copy AI save file to build directory
file(COPY ${CMAKE_SOURCE_DIR}/compile/emerald-ai.sav DESTINATION ${CMAKE_BINARY_DIR})
