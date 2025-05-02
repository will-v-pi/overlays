
set(overlays "")
set(PROCESSING FALSE)
foreach(i RANGE ${CMAKE_ARGC})
    if(PROCESSING)
        list(APPEND overlays ${CMAKE_ARGV${i}})
    endif()
    if("${CMAKE_ARGV${i}}" STREQUAL "--")
        set(PROCESSING TRUE)
    endif()
endforeach()

# temporary fix, as picotool 2.1.1 throws overlapping memory errors
string(REPLACE "\\" "/" target_file ${target_file})
string(REPLACE "C:/" "/mnt/c/" target_file ${target_file})

message(STATUS "Hashing ${overlays} in ${target_file} using ${picotool_path}")

foreach(overlay IN LISTS overlays)
    file(SHA256 ${CMAKE_CURRENT_BINARY_DIR}/${overlay}.bin hash_value)

    execute_process(
        COMMAND wsl picotool config -s ${overlay}_hash ${hash_value} ${target_file}
        # COMMAND ${picotool_path} config -s ${overlay}_hash ${hash_value} ${target_file}
        RESULT_VARIABLE result
        OUTPUT_QUIET
    )
    if(result EQUAL 0)
        message(STATUS "Hashed overlay ${overlay}")
    else()
        message(STATUS "Failed to hash overlay ${overlay}")
    endif()
endforeach()
