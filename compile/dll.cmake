cmake_minimum_required(VERSION 3.18)

project(PokeEmerald-dll
	VERSION 0.0.0
	LANGUAGES C CXX
)

# requires at least C99/C++17-compatible compiler to compile the code
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# ROM information
add_compile_definitions(TITLE=POKEMON EMER)
add_compile_definitions(GAME_CODE=BPEE)
add_compile_definitions(MAKER_CODE=01)
add_compile_definitions(REVISION=0)

# Compiler toggle flags
set(COMPILE_DEFS
	-DRENDERER_EASY_DRAW
	-DNONMATCHING
	-DMODERN=1
	-DPORTABLE=1
	-DUBFIX=1
	-DIS_DLL
)

# Preprocessor options
set(CPP_OPTS
	-iquote include -Wno-trigraphs
	-D NONMATCHING -D PORTABLE -D RENDERER_EASY_DRAW -D MODERN=1 -D UBFIX -D IS_DLL
)

# Compiler options
set(COMPILE_OPTS
	-Wformat -Wformat-security -fomit-frame-pointer
	-Wno-trigraphs -Wimplicit -Wno-int-conversion -Wparentheses -Wunused
	-fleading-underscore -fno-dce -fno-builtin -Wno-unused-function
	-mmmx -msse -msse2 -mfxsr
	-m32 -std=gnu99 -O3
)

# Assembler options
set(AS_OPTS
	--32 --defsym MODERN=1 --defsym PORTABLE=1 --defsym UBFIX=1
)

# Add `DEBUG` macro definition if compiling under Debug or RelWithDebInfo
# configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_definitions(DEBUG)
endif()

cmake_policy(SET CMP0065 NEW)

# Command to add preproc to build process
function(asm_src_preprocess OUTPUT_VAR OUT_DIR SRC_DIR SOURCE_FILES)
	set(OBJECT_FILES)

	foreach(SOURCE_FILE IN LISTS SOURCE_FILES)
		set(OUT_DIR_FULL "${CMAKE_BINARY_DIR}/${OUT_DIR}")
		set(INPUT_SRC "${CMAKE_SOURCE_DIR}/${SRC_DIR}/${SOURCE_FILE}")

		# Construct paths for intermediate and output files
		set(INTERMEDIATE_P1	"${OUT_DIR_FULL}/${SOURCE_FILE}-a.s")
		set(INTERMEDIATE_PP	"${OUT_DIR_FULL}/${SOURCE_FILE}-b.s")
		set(INTERMEDIATE_P2	"${OUT_DIR_FULL}/${SOURCE_FILE}-c.s")
		set(OUTPUT_OBJ		"${OUT_DIR_FULL}/${SOURCE_FILE}.o")

		# Ensure the output directory exists
		cmake_path(GET OUTPUT_OBJ PARENT_PATH PARENT_DIR)
		make_directory(${PARENT_DIR})

		# Add a custom command for preprocessing, transformation, and assembly.
		# This is a level of jank that has me worried for my sanity
		add_custom_command(
			OUTPUT ${OUTPUT_OBJ}
			COMMAND ${PREPROC} ${INPUT_SRC} charmap.txt > ${INTERMEDIATE_P1}
			COMMAND ${CPP} -I include ${INTERMEDIATE_P1} -o ${INTERMEDIATE_PP}
			COMMAND ${PREPROC} -e ${INTERMEDIATE_PP} charmap.txt > ${INTERMEDIATE_P2}
			COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/compile/replace-text.cmake
					"${INTERMEDIATE_P2}" "${INTERMEDIATE_P2}"
					".2byte" ".short"
					".4byte" ".int"
			COMMAND ${AS} ${AS_OPTS} -o ${OUTPUT_OBJ} ${INTERMEDIATE_P2}
			COMMAND ${OBJCOPY} --prefix-symbol _ ${OUTPUT_OBJ}
			DEPENDS ${INPUT_SRC}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			COMMENT "Processing ${INPUT_SRC} to generate ${OUTPUT_OBJ}"
		)

		# Append the generated object file to the list
		list(APPEND OBJECT_FILES ${OUTPUT_OBJ})
	endforeach()

	# Set the output variable to the list of object files
	set(${OUTPUT_VAR} ${OBJECT_FILES} PARENT_SCOPE)
endfunction()

