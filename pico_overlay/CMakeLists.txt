pico_add_library(pico_overlay)

target_sources(pico_overlay INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/overlay.c
)

target_include_directories(pico_overlay_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

pico_mirrored_target_link_libraries(pico_overlay INTERFACE
        pico_sha256
        pico_binary_info
)

# needed by functions below
set(PICO_OVERLAY_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

function(pico_add_overlay TARGET NAME)
    set_property(TARGET ${TARGET} PROPERTY OVERLAYS ${NAME} APPEND)
endfunction()


function(pico_link_overlay_library TARGET NAME)
    include(${PICO_OVERLAY_DIR}/gather_vars.cmake)

    if (ARGC GREATER 2)
        message(STATUS "Linking overlay library ${NAME} with ${ARGV2}")
        set(OVERLAY_NAME ${ARGV2})
    else()
        set(OVERLAY_NAME ${NAME})
    endif()

    get_property(target_type TARGET ${NAME} PROPERTY TYPE)
    if ("INTERFACE_LIBRARY" STREQUAL "${target_type}")
        set(NEW_NAME ${NAME}_static)
        message(STATUS "Creating static library for overlay from interface library ${NAME}")
        # Create a static library from the interface library
        add_library(${NEW_NAME} STATIC ${PICO_OVERLAY_DIR}/static_dummy.c)
        target_link_libraries(${NEW_NAME} PUBLIC ${NAME})

        # Compile definitions
        gather_vars(COMPILE_DEFINITIONS INTERFACE_COMPILE_DEFINITIONS _touched_cd ${NEW_NAME})
        list(REMOVE_DUPLICATES COMPILE_DEFINITIONS)
        target_compile_definitions(${NEW_NAME} PUBLIC ${COMPILE_DEFINITIONS})
        target_compile_definitions(${NEW_NAME} INTERFACE ${COMPILE_DEFINITIONS})
        message(STATUS "Done compile definitions")

        # Compile options
        gather_vars(COMPILE_OPTIONS INTERFACE_COMPILE_OPTIONS _touched_cf ${NEW_NAME})
        list(REMOVE_DUPLICATES COMPILE_OPTIONS)
        target_compile_options(${NEW_NAME} PUBLIC ${COMPILE_OPTIONS})
        target_compile_options(${NEW_NAME} INTERFACE ${COMPILE_OPTIONS})
        message(STATUS "Done compile options")

        # Include dirs
        gather_vars(INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES _touched_id ${NEW_NAME})
        list(REMOVE_DUPLICATES INCLUDE_DIRECTORIES)
        target_include_directories(${NEW_NAME} PUBLIC ${INCLUDE_DIRECTORIES})
        target_include_directories(${NEW_NAME} INTERFACE ${INCLUDE_DIRECTORIES})
        message(STATUS "Done include dirs")

        # Link options
        gather_vars(LINK_FLAGS INTERFACE_LINK_OPTIONS _touched_l ${NEW_NAME})
        target_link_options(${NEW_NAME} PUBLIC ${LINK_FLAGS})
        target_link_options(${NEW_NAME} INTERFACE ${LINK_FLAGS})
        message(STATUS "Done link options")

        # Target properties were scoped to the target being built, lets scope them to static library
        string(REPLACE "$<TARGET_PROPERTY:" "$<TARGET_PROPERTY:${NEW_NAME}," COMPILE_DEFINITIONS "${COMPILE_DEFINITIONS}")
        string(REPLACE "$<TARGET_PROPERTY:" "$<TARGET_PROPERTY:${NEW_NAME}," COMPILE_OPTIONS "${COMPILE_OPTIONS}")
        string(REPLACE "$<TARGET_PROPERTY:" "$<TARGET_PROPERTY:${NEW_NAME}," INCLUDE_DIRECTORIES "${INCLUDE_DIRECTORIES}")
        string(REPLACE "$<TARGET_PROPERTY:" "$<TARGET_PROPERTY:${NEW_NAME}," LINK_FLAGS "${LINK_FLAGS}")
        message(STATUS "Done target properties")

        # The only way to get CMake to do some generator expression evaluation is to generate a file
        file(GENERATE OUTPUT ${NEW_NAME}_build_info.txt CONTENT
"Compile definitions: ${COMPILE_DEFINITIONS}
Compile options    : ${COMPILE_OPTIONS}
Include dirs       : ${INCLUDE_DIRECTORIES}
Link flags         : ${LINK_FLAGS}"
        )
        message(STATUS "Done build info")

        # Clearing all interface properties, else they will be used instead of the static library
        set_property(TARGET ${NEW_NAME} PROPERTY INTERFACE_LINK_LIBRARIES "")

    else()
        message(STATUS "Using non-interface library ${NAME} directly for overlay")
        set(NEW_NAME ${NAME})
    endif()

    # Move .text and .rodata sections into the overlay section
    add_custom_command(TARGET ${NEW_NAME} POST_BUILD COMMAND
        ${CMAKE_OBJCOPY} --rename-section .text=.${OVERLAY_NAME}_text --rename-section .rodata=.${OVERLAY_NAME}_rodata $<TARGET_FILE:${NEW_NAME}> $<TARGET_FILE:${NEW_NAME}>
        VERBATIM)

    target_link_libraries(${TARGET} ${NEW_NAME})

    get_property(overlays TARGET ${TARGET} PROPERTY OVERLAYS)
    if (NOT ${OVERLAY_NAME} IN_LIST overlays)
        set_property(TARGET ${TARGET} PROPERTY OVERLAYS ${OVERLAY_NAME} APPEND)
    endif()
endfunction()


function(pico_postprocess_overlays TARGET)
    get_property(overlays TARGET ${TARGET} PROPERTY OVERLAYS)

    pico_init_picotool()
    get_property(PICOTOOL_LOCATION TARGET picotool PROPERTY LOCATION)
    message(STATUS "PICOTOOL_LOCATION: ${PICOTOOL_LOCATION}")

    # create all_overlays.h
    set(CMAKE_CONFIGURABLE_FILE_CONTENT "#ifndef ALL_OVERLAYS_H\n#define ALL_OVERLAYS_H\n")
    foreach(overlay IN LISTS overlays)
        set(CMAKE_CONFIGURABLE_FILE_CONTENT
            "${CMAKE_CONFIGURABLE_FILE_CONTENT}\nextern uint8_t __load_start_overlay_${overlay}[];\nextern uint8_t __load_stop_overlay_${overlay}[];\nbi_decl(bi_ptr_string(0, 0, ${overlay}_hash,\"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\", SHA256_RESULT_BYTES * 2 + 1));")
    endforeach()
    set(CMAKE_CONFIGURABLE_FILE_CONTENT "${CMAKE_CONFIGURABLE_FILE_CONTENT}\n#endif\n")


    configure_file("${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in"
        "${CMAKE_CURRENT_BINARY_DIR}/all_overlays.h"
        @ONLY
    )
    unset(CMAKE_CONFIGURABLE_FILE_CONTENT)
    target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

    # create pico_overlay.ld
    set(CMAKE_CONFIGURABLE_FILE_CONTENT "OVERLAY : NOCROSSREFS {\n")
    foreach(overlay IN LISTS overlays)
        set(CMAKE_CONFIGURABLE_FILE_CONTENT
            "${CMAKE_CONFIGURABLE_FILE_CONTENT}\n.overlay_${overlay} {\nKEEP (*(.${overlay}*))\n. = ALIGN(4);\n}")
    endforeach()
    set(CMAKE_CONFIGURABLE_FILE_CONTENT "${CMAKE_CONFIGURABLE_FILE_CONTENT}\n} > RAM AT > FLASH")


    configure_file("${CMAKE_ROOT}/Modules/CMakeConfigurableFile.in"
        "${CMAKE_CURRENT_BINARY_DIR}/pico_overlay.ld"
        @ONLY
    )
    unset(CMAKE_CONFIGURABLE_FILE_CONTENT)

    set(OVERLAY_DUMP_ARGS "")
    foreach(overlay IN LISTS overlays)
        list(APPEND OVERLAY_DUMP_ARGS "--dump-section")
        list(APPEND OVERLAY_DUMP_ARGS ".overlay_${overlay}=${CMAKE_CURRENT_BINARY_DIR}/${overlay}.bin")
    endforeach()

    # dump the overlays as .bin files
    add_custom_command(TARGET ${TARGET} POST_BUILD COMMAND
        ${CMAKE_OBJCOPY}
        ARGS -Obinary ${OVERLAY_DUMP_ARGS} $<TARGET_FILE:${TARGET}> ${CMAKE_CURRENT_BINARY_DIR}/tmp
        COMMAND_EXPAND_LISTS VERBATIM
    )

    # seal the binary to add the load_map, else picotool config will fail
    add_custom_command(TARGET ${TARGET} POST_BUILD
            COMMAND picotool seal --quiet $<TARGET_FILE:${TARGET}> $<TARGET_FILE:${TARGET}>
            VERBATIM
    )

    # run script to hash all overlays and write the hashes in the binary
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} 
        ARGS -D target_file=$<TARGET_FILE:${TARGET}> -D picotool_path=${PICOTOOL_LOCATION} -P ${PICO_OVERLAY_DIR}/process_hashes.cmake -- ${overlays}
        COMMAND_EXPAND_LISTS VERBATIM
    )
endfunction()
