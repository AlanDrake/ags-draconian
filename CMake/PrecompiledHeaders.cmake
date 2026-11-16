# Precompiled headers of stable content (docs/refactor/PLAN.md, 2.10b).
#
# ags_precompile_headers(<target> [ENGINE] [GTEST])
#
# Gives a first-party C++ target a precompiled header of the standard library, the four Common headers
# that nearly every file includes (platform.h, types.h, string.h, assert.h) and, on request, glm (ENGINE:
# a third of the engine's files include it; a few more standard headers, too) or googletest (GTEST: the
# tests). Never a header of the game (Common/game, Common/gui, Engine/game...): those change in every
# step of the refactor, and each change would rebuild the whole target. Allegro and SDL are left out on
# purpose (measured in 2.10b: Allegro parses in 0.04 s, SDL.h is included by a seventh of the engine's
# files, and Allegro's macros would reach the files that do not include it today).
#
# Switched off, the build is the one without it:
#   - CMAKE_DISABLE_PRECOMPILE_HEADERS=ON (CMake's own switch; Script/compare_objects.py, Script/prepush.py,
#     the header checks and the compile-time readings set it, so that a missing #include still fails);
#   - a compiler launcher (ccache): MSVC's /Yu cannot be cached, and GCC's and Clang's need ccache settings
#     (sloppiness pch_defines,time_macros) that CI does not have; AGS_PCH_WITH_COMPILER_LAUNCHER=ON keeps
#     the precompiled header anyway;
#   - Emscripten, Android, Apple (Objective-C++ sources), FreeBSD and GCC older than 11: not verified there.
# The header is PRIVATE: the targets that link a library do not inherit it (each has its own, with the
# flags it is compiled with), and the header_check objects never see it.

option(AGS_PCH_WITH_COMPILER_LAUNCHER "Use precompiled headers although a compiler launcher (ccache) is set" OFF)

set(AGS_PCH_ENABLED TRUE)
set(AGS_PCH_REASON "")
if(CMAKE_DISABLE_PRECOMPILE_HEADERS)
    set(AGS_PCH_ENABLED FALSE)
    set(AGS_PCH_REASON "CMAKE_DISABLE_PRECOMPILE_HEADERS is ON")
elseif(NOT (MSVC OR (LINUX AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
        (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11)))))
    set(AGS_PCH_ENABLED FALSE)
    set(AGS_PCH_REASON "only MSVC and GCC 11+ or Clang on Linux are verified")
elseif((CMAKE_CXX_COMPILER_LAUNCHER OR CMAKE_C_COMPILER_LAUNCHER) AND NOT AGS_PCH_WITH_COMPILER_LAUNCHER)
    set(AGS_PCH_ENABLED FALSE)
    set(AGS_PCH_REASON "a compiler launcher is set (AGS_PCH_WITH_COMPILER_LAUNCHER=ON keeps it)")
endif()
if(AGS_PCH_ENABLED)
    message(STATUS "Precompiled headers: on")
else()
    message(STATUS "Precompiled headers: off (${AGS_PCH_REASON})")
endif()

function(ags_precompile_headers target)
    if(NOT AGS_PCH_ENABLED)
        return()
    endif()
    cmake_parse_arguments(ARG "ENGINE;GTEST" "" "" ${ARGN})

    set(system
        cassert cerrno climits cmath cstddef cstdint cstdio cstdlib cstring ctime
        algorithm array atomic functional list memory stdexcept string unordered_map vector)
    set(common
        "${PROJECT_SOURCE_DIR}/Common/platform/platform.h"
        "${PROJECT_SOURCE_DIR}/Common/platform/types.h"
        "${PROJECT_SOURCE_DIR}/Common/util/string.h"
        "${PROJECT_SOURCE_DIR}/Common/debug/assert.h")
    # C++ only: the C files of a target (stdio_compat.c, Allegro's sources) get no precompiled header.
    # $<ANGLE-R> because the condition's text ends at the first bare '>' when it is false (the C case)
    set(only_cxx "$<COMPILE_LANGUAGE:CXX>")
    set(headers "")
    foreach(header ${system})
        list(APPEND headers "$<${only_cxx}:<${header}$<ANGLE-R>>")
    endforeach()
    if(ARG_ENGINE)
        foreach(header chrono deque map numeric set)
            list(APPEND headers "$<${only_cxx}:<${header}$<ANGLE-R>>")
        endforeach()
    endif()
    if(ARG_GTEST)
        list(APPEND headers "$<${only_cxx}:<gtest/gtest.h$<ANGLE-R>>")
    endif()
    foreach(header ${common})
        list(APPEND headers "$<${only_cxx}:${header}>")
    endforeach()
    if(ARG_ENGINE)
        list(APPEND headers "$<${only_cxx}:<glm/ext/matrix_transform.hpp$<ANGLE-R>>"
                            "$<${only_cxx}:<glm/mat4x4.hpp$<ANGLE-R>>")
    endif()
    target_precompile_headers(${target} PRIVATE ${headers})
endfunction()
