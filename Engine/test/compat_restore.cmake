# Compatibility contracts C1 and C2 (docs/refactor/PLAN.md): the engine runs a game built by a
# released AGS 4 editor and restores a save written by that release's engine. The game's own
# script (Common/test/data/compat/fixture.asc) checks the restored state and writes c1result.txt.
#
#   cmake -DENGINE=<ags> -DFIXTURE=<Common/test/data/compat/<version>> -DWORK=<dir> -P compat_restore.cmake
foreach(var ENGINE FIXTURE WORK)
    if(NOT DEFINED ${var})
        message(FATAL_ERROR "${var} is not set")
    endif()
endforeach()

file(REMOVE_RECURSE "${WORK}")
file(MAKE_DIRECTORY "${WORK}")
file(COPY "${FIXTURE}/agssave.001" DESTINATION "${WORK}")

execute_process(
    COMMAND "${ENGINE}" --gfxdriver Software --windowed --no-message-box --nosound
            --log-stdout=all:info --user-data-dir "${WORK}" "${FIXTURE}/game.ags"
    WORKING_DIRECTORY "${WORK}"
    TIMEOUT 120
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE out)
file(WRITE "${WORK}/engine.log" "${out}")

set(result_file "${WORK}/c1result.txt")
if(NOT EXISTS "${result_file}")
    message(FATAL_ERROR "The game wrote no result (engine exit: ${rc}); log: ${WORK}/engine.log\n${out}")
endif()
file(STRINGS "${result_file}" result LIMIT_COUNT 1)
if(NOT result STREQUAL "PASS")
    message(FATAL_ERROR "The restored state differs: ${result}; log: ${WORK}/engine.log")
endif()
message(STATUS "${FIXTURE}: ${result}")
