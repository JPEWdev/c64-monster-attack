cmake_minimum_required(VERSION 3.18)

set(LLVM_MOS_PLATFORM c64)
find_package(llvm-mos-sdk REQUIRED)

find_program(LLVM_OBJDUMP NAMES llvm-objdump REQUIRED)
find_program(LLVM_NM NAMES llvm-nm REQUIRED)
find_program(PYTHON3 NAMES python3 REQUIRED)

if (DEFINED ENV{X64SC})
    set(X64SC $ENV{X64SC})
else()
    find_program(X64SC NAMES x64sc REQUIRED)
endif()

if (DEFINED ENV{C1541})
    set(C1541 $ENV{C1541})
else()
    find_program(C1541 NAMES c1541 REQUIRED)
endif()

option(DEBUG_MODE "Enable visual debug mode" OFF)

project(monster-attack VERSION 0.0.1)
enable_language(C ASM)
set(D64_PROG_NAME "ma")

function(generate_sprite FN SOURCE_LIST)
    get_filename_component(D ${CMAKE_CURRENT_BINARY_DIR}/${FN}.S DIRECTORY)
    file(MAKE_DIRECTORY ${D})
    add_custom_command(
        COMMAND ${PYTHON3} ${CMAKE_SOURCE_DIR}/scripts/spm2code.py ${CMAKE_CURRENT_SOURCE_DIR}/${FN} ${CMAKE_CURRENT_BINARY_DIR}/${FN}.S --format=gas
        DEPENDS ${CMAKE_SOURCE_DIR}/scripts/spm2code.py ${FN}
        OUTPUT ${FN}.S
        COMMENT "Generating code for sprite ${FN}"
        VERBATIM
    )
    list(APPEND ${SOURCE_LIST} ${CMAKE_CURRENT_BINARY_DIR}/${FN}.S)
    set(${SOURCE_LIST} "${${SOURCE_LIST}}" PARENT_SCOPE)
endfunction()

function(generate_charset FN SOURCE_LIST SECTION)
    get_filename_component(D ${CMAKE_CURRENT_BINARY_DIR}/${FN}.S DIRECTORY)
    file(MAKE_DIRECTORY ${D})
    add_custom_command(
        COMMAND ${PYTHON3} ${CMAKE_SOURCE_DIR}/scripts/cst2code.py ${CMAKE_CURRENT_SOURCE_DIR}/${FN} ${CMAKE_CURRENT_BINARY_DIR}/${FN}.S --section=${SECTION}
        DEPENDS ${CMAKE_SOURCE_DIR}/scripts/cst2code.py ${FN}
        OUTPUT ${FN}.S
        COMMENT "Generating code for charset ${FN}"
        VERBATIM
    )
    list(APPEND ${SOURCE_LIST} ${CMAKE_CURRENT_BINARY_DIR}/${FN}.S)
    set(${SOURCE_LIST} "${${SOURCE_LIST}}" PARENT_SCOPE)
endfunction()

list(APPEND SOURCES
	src/draw.c
	src/input.c
    src/isr.c
    src/isr-handler.S
	src/main.c
	src/map.c
	src/mobs.c
	src/player-sprite.c
	src/screen.S
	src/sprite.c
    src/main.c
    src/player.c
    src/util.c
    src/move.c
)

list(APPEND SPRITES
    sprites/coin.spm
    sprites/heart.spm
    sprites/player_attack_east.spm
    sprites/player_attack_north.spm
    sprites/player_attack_south.spm
    sprites/player_attack_west.spm
    sprites/player_east.spm
    sprites/player_north.spm
    sprites/player_south.spm
    sprites/player_west.spm
    sprites/skeleton.spm
    sprites/sword_east.spm
    sprites/sword_north.spm
    sprites/sword_south.spm
    sprites/sword_west.spm
)

list(APPEND CHAR_SETS
    game_tiles.cst
)

foreach(S ${SPRITES})
    generate_sprite(${S} SOURCES)
endforeach()

foreach(C ${CHAR_SETS})
    generate_charset(${C} SOURCES video)
endforeach()

add_custom_command(
    COMMAND ${PYTHON3} ${CMAKE_SOURCE_DIR}/scripts/map2code.py ${CMAKE_CURRENT_SOURCE_DIR}/map.yaml ${CMAKE_CURRENT_BINARY_DIR}/map-data.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/map.yaml ${CMAKE_SOURCE_DIR}/scripts/map2code.py
    OUTPUT map-data.c
    VERBATIM
)
list(APPEND SOURCES map-data.c)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker-script.ld)

