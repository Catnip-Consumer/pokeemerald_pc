set(SOUND_SUBDIR "${CMAKE_SOURCE_DIR}/sound/songs")
set(SOUND_BUILDDIR "${CMAKE_BINARY_DIR}/sound/songs")

set(MID_SUBDIR "${SOUND_SUBDIR}/midi")
set(MID_BUILDDIR "${SOUND_BUILDDIR}/midi")
set(MID_CFG_PATH "${MID_SUBDIR}/midi.cfg")

# Function to remove a prefix from a string if it exists
function(remove_prefix INPUT PREFIX OUTPUT)
	string(FIND "${INPUT}" "${PREFIX}" PREFIX_INDEX)

	if(PREFIX_INDEX EQUAL 0)
		# Remove the prefix
		string(LENGTH "${PREFIX}" PREFIX_LENGTH)
		string(SUBSTRING "${INPUT}" ${PREFIX_LENGTH} -1 RESULT)
	else()
		# Keep the original string
		set(RESULT "${INPUT}")
	endif()

	# Return the result
	set(${OUTPUT} "${RESULT}" PARENT_SCOPE)
endfunction()

# Assembler options
set(AS_OPTS
	--32 --defsym MODERN=1 --defsym PORTABLE=1 --defsym UBFIX=1
)

# Read all .s sound files
file(GLOB SND_ASM_FILES "${SOUND_SUBDIR}/*.s")

# Read the midi.cfg file
file(READ "${MID_CFG_PATH}" MIDI_CFG_CONTENTS)
string(REPLACE "\n" ";" MIDI_CFG_LINES "${MIDI_CFG_CONTENTS}")

# Process each line
foreach(LINE IN LISTS MIDI_CFG_LINES)
	# Skip empty lines
	if(LINE STREQUAL "")
		continue()
	endif()

	# Extract the .mid file and options
	string(FIND "${LINE}" ":" COLON_INDEX)
	if(COLON_INDEX EQUAL -1)
		message(FATAL_ERROR "Invalid line in midi.cfg: ${LINE}")
	endif()

	string(SUBSTRING "${LINE}" 0 ${COLON_INDEX} MID_FILE)
	math(EXPR COLON_INDEX "${COLON_INDEX} + 1")
	string(SUBSTRING "${LINE}" ${COLON_INDEX} -1 OPTIONS)
	string(STRIP "${OPTIONS}" OPTIONS)

	# Create .o and .s files from .mid file
	set(MID_ASM_FILE "${MID_BUILDDIR}/${MID_FILE}.s")
	set(MID_FILE "${MID_SUBDIR}/${MID_FILE}")

	# Ensure the output directory exists
	cmake_path(GET MID_ASM_FILE PARENT_PATH PARENT_DIR)
	make_directory(${PARENT_DIR})

	# Add a custom command to process the .mid file
	add_custom_command(
		OUTPUT "${MID_ASM_FILE}"
		COMMAND ${MID2AGB} "${MID_FILE}" "${MID_ASM_FILE}" ${OPTIONS} > ${NULL_DEVICE}
		DEPENDS "${MID_FILE}" "${MID_CFG_PATH}"
		COMMENT "Processing ${MID_FILE} into ${MID_ASM_FILE} with options: ${OPTIONS}"
	)

	# Add the output file to the list of generated files
	list(APPEND SND_ASM_FILES "${MID_ASM_FILE}")
endforeach()

# Assemble all .s files into .o files
foreach(SOURCE_FILE IN LISTS SND_ASM_FILES)
	# Silly hack to remove the build/ directory for midi files
	remove_prefix("${SOURCE_FILE}" "${CMAKE_BINARY_DIR}" OUT_FILE)
	remove_prefix("${OUT_FILE}" "${CMAKE_SOURCE_DIR}" OUT_FILE)

	set(OUT_FILE_TEMP "${CMAKE_BINARY_DIR}/${OUT_FILE}.tmp.s")
	set(OUT_FILE "${CMAKE_BINARY_DIR}/${OUT_FILE}.o")

	# Ensure the output directory exists
	cmake_path(GET OUT_FILE PARENT_PATH PARENT_DIR)
	make_directory(${PARENT_DIR})

	# Add a custom command to assemble the .s file into a .o file
	add_custom_command(
		OUTPUT "${OUT_FILE}"
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/compile/replace-text.cmake
				"${SOURCE_FILE}" "${OUT_FILE_TEMP}"
				".2byte" ".short"
				".4byte" ".int"
		COMMAND ${AS} ${AS_OPTS} -I sound -o "${OUT_FILE}" "${OUT_FILE_TEMP}"
		COMMAND ${OBJCOPY} --prefix-symbol _ ${OUT_FILE}
		DEPENDS "${SOURCE_FILE}"
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMENT "Assembling ${SOURCE_FILE} into ${OUT_FILE}"
	)

	# Add the .o file to the list of object files
	list(APPEND SND_OBJ_FILES "${OUT_FILE}")
endforeach()

# Function to convert .aif files to object files
function(aif_convert OUTPUT_VAR EXTRA_FLAGS SOURCE_FILES)
	set(OBJECT_FILES)

	foreach(SOURCE_FILE IN LISTS SOURCE_FILES)
		remove_prefix("${SOURCE_FILE}" "${CMAKE_SOURCE_DIR}" OUTPUT_OBJ)
		set(OUTPUT_OBJ "${CMAKE_BINARY_DIR}/${OUTPUT_OBJ}.bin")

		# Ensure the output directory exists
		cmake_path(GET OUTPUT_OBJ PARENT_PATH PARENT_DIR)
		make_directory(${PARENT_DIR})

		# Create the command to process the file
		add_custom_command(
			OUTPUT ${OUTPUT_OBJ}
			COMMAND ${AIF2PCM} ${SOURCE_FILE} ${OUTPUT_OBJ} ${EXTRA_FLAGS}
			DEPENDS ${SOURCE_FILE}
			COMMENT "Convert ${SOURCE_FILE} to ${OUTPUT_OBJ}"
		)

		# Append the generated object file to the list
		list(APPEND OBJECT_FILES ${OUTPUT_OBJ})
	endforeach()

	# Set the output variable to the list of object files
	set(${OUTPUT_VAR} ${OBJECT_FILES} PARENT_SCOPE)
endfunction()

# Convert Pokemon cries to object files
file(GLOB CRIES "${CMAKE_SOURCE_DIR}/sound/direct_sound_samples/cries/*.aif")
aif_convert(CRIES_OBJ "--compress" "${CRIES}")

# Convert samples to object files
file(GLOB SAMPLES "${CMAKE_SOURCE_DIR}/sound/direct_sound_samples/*.aif")
aif_convert(SAMPLES_OBJ "" "${SAMPLES}")

# Convert samples to object files
file(GLOB PHONEMES "${CMAKE_SOURCE_DIR}/sound/direct_sound_samples/phonemes/*.aif")
aif_convert(PHONEMES_OBJ "" "${PHONEMES}")

# Add a custom target to build all sound files
add_custom_target(emerald-sound-files ALL DEPENDS ${SND_ASM_FILES} ${CRIES_OBJ} ${SAMPLES_OBJ} ${PHONEMES_OBJ})
