
set(CMAKE_C_STANDARD 11)

add_executable(gbagfx
	"tools/gbagfx/main.c"
	"tools/gbagfx/convert_png.c"
	"tools/gbagfx/gfx.c"
	"tools/gbagfx/jasc_pal.c"
	"tools/gbagfx/lz.c"
	"tools/gbagfx/rl.c"
	"tools/gbagfx/util.c"
	"tools/gbagfx/font.c"
	"tools/gbagfx/huff.c"
)

# libpng
find_package(PNG REQUIRED)
target_include_directories(gbagfx PRIVATE ${PNG_INCLUDE_DIR})
target_link_libraries(gbagfx PRIVATE ${PNG_LIBRARY})

target_compile_definitions(gbagfx PRIVATE APPDIR="tools/gbagfx")
target_compile_definitions(gbagfx PRIVATE PROGRAMNAME="gbagfx")
set_target_properties(gbagfx PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")
target_compile_definitions(gbagfx PRIVATE PNG_SKIP_SETJMP_CHECK)

cmake_policy(SET CMP0065 NEW)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(gbagfx PRIVATE -Wall -Wextra -Werror -Wno-sign-compare)
endif()