add_executable(${CMAKE_PROJECT_NAME}.prg ${SOURCES})
if (DEBUG_MODE)
    target_compile_definitions(${CMAKE_PROJECT_NAME}.prg PRIVATE DEBUG=1)
endif()
target_compile_definitions(${CMAKE_PROJECT_NAME}.prg PRIVATE VERSION="${CMAKE_PROJECT_VERSION}")
target_compile_options(${CMAKE_PROJECT_NAME}.prg PRIVATE -Wall -Werror)
target_include_directories(${CMAKE_PROJECT_NAME}.prg PRIVATE src)

set_target_properties(${CMAKE_PROJECT_NAME}.prg PROPERTIES
    LINK_FLAGS "-T ${LINKER_SCRIPT} -Wl,-Map,${CMAKE_PROJECT_NAME}.map"
    LINK_DEPENDS ${LINKER_SCRIPT}
)

add_custom_command(
    COMMAND ${LLVM_OBJDUMP} -S ${CMAKE_PROJECT_NAME}.prg.elf > ${CMAKE_PROJECT_NAME}.lst
    DEPENDS ${CMAKE_PROJECT_NAME}.prg
    OUTPUT ${CMAKE_PROJECT_NAME}.lst
    VERBATIM
)
add_custom_target(listfile ALL DEPENDS ${CMAKE_PROJECT_NAME}.lst)

add_custom_command(
    COMMAND ${C1541} -format ${CMAKE_PROJECT_NAME},1 d64 ${CMAKE_PROJECT_NAME}.d64 -attach ${CMAKE_PROJECT_NAME}.d64 -write ${CMAKE_PROJECT_NAME}.prg ${D64_PROG_NAME}
    DEPENDS ${CMAKE_PROJECT_NAME}.prg
    OUTPUT ${CMAKE_PROJECT_NAME}.d64
    VERBATIM
)
add_custom_target(d64 ALL DEPENDS ${CMAKE_PROJECT_NAME}.d64)

add_custom_command(
    COMMAND ${LLVM_NM} ${CMAKE_PROJECT_NAME}.prg.elf > ${CMAKE_PROJECT_NAME}.sym
    DEPENDS ${CMAKE_PROJECT_NAME}.prg
    OUTPUT ${CMAKE_PROJECT_NAME}.sym
    VERBATIM
)

add_custom_command(
    COMMAND ${PYTHON3} ${CMAKE_SOURCE_DIR}/scripts/syms2labels.py ${CMAKE_PROJECT_NAME}.sym ${CMAKE_PROJECT_NAME}-debug.ll --break main
    DEPENDS ${CMAKE_PROJECT_NAME}.sym ${CMAKE_SOURCE_DIR}/scripts/syms2labels.py
    OUTPUT ${CMAKE_PROJECT_NAME}-debug.ll
    VERBATIM
)

add_custom_command(
    COMMAND ${PYTHON3} ${CMAKE_SOURCE_DIR}/scripts/syms2labels.py ${CMAKE_PROJECT_NAME}.sym ${CMAKE_PROJECT_NAME}.ll
    DEPENDS ${CMAKE_PROJECT_NAME}.sym ${CMAKE_SOURCE_DIR}/scripts/syms2labels.py
    OUTPUT ${CMAKE_PROJECT_NAME}.ll
    VERBATIM
)

add_custom_target(debug
    COMMAND /bin/sh -c "exec ${X64SC} -nativemonitor -moncommands ${CMAKE_PROJECT_NAME}-debug.ll ${CMAKE_PROJECT_NAME}.prg"
    DEPENDS ${CMAKE_PROJECT_NAME}.prg ${CMAKE_PROJECT_NAME}-debug.ll
    USES_TERMINAL
    VERBATIM
)

add_custom_target(run
    COMMAND /bin/sh -c "exec ${X64SC} -nativemonitor -moncommands ${CMAKE_PROJECT_NAME}.ll ${CMAKE_PROJECT_NAME}.prg"
    DEPENDS ${CMAKE_PROJECT_NAME}.prg ${CMAKE_PROJECT_NAME}.ll
    USES_TERMINAL
    VERBATIM
)

