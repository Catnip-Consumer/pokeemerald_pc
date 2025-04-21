
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)

add_executable(mid2agb
	"tools/mid2agb/agb.cpp"
	"tools/mid2agb/error.cpp"
	"tools/mid2agb/main.cpp"
	"tools/mid2agb/midi.cpp"
	"tools/mid2agb/tables.cpp"
)

target_compile_definitions(mid2agb PRIVATE APPDIR="tools/mid2agb")
target_compile_definitions(mid2agb PRIVATE PROGRAMNAME="mid2agb")
set_target_properties(mid2agb PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tools")
set(MID2AGB $<TARGET_FILE:mid2agb>)

cmake_policy(SET CMP0065 NEW)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(mid2agb PRIVATE -Wall -Wno-switch -Werror)
endif()
