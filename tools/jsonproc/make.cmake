
set(CMAKE_CXX_STANDARD 17)

add_executable(jsonproc
	"tools/jsonproc/jsonproc.cpp"
)

target_compile_definitions(jsonproc PRIVATE APPDIR="tools/jsonproc")
target_compile_definitions(jsonproc PRIVATE PROGRAMNAME="jsonproc")
set_target_properties(jsonproc PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")
set(JSONPROC $<TARGET_FILE:jsonproc>)

target_include_directories(jsonproc PRIVATE ${CMAKE_SOURCE_DIR}/extern/json/include)
cmake_policy(SET CMP0065 NEW)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(jsonproc PRIVATE -Wall)
endif()
