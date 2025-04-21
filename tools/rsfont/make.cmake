
set(CMAKE_C_STANDARD 11)

add_executable(rsfont
	"tools/rsfont/main.c"
	"tools/rsfont/convert_png.c"
	"tools/rsfont/util.c"
	"tools/rsfont/font.c"
)

# libpng
find_package(PNG REQUIRED)
target_include_directories(rsfont PRIVATE ${PNG_INCLUDE_DIR})
target_link_libraries(rsfont PRIVATE ${PNG_LIBRARY})
target_compile_definitions(rsfont PRIVATE PNG_SKIP_SETJMP_CHECK)

target_compile_definitions(rsfont PRIVATE APPDIR="tools/rsfont")
target_compile_definitions(rsfont PRIVATE PROGRAMNAME="rsfont")
set_target_properties(rsfont PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")

cmake_policy(SET CMP0065 NEW)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(rsfont PRIVATE -Wall -Werror -Wextra)
endif()
