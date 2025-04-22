
set(CMAKE_C_STANDARD 11)

add_executable(aif2pcm
	"tools/aif2pcm/main.c"
	"tools/aif2pcm/extended.c"
)

target_compile_definitions(aif2pcm PRIVATE APPDIR="tools/aif2pcm")
target_compile_definitions(aif2pcm PRIVATE PROGRAMNAME="aif2pcm")
set_target_properties(aif2pcm PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")
set(AIF2PCM $<TARGET_FILE:aif2pcm>)

cmake_policy(SET CMP0065 NEW)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(aif2pcm PRIVATE -Wall -Wextra -Wno-switch -Werror)
endif()
