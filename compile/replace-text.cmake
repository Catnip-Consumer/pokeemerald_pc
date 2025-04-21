cmake_minimum_required(VERSION 3.18)
# Arguments: input_file, output_file, search1, replace1, search2, replace2, ...

# Read the input file
file(READ "${CMAKE_ARGV3}" CONTENTS)

# Perform the replacements
# I swear this should be cleaner, but I can't figure out how to make it work.
math(EXPR CNT "(${CMAKE_ARGC} - 5) / 2 - 1")
foreach(INDEX RANGE ${CNT})
	math(EXPR SEARCH "(${INDEX} * 2) + 5")
	set(SEARCH "${CMAKE_ARGV${SEARCH}}")

	math(EXPR REPLACE "(${INDEX} * 2) + 6")
	set(REPLACE "${CMAKE_ARGV${REPLACE}}")

	string(REPLACE "${SEARCH}" "${REPLACE}" CONTENTS "${CONTENTS}")
endforeach()

# Write the modified contents to the output file
file(WRITE "${CMAKE_ARGV4}" "${CONTENTS}")
