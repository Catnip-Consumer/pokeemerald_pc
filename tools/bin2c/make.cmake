
set(CMAKE_C_STANDARD 11)

add_executable(bin2c
	"tools/bin2c/bin2c.c"
)

target_compile_definitions(bin2c PRIVATE APPDIR="tools/bin2c")
target_compile_definitions(bin2c PRIVATE PROGRAMNAME="bin2c")
set_target_properties(bin2c PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")

cmake_policy(SET CMP0065 NEW)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(bin2c PRIVATE -Wall -Wextra -Werror)
endif()
