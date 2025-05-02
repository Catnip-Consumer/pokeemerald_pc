cmake_minimum_required(VERSION 3.18)

project(PokeEmerald-ai
	VERSION 0.0.0
	LANGUAGES C CXX
)

set(AI_HAS_GUI ON CACHE STRING "If enabled, SDL2 with ImGui will be compiled and used to display what AI is doing.")

# requires at least C11/C++23-compatible compiler to compile the code
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)

cmake_policy(SET CMP0065 NEW)

# Source files (data/ directory)
set(PokeEmerald_AI_SOURCES
	"src/ai/system/shared.cpp"
	"src/ai/main.cpp"
	"src/ai/agent.cpp"
	"src/ai/model-manager.cpp"
	"src/ai/library/emerald-str.cpp"
)

if(WIN32)
	list(APPEND PokeEmerald_AI_SOURCES
		"src/ai/system/win32.cpp"
	)
elseif(LINUX)
	list(APPEND PokeEmerald_AI_SOURCES
		"src/ai/system/linux.cpp"
	)
else()
	list(APPEND PokeEmerald_AI_SOURCES
		"src/ai/system/macos.cpp"
	)
endif()

if(AI_HAS_GUI)
	list(APPEND PokeEmerald_AI_SOURCES
		"src/ai/system/thread_safe_gba_easy_draw.cpp"
		"src/ai/gui/sdl2.cpp"
		"src/ai/gui/view-all-ai.cpp"
		"src/ai/gui/ai-info.cpp"
		"src/ai/gui/log-window.cpp"
		"src/ai/gui/pokemon-details.cpp"

		"extern/imgui/imgui.cpp"
		"extern/imgui/imgui_draw.cpp"
		"extern/imgui/imgui_tables.cpp"
		"extern/imgui/imgui_widgets.cpp"
		"extern/imgui/backends/imgui_impl_sdlrenderer2.cpp"
		"extern/imgui/backends/imgui_impl_opengl3.cpp"
		"extern/imgui/backends/imgui_impl_sdl2.cpp"
		"extern/imgui/misc/cpp/imgui_stdlib.cpp"
	)
endif()

# DLL target
add_executable(emerald-ai
	${PokeEmerald_AI_SOURCES}
)

target_link_libraries(emerald-ai PRIVATE openblas)

set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")

# Extra libraries and compiler flag if SDL2 is enabled
if(AI_HAS_GUI)
	target_compile_definitions(emerald-ai PRIVATE ENABLE_SDL2)
	target_link_libraries(emerald-ai PRIVATE SDL2main SDL2-static xinput ${OPENGL_LIBRARIES})
endif()

# Build flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(emerald-ai PRIVATE -Wall -Wformat -Wformat-security -fomit-frame-pointer)
	target_compile_options(emerald-ai PRIVATE -Wno-trigraphs -Wparentheses -Wunused -fopenmp -Wno-deprecated-declarations)
    target_link_options(emerald-ai PRIVATE -fopenmp)

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
target_include_directories(emerald-ai PRIVATE ${CMAKE_SOURCE_DIR}/extern/cereal/include)
target_include_directories(emerald-ai PRIVATE ${CMAKE_SOURCE_DIR}/extern/ensmallen/include)
target_include_directories(emerald-ai PRIVATE ${ARMADILLO_INCLUDE_DIRS})

# Compiler toggle flags
target_compile_definitions(emerald-ai PRIVATE RENDERER_EASY_DRAW)
target_compile_definitions(emerald-ai PRIVATE NONMATCHING)
target_compile_definitions(emerald-ai PRIVATE MODERN=1)
target_compile_definitions(emerald-ai PRIVATE PORTABLE=1)
target_compile_definitions(emerald-ai PRIVATE UBFIX=1)
target_compile_definitions(emerald-ai PRIVATE VER_64BIT=1)

# Armadillo
set(Armadillo_DIR "" CACHE PATH "Path to Armadillo installation")
if(NOT Armadillo_DIR)
	message(FATAL_ERROR "Armadillo installation not found. Please set Armadillo_DIR to the path of your Armadillo installation.")
endif()

target_include_directories(emerald-ai PRIVATE "${Armadillo_DIR}/include")
target_link_directories(emerald-ai PRIVATE "${Armadillo_DIR}/examples/lib_win64")

# mlpack
set(mlpack_DIR "" CACHE PATH "Path to mlpack installation")
if(NOT mlpack_DIR)
	message(FATAL_ERROR "mlpack installation not found. Please set mlpack_DIR to the path of your mlpack installation.")
endif()

target_include_directories(emerald-ai PRIVATE "${mlpack_DIR}/include")
target_link_directories(emerald-ai PRIVATE "${mlpack_DIR}/lib")

# Add `DEBUG` macro definition if compiling under Debug or RelWithDebInfo
# configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_definitions(emerald-ai PRIVATE DEBUG)
endif()

# Copy AI save file to build directory
file(COPY ${CMAKE_SOURCE_DIR}/compile/emerald-ai.sav DESTINATION ${CMAKE_BINARY_DIR})

# Copy resources to build directory
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_BINARY_DIR})
