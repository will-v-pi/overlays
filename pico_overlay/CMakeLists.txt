pico_add_library(pico_overlay)

target_sources(pico_overlay INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/overlay.c
)

target_include_directories(pico_overlay_headers SYSTEM INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)

pico_mirrored_target_link_libraries(pico_overlay INTERFACE
        pico_sha256
        pico_binary_info
)



function(add_overlay TARGET NAME)
    set_property(TARGET ${TARGET} PROPERTY OVERLAYS ${NAME} APPEND)
endfunction()


function(postprocess_overlays TARGET)
    get_property(overlays TARGET ${TARGET} PROPERTY OVERLAYS)
    find_package (Python3 REQUIRED COMPONENTS Interpreter)
    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/process_hashes.py $<TARGET_FILE:${TARGET}>
        VERBATIM)

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
endfunction()