# Command to add preproc to build process
function(c_src_preprocess OUTPUT_VAR OUT_DIR SRC_DIR SOURCE_FILES)
	set(OBJECT_FILES)

	foreach(SOURCE_FILE IN LISTS SOURCE_FILES)
		set(OUT_DIR_FULL "${CMAKE_BINARY_DIR}/${OUT_DIR}")
		set(INPUT_SRC "${CMAKE_SOURCE_DIR}/${SRC_DIR}/${SOURCE_FILE}")

		# Construct paths for intermediate and output files
		set(INTERMEDIATE_I	"${OUT_DIR_FULL}/${SOURCE_FILE}-a.i")
		set(INTERMEDIATE_P	"${OUT_DIR_FULL}/${SOURCE_FILE}-b.i")
		set(INTERMEDIATE_S	"${OUT_DIR_FULL}/${SOURCE_FILE}.s")
		set(OUTPUT_OBJ		"${OUT_DIR_FULL}/${SOURCE_FILE}.o")

		# Ensure the output directory exists
		cmake_path(GET OUTPUT_OBJ PARENT_PATH PARENT_DIR)
		make_directory(${PARENT_DIR})

		# Add a custom command for preprocessing, transformation, and assembly.
		# This is a level of jank that has me worried for my sanity
		add_custom_command(
			OUTPUT ${OUTPUT_OBJ}
			COMMAND ${CPP} ${CPP_OPTS} ${INPUT_SRC} -o ${INTERMEDIATE_I}
			COMMAND ${PREPROC} ${INTERMEDIATE_I} charmap.txt > ${INTERMEDIATE_P}
			COMMAND ${GCC} -S ${COMPILE_DEFS} ${COMPILE_OPTS} ${INTERMEDIATE_P} -o ${INTERMEDIATE_S}
			COMMAND ${AS} ${AS_OPTS} -o ${OUTPUT_OBJ} ${INTERMEDIATE_S}
			DEPENDS ${INPUT_SRC}
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			COMMENT "Processing ${INPUT_SRC} to generate ${OUTPUT_OBJ}"
		)

		# Append the generated object file to the list
		list(APPEND OBJECT_FILES ${OUTPUT_OBJ})
	endforeach()

	# Set the output variable to the list of object files
	set(${OUTPUT_VAR} ${OBJECT_FILES} PARENT_SCOPE)
endfunction()

# Source files (data/ directory)
set(PokeEmerald_DATA_ASM_SOURCES
	"battle_ai_scripts.s"
	"battle_anim_scripts.s"
	"battle_scripts_1.s"
	"battle_scripts_2.s"
	"contest_ai_scripts.s"
	"event_scripts.s"
	"field_effect_scripts.s"
	"map_events.s"
	"maps.s"
	"multiboot_berry_glitch_fix.s"
	"multiboot_ereader.s"
	"multiboot_pokemon_colosseum.s"
	"mystery_event_script_cmd_table.s"
	"mystery_gift.s"
	"sound_data.s"
)

