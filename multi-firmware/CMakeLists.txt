cmake_minimum_required(VERSION 3.13)

add_compile_definitions(PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

set(PICO_SDK_PATH "${CMAKE_SOURCE_DIR}/pico-sdk")
include(pico_sdk_import.cmake)

project(multi-firmware)

pico_sdk_init()
pico_init_picotool()
find_package (Python3 REQUIRED COMPONENTS Interpreter)

function(setup_relocated_pico_app SUBPROJECT_DIR TARGET FLASH_ORIGIN FLASH_LENGTH)
    set(GENERATED_LD_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/memmap_${TARGET}.ld")
    set(GENERATED_CMAKE_INJECT "${CMAKE_CURRENT_BINARY_DIR}/inject_${TARGET}.cmake")
    set(SUBPROJECT_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}")

    configure_file(
        "${CMAKE_SOURCE_DIR}/memmap.ld.in"
        "${GENERATED_LD_SCRIPT}"
        @ONLY
    )

    configure_file(
        "${CMAKE_SOURCE_DIR}/inject.cmake.in"
        "${GENERATED_CMAKE_INJECT}"
        @ONLY
    )

    add_custom_target(${TARGET}
        COMMAND ${CMAKE_COMMAND} -S ${SUBPROJECT_DIR} -B ${SUBPROJECT_BINARY_DIR} -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=${GENERATED_CMAKE_INJECT}
        COMMAND ${CMAKE_COMMAND} --build ${SUBPROJECT_BINARY_DIR} --target ${TARGET}
        DEPENDS ${GENERATED_LD_SCRIPT} ${GENERATED_CMAKE_INJECT}
        BYPRODUCTS "${SUBPROJECT_BINARY_DIR}/${TARGET}.bin"
        COMMENT "Building ${TARGET} from ${SUBPROJECT_DIR}..."
    )
endfunction()

add_executable(dispatcher
    dispatcher/dispatcher.c
)

target_link_libraries(dispatcher
    pico_stdlib
    cmsis_core
)

pico_add_extra_outputs(dispatcher)

# Memory map
set(APP1_OFFSET 0x10010000)
set(APP1_LENGTH 192k)
set(APP2_OFFSET 0x10040000)
set(APP2_LENGTH 64k)
set(APP3_OFFSET 0x10050000)
set(APP3_LENGTH 64k)
set(APP4_OFFSET 0x10060000)
set(APP4_LENGTH 64k)
set(APP5_OFFSET 0x10070000)
set(APP5_LENGTH 64k)
set(APP6_OFFSET 0x10080000)
set(APP6_LENGTH 64k)

target_compile_definitions(dispatcher PUBLIC APP1_START_ADDRESS=${APP1_OFFSET})
target_compile_definitions(dispatcher PUBLIC APP2_START_ADDRESS=${APP2_OFFSET})
target_compile_definitions(dispatcher PUBLIC APP3_START_ADDRESS=${APP3_OFFSET})
target_compile_definitions(dispatcher PUBLIC APP4_START_ADDRESS=${APP4_OFFSET})
target_compile_definitions(dispatcher PUBLIC APP5_START_ADDRESS=${APP5_OFFSET})
target_compile_definitions(dispatcher PUBLIC APP6_START_ADDRESS=${APP6_OFFSET})

setup_relocated_pico_app(${CMAKE_CURRENT_SOURCE_DIR}/hid-remapper/firmware remapper_serial ${APP1_OFFSET} ${APP1_LENGTH})
setup_relocated_pico_app(${CMAKE_CURRENT_SOURCE_DIR}/spaceball-2003 spaceball ${APP2_OFFSET} ${APP2_LENGTH})
setup_relocated_pico_app(${CMAKE_CURRENT_SOURCE_DIR}/magellan-spacemouse magellan ${APP3_OFFSET} ${APP3_LENGTH})
setup_relocated_pico_app(${CMAKE_CURRENT_SOURCE_DIR}/pico-uart-bridge uart_bridge ${APP4_OFFSET} ${APP4_LENGTH})
setup_relocated_pico_app(${CMAKE_CURRENT_SOURCE_DIR}/spaceorb360 spaceorb ${APP5_OFFSET} ${APP5_LENGTH})
setup_relocated_pico_app(${CMAKE_CURRENT_SOURCE_DIR}/spaceorb360 spaceorb_joystick ${APP6_OFFSET} ${APP6_LENGTH})

set(COMBINED_BIN_FILE "${CMAKE_BINARY_DIR}/multi-firmware.bin")
set(COMBINED_UF2_FILE "${CMAKE_BINARY_DIR}/multi-firmware.uf2")

add_custom_command(
    OUTPUT ${COMBINED_BIN_FILE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/combine.py
        --output ${COMBINED_BIN_FILE}
        --image dispatcher.bin@0x10000000
        --image remapper_serial/remapper_serial.bin@${APP1_OFFSET}
        --image spaceball/spaceball.bin@${APP2_OFFSET}
        --image magellan/magellan.bin@${APP3_OFFSET}
        --image uart_bridge/uart_bridge.bin@${APP4_OFFSET}
        --image spaceorb/spaceorb.bin@${APP5_OFFSET}
        --image spaceorb_joystick/spaceorb_joystick.bin@${APP6_OFFSET}
    DEPENDS
        dispatcher "${CMAKE_CURRENT_BINARY_DIR}/dispatcher.bin"
        remapper_serial "${CMAKE_CURRENT_BINARY_DIR}/remapper_serial/remapper_serial.bin"
        spaceball "${CMAKE_CURRENT_BINARY_DIR}/spaceball/spaceball.bin"
        magellan "${CMAKE_CURRENT_BINARY_DIR}/magellan/magellan.bin"
        uart_bridge "${CMAKE_CURRENT_BINARY_DIR}/uart_bridge/uart_bridge.bin"
        spaceorb "${CMAKE_CURRENT_BINARY_DIR}/spaceorb/spaceorb.bin"
        spaceorb_joystick "${CMAKE_CURRENT_BINARY_DIR}/spaceorb_joystick/spaceorb_joystick.bin"
    COMMENT "Combining application binaries..."
)

add_custom_command(
    OUTPUT ${COMBINED_UF2_FILE}
    COMMAND picotool uf2 convert ${COMBINED_BIN_FILE} ${COMBINED_UF2_FILE}
    DEPENDS ${COMBINED_BIN_FILE}
    COMMENT "Creating final UF2 file..."
)

add_custom_target(combined_image ALL
    DEPENDS ${COMBINED_UF2_FILE}
)
