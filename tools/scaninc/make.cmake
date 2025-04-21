
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

add_executable(scaninc
	"tools/scaninc/asm_file.cpp"
	"tools/scaninc/c_file.cpp"
	"tools/scaninc/scaninc.cpp"
	"tools/scaninc/source_file.cpp"
)

target_compile_definitions(scaninc PRIVATE APPDIR="tools/scaninc")
target_compile_definitions(scaninc PRIVATE PROGRAMNAME="scaninc")
set_target_properties(scaninc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")

cmake_policy(SET CMP0065 NEW)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(scaninc PRIVATE -Wall -Werror)
endif()