# Source files (src/ directory)
set(PokeEmerald_SOURCES
	"AgbRfu_LinkManager.c"
	"agb_flash.c"
	"agb_flash_1m.c"
	"agb_flash_dummy.c"
	"agb_flash_le.c"
	"agb_flash_mx.c"
	"anim_mon_front_pics.c"
	"apprentice.c"
	"bard_music.c"
	"battle_ai_script_commands.c"
	"battle_ai_switch_items.c"
	"battle_anim.c"
	"battle_anim_bug.c"
	"battle_anim_dark.c"
	"battle_anim_dragon.c"
	"battle_anim_effects_1.c"
	"battle_anim_effects_2.c"
	"battle_anim_effects_3.c"
	"battle_anim_electric.c"
	"battle_anim_fight.c"
	"battle_anim_fire.c"
	"battle_anim_flying.c"
	"battle_anim_ghost.c"
	"battle_anim_ground.c"
	"battle_anim_ice.c"
	"battle_anim_mons.c"
	"battle_anim_mon_movement.c"
	"battle_anim_normal.c"
	"battle_anim_poison.c"
	"battle_anim_psychic.c"
	"battle_anim_rock.c"
	"battle_anim_smokescreen.c"
	"battle_anim_sound_tasks.c"
	"battle_anim_status_effects.c"
	"battle_anim_throw.c"
	"battle_anim_utility_funcs.c"
	"battle_anim_water.c"
	"battle_arena.c"
	"battle_bg.c"
	"battle_controllers.c"
	"battle_controller_link_opponent.c"
	"battle_controller_link_partner.c"
	"battle_controller_opponent.c"
	"battle_controller_player.c"
	"battle_controller_player_partner.c"
	"battle_controller_recorded_opponent.c"
	"battle_controller_recorded_player.c"
	"battle_controller_safari.c"
	"battle_controller_wally.c"
	"battle_dome.c"
	"battle_factory.c"
	"battle_factory_screen.c"
	"battle_gfx_sfx_util.c"
	"battle_interface.c"
	"battle_intro.c"
	"battle_main.c"
	"battle_message.c"
	"battle_palace.c"
	"battle_pike.c"
	"battle_pyramid.c"
	"battle_pyramid_bag.c"
	"battle_records.c"
	"battle_script_commands.c"
	"battle_setup.c"
	"battle_tent.c"
	"battle_tower.c"
	"battle_transition.c"
	"battle_transition_frontier.c"
	"battle_tv.c"
	"battle_util.c"
	"battle_util2.c"
	"berry.c"
	"berry_blender.c"
	"berry_crush.c"
	"berry_fix_graphics.c"
	"berry_fix_program.c"
	"berry_powder.c"
	"berry_tag_screen.c"
	"bg.c"
	"bike.c"
	"birch_pc.c"
	"blit.c"
	"braille.c"
	"braille_puzzles.c"
	"cable_car.c"
	"cable_club.c"
	"clear_save_data_screen.c"
	"clock.c"
	"coins.c"
	"confetti_util.c"
	"contest.c"
	"contest_ai.c"
	"contest_effect.c"
	"contest_link.c"
	"contest_link_util.c"
	"contest_painting.c"
	"contest_util.c"
	"coord_event_weather.c"
	"credits.c"
	"data.c"
	"daycare.c"
	"decompress.c"
	"decoration.c"
	"decoration_inventory.c"
	"dewford_trend.c"
	"digit_obj_util.c"
	"diploma.c"
	"dma3_manager.c"
	"dodrio_berry_picking.c"
	"dynamic_placeholder_text_util.c"
	"easy_chat.c"
	"egg_hatch.c"
	"ereader_helpers.c"
	"ereader_screen.c"
	"event_data.c"
	"event_object_lock.c"
	"event_object_movement.c"
	"evolution_graphics.c"
	"evolution_scene.c"
	"faraway_island.c"
	"fieldmap.c"
	"field_camera.c"
	"field_control_avatar.c"
	"field_door.c"
	"field_effect.c"
	"field_effect_helpers.c"
	"field_message_box.c"
	"field_player_avatar.c"
	"field_poison.c"
	"field_region_map.c"
	"field_screen_effect.c"
	"field_specials.c"
	"field_special_scene.c"
	"field_tasks.c"
	"field_weather.c"
	"field_weather_effect.c"
	"fldeff_cut.c"
	"fldeff_dig.c"
	"fldeff_escalator.c"
	"fldeff_flash.c"
	"fldeff_misc.c"
	"fldeff_rocksmash.c"
	"fldeff_softboiled.c"
	"fldeff_strength.c"
	"fldeff_sweetscent.c"
	"fldeff_teleport.c"
	"fonts.c"
	"frontier_pass.c"
	"frontier_util.c"
	"gpu_regs.c"
	"graphics.c"
	"gym_leader_rematch.c"
	"hall_of_fame.c"
	"heal_location.c"
	"hof_pc.c"
	"image_processing_effects.c"
	"international_string_util.c"
	"intro.c"
	"intro_credits_graphics.c"
	"io_reg.c"
	"item.c"
	"item_icon.c"
	"item_menu.c"
	"item_menu_icons.c"
	"item_use.c"
	"landmark.c"
	"libisagbprn.c"
	"librfu_intr.c"
	"librfu_rfu.c"
	"librfu_sio32id.c"
	"librfu_stwi.c"
	"lilycove_lady.c"
	"link.c"
	"link_rfu_2.c"
	"link_rfu_3.c"
	"list_menu.c"
	"load_save.c"
	"lottery_corner.c"
	"m4a.c"
	"m4a_tables.c"
	"mail.c"
	"mail_data.c"
	"main.c"
	"main_menu.c"
	"malloc.c"
	"map_name_popup.c"
	"match_call.c"
	"math_util.c"
	"mauville_old_man.c"
	"menu.c"
	"menu_helpers.c"
	"menu_specialized.c"
	"metatile_behavior.c"
	"minigame_countdown.c"
	"mini_printf.c"
	"mirage_tower.c"
	"money.c"
	"mon_markings.c"
	"move_relearner.c"
	"multiboot.c"
	"music_player.c"
	"mystery_event_menu.c"
	"mystery_event_msg.c"
	"mystery_event_script.c"
	"mystery_gift.c"
	"mystery_gift_client.c"
	"mystery_gift_link.c"
	"mystery_gift_menu.c"
	"mystery_gift_scripts.c"
	"mystery_gift_server.c"
	"mystery_gift_view.c"
	"naming_screen.c"
	"new_game.c"
	"option_menu.c"
	"overworld.c"
	"palette.c"
	"palette_util.c"
	"party_menu.c"
	"player_pc.c"
	"play_time.c"
	"pokeball.c"
	"pokeblock.c"
	"pokeblock_feed.c"
	"pokedex.c"
	"pokedex_area_region_map.c"
	"pokedex_area_screen.c"
	"pokedex_cry_screen.c"
	"pokemon.c"
	"pokemon_animation.c"
	"pokemon_icon.c"
	"pokemon_jump.c"
	"pokemon_size_record.c"
	"pokemon_storage_system.c"
	"pokemon_summary_screen.c"
	"pokenav.c"
	"pokenav_conditions.c"
	"pokenav_conditions_gfx.c"
	"pokenav_conditions_search_results.c"
	"pokenav_list.c"
	"pokenav_main_menu.c"
	"pokenav_match_call_data.c"
	"pokenav_match_call_gfx.c"
	"pokenav_match_call_list.c"
	"pokenav_menu_handler.c"
	"pokenav_menu_handler_gfx.c"
	"pokenav_region_map.c"
	"pokenav_ribbons_list.c"
	"pokenav_ribbons_summary.c"
	"post_battle_event_funcs.c"
	"random.c"
	"rayquaza_scene.c"
	"recorded_battle.c"
	"record_mixing.c"
	"region_map.c"
	"reload_save.c"
	"reset_rtc_screen.c"
	"reshow_battle_screen.c"
	"roamer.c"
	"rom_header_gf.c"
	"rotating_gate.c"
	"rotating_tile_puzzle.c"
	"roulette.c"
	"rtc.c"
	"safari_zone.c"
	"save.c"
	"save_failed_screen.c"
	"save_location.c"
	"scanline_effect.c"
	"scrcmd.c"
	"script.c"
	"script_menu.c"
	"script_movement.c"
	"script_pokemon_util.c"
	"secret_base.c"
	"shop.c"
	"siirtc.c"
	"slot_machine.c"
	"sound.c"
	"sound_mixer.c"
	"sprite.c"
	"starter_choose.c"
	"start_menu.c"
	"strings.c"
	"string_util.c"
	"stub.c"
	"task.c"
	"text.c"
	"text_input_strings.c"
	"text_window.c"
	"tilesets.c"
	"tileset_anims.c"
	"time_events.c"
	"title_screen.c"
	"trade.c"
	"trader.c"
	"trainer_card.c"
	"trainer_hill.c"
	"trainer_pokemon_sprites.c"
	"trainer_see.c"
	"trig.c"
	"tv.c"
	"union_room.c"
	"union_room_battle.c"
	"union_room_chat.c"
	"union_room_player_avatar.c"
	"use_pokeblock.c"
	"util.c"
	"walda_phrase.c"
	"wallclock.c"
	"wild_encounter.c"
	"window.c"
	"wireless_communication_status_screen.c"
	"wonder_news.c"
)

