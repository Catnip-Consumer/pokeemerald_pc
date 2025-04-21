
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

add_executable(preproc
	"tools/preproc/asm_file.cpp"
	"tools/preproc/c_file.cpp"
	"tools/preproc/charmap.cpp"
	"tools/preproc/preproc.cpp"
	"tools/preproc/string_parser.cpp"
	"tools/preproc/utf8.cpp"
	"tools/preproc/io.cpp"
)

target_compile_definitions(preproc PRIVATE APPDIR="tools/preproc")
target_compile_definitions(preproc PRIVATE PROGRAMNAME="preproc")
set_target_properties(preproc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")
set(PREPROC $<TARGET_FILE:preproc>)

cmake_policy(SET CMP0065 NEW)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(preproc PRIVATE -Wall -Wno-switch -Werror)
endif()
