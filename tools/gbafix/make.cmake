
set(CMAKE_C_STANDARD 11)

add_executable(gbafix
	"tools/gbafix/gbafix.c"
)

target_compile_definitions(gbafix PRIVATE APPDIR="tools/gbafix")
target_compile_definitions(gbafix PRIVATE PROGRAMNAME="gbafix")
set_target_properties(gbafix PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")

cmake_policy(SET CMP0065 NEW)
