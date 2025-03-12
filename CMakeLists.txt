cmake_minimum_required(VERSION 3.18)

set(LLVM_MOS_PLATFORM c64)
find_package(llvm-mos-sdk REQUIRED)

find_program(OBJDUMP NAMES llvm-objdump objdump REQUIRED)
find_program(OBJCOPY NAMES llvm-objcopy objcopy REQUIRED)
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

project(monster-attack VERSION 0.0.4)
enable_language(C ASM)
set(D64_PROG_NAME "ma")

function(generate_sprite FN SOURCE_LIST)
    get_filename_component(D ${CMAKE_CURRENT_BINARY_DIR}/${FN}.S DIRECTORY)
    file(MAKE_DIRECTORY ${D})
    add_custom_command(
        COMMAND ${PYTHON3} ${CMAKE_SOURCE_DIR}/scripts/spm2code.py ${CMAKE_CURRENT_SOURCE_DIR}/${FN} ${CMAKE_CURRENT_BINARY_DIR}/${FN}.c ${CMAKE_CURRENT_BINARY_DIR}/${FN}.h
        DEPENDS ${CMAKE_SOURCE_DIR}/scripts/spm2code.py ${FN}
        OUTPUT ${FN}.c ${FN}.h
        COMMENT "Generating code for sprite ${FN}"
        VERBATIM
    )
    list(APPEND ${SOURCE_LIST} ${CMAKE_CURRENT_BINARY_DIR}/${FN}.c)
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
    src/bcd.c
    src/draw.c
    src/input.c
    src/main.c
    src/map.c
    src/mobs.c
    src/player-sprite.c
    src/screen.S
    src/sprite.c
    src/store.c
    src/isr-handler.S
    src/isr.c
    src/main.c
    src/mobs/arrow.c
    src/mobs/coin.c
    src/mobs/heart.c
    src/mobs/skeleton.c
    src/mobs/skeleton_archer.c
    src/move.c
    src/player.c
    src/util.c
    src/tick.c
)

list(APPEND SPRITES
    sprites/arrow_east.spm
    sprites/arrow_north.spm
    sprites/arrow_south.spm
    sprites/arrow_west.spm
    sprites/bow_east.spm
    sprites/bow_north.spm
    sprites/bow_south.spm
    sprites/bow_west.spm
    sprites/coin.spm
    sprites/flail.spm
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
    sprites/skeleton_archer.spm
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

add_custom_command(
    COMMAND ${PYTHON3} ${CMAKE_SOURCE_DIR}/scripts/trigconst.py ${CMAKE_CURRENT_BINARY_DIR}/trigconst.h
    DEPENDS ${CMAKE_SOURCE_DIR}/scripts/trigconst.py
    OUTPUT trigconst.h
    VERBATIM
)
list(APPEND SOURCES trigconst.h)

set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker-script.ld)

set(PROG_OUTPUT ${CMAKE_PROJECT_NAME}.prg)
add_executable(${PROG_OUTPUT} ${SOURCES})
if (DEBUG_MODE)
    target_compile_definitions(${PROG_OUTPUT} PRIVATE DEBUG=1)
endif()
target_compile_definitions(${PROG_OUTPUT} PRIVATE VERSION="${CMAKE_PROJECT_VERSION}")
target_compile_options(${PROG_OUTPUT} PRIVATE -Wall -Werror -fnonreentrant)
target_include_directories(${PROG_OUTPUT} PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}/sprites ${CMAKE_CURRENT_BINARY_DIR})

set_target_properties(${PROG_OUTPUT} PROPERTIES
    LINK_FLAGS "-T ${LINKER_SCRIPT} -Wl,-Map,${CMAKE_PROJECT_NAME}.map"
    LINK_DEPENDS ${LINKER_SCRIPT}
)

add_custom_command(
    COMMAND ${OBJCOPY} -O binary --only-section=.video ${PROG_OUTPUT}.elf graphics.seq
    DEPENDS ${PROG_OUTPUT}
    OUTPUT graphics.seq
    VERBATIM
)

add_custom_command(
    COMMAND ${OBJDUMP} -S ${PROG_OUTPUT}.elf > ${CMAKE_PROJECT_NAME}.lst
    DEPENDS ${PROG_OUTPUT}
    OUTPUT ${CMAKE_PROJECT_NAME}.lst
    VERBATIM
)
add_custom_target(listfile ALL DEPENDS ${CMAKE_PROJECT_NAME}.lst)

configure_file(${CMAKE_SOURCE_DIR}/scripts/disk.txt.in disk.txt)
add_custom_command(
    COMMAND ${C1541} < disk.txt
    DEPENDS disk.txt ${PROG_OUTPUT} graphics.seq
    OUTPUT ${CMAKE_PROJECT_NAME}.d64
    VERBATIM
)
add_custom_target(d64 ALL DEPENDS ${CMAKE_PROJECT_NAME}.d64)

add_custom_command(
    COMMAND ${LLVM_NM} ${PROG_OUTPUT}.elf > ${CMAKE_PROJECT_NAME}.sym
    DEPENDS ${PROG_OUTPUT}
    OUTPUT ${CMAKE_PROJECT_NAME}.sym
    VERBATIM
)

add_custom_command(
    COMMAND ${PYTHON3} ${CMAKE_SOURCE_DIR}/scripts/syms2labels.py ${CMAKE_PROJECT_NAME}.sym ${CMAKE_PROJECT_NAME}-debug.ll --break _start
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
    COMMAND /bin/sh -c "exec ${X64SC} -nativemonitor -moncommands ${CMAKE_PROJECT_NAME}-debug.ll ${CMAKE_PROJECT_NAME}.d64"
    DEPENDS ${CMAKE_PROJECT_NAME}.d64 ${CMAKE_PROJECT_NAME}-debug.ll
    USES_TERMINAL
    VERBATIM
)

add_custom_target(run
    COMMAND /bin/sh -c "exec ${X64SC} -nativemonitor -moncommands ${CMAKE_PROJECT_NAME}.ll ${CMAKE_PROJECT_NAME}.d64"
    DEPENDS ${CMAKE_PROJECT_NAME}.d64 ${CMAKE_PROJECT_NAME}.ll
    USES_TERMINAL
    VERBATIM
)