list(APPEND PokeEmerald_SOURCES
	"platform/bios.c"
	"platform/cgb_audio.c"
	"platform/dma.c"
	"platform/nostd.c"
	"platform/nostd.c"
	"platform/dll.c"
)

c_src_preprocess(PokeEmerald_Proc_SOURCES preproc/src src "${PokeEmerald_SOURCES}")
asm_src_preprocess(PokeEmerald_Proc_DATA_ASM_SOURCES preproc/data data "${PokeEmerald_DATA_ASM_SOURCES}")

# DLL target
add_library(emerald SHARED
	${PokeEmerald_Proc_SOURCES}
	${PokeEmerald_Proc_DATA_ASM_SOURCES}
	${SND_OBJ_FILES}
)

set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")
set_target_properties(emerald PROPERTIES LINKER_LANGUAGE C)

# Build flags
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(emerald PRIVATE -Wreturn-type -Werror=return-type -Wformat -Wformat-security -fomit-frame-pointer)
	target_compile_options(emerald PRIVATE -Wno-trigraphs -Wimplicit -Wno-int-conversion -Wparentheses -Wunused)
	target_compile_options(emerald PRIVATE -fleading-underscore -fno-dce -fno-builtin -Wno-unused-function)

	if(WIN32)
		# handle windows-specific options
		link_libraries(-static gcc stdc++ winpthread)

		if(CMAKE_BUILD_TYPE STREQUAL "Debug")
			target_compile_options(emerald PRIVATE -mconsole)
		endif()
	endif()

	# strip all symbol info on gnu on release builds
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		target_compile_options(emerald PRIVATE -fdata-sections -ffunction-sections -flto)
		target_link_options(emerald PRIVATE -Wl,--gc-sections -Wl,-flto -Wl,--strip-all)
	endif()
endif()

# Include dirs
target_include_directories(emerald PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(emerald PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Extra depencies
add_dependencies(emerald emerald-sound-files)
