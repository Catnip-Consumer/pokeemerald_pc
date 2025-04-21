
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

add_executable(mapjson
	"tools/mapjson/json11.cpp"
	"tools/mapjson/mapjson.cpp"
)

target_compile_definitions(mapjson PRIVATE APPDIR="tools/mapjson")
target_compile_definitions(mapjson PRIVATE PROGRAMNAME="mapjson")
set_target_properties(mapjson PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")

cmake_policy(SET CMP0065 NEW)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(mapjson PRIVATE -Wall)
endif()
