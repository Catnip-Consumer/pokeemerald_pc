
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

add_executable(ramscrgen
	"tools/ramscrgen/main.cpp"
	"tools/ramscrgen/elf.cpp"
	"tools/ramscrgen/sym_file.cpp"
)

target_compile_definitions(ramscrgen PRIVATE APPDIR="tools/ramscrgen")
target_compile_definitions(ramscrgen PRIVATE PROGRAMNAME="ramscrgen")
set_target_properties(ramscrgen PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")

cmake_policy(SET CMP0065 NEW)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(ramscrgen PRIVATE -Wall -Wno-switch -Werror)
endif()
