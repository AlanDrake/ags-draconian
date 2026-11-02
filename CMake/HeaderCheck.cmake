# Header self-containment check (docs/refactor/PLAN.md, 2.6).
#
# ags_add_header_check(<name> <target> <directory> [EXCLUDE <regex>...])
#
# Adds an OBJECT library <name> which compiles each header under <directory> on its own: one generated
# source file per header that includes nothing else. A header that does not include what it uses (or
# forward declare it) fails to compile. The library takes the include directories, definitions, options
# and libraries from <target>. It is left out of "all": build the "header_check" target.
# The regular expressions in EXCLUDE are matched against the header's path relative to <directory>.

function(ags_add_header_check name target directory)
    cmake_parse_arguments(ARG "" "" "EXCLUDE" ${ARGN})
    if(NOT TARGET ${target})
        return()
    endif()

    file(GLOB_RECURSE headers RELATIVE "${directory}" CONFIGURE_DEPENDS "${directory}/*.h" "${directory}/*.hpp")
    set(stubs "")
    foreach(header ${headers})
        set(skip FALSE)
        foreach(pattern ${ARG_EXCLUDE})
            if(header MATCHES "${pattern}")
                set(skip TRUE)
            endif()
        endforeach()
        if(skip)
            continue()
        endif()
        set(stub "${CMAKE_CURRENT_BINARY_DIR}/header_check/${name}/${header}.cpp")
        # Rewrites the file only when its content changes, so a configure does not rebuild everything
        file(CONFIGURE OUTPUT "${stub}" CONTENT "#include \"${directory}/${header}\"\n" @ONLY)
        list(APPEND stubs "${stub}")
    endforeach()

    add_library(${name} OBJECT EXCLUDE_FROM_ALL ${stubs})
    set_target_properties(${name} PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS NO)
    target_include_directories(${name} PRIVATE "$<TARGET_PROPERTY:${target},INCLUDE_DIRECTORIES>")
    target_compile_definitions(${name} PRIVATE "$<TARGET_PROPERTY:${target},COMPILE_DEFINITIONS>")
    target_compile_options(${name} PRIVATE "$<TARGET_PROPERTY:${target},COMPILE_OPTIONS>")
    target_link_libraries(${name} PRIVATE "$<TARGET_PROPERTY:${target},LINK_LIBRARIES>")
    add_dependencies(header_check ${name})
endfunction()

# The headers of each platform are checked on that platform only
function(ags_header_check_platform_excludes out)
    set(all android bsd emscripten ios linux osx windows)
    if(WIN32)
        set(own windows)
    elseif(ANDROID)
        set(own android linux)
    elseif(EMSCRIPTEN)
        set(own emscripten)
    elseif(APPLE)
        set(own osx)
    elseif(CMAKE_SYSTEM_NAME MATCHES "BSD")
        set(own bsd linux)
    else()
        set(own linux)
    endif()
    set(excludes "")
    foreach(platform ${all})
        if(NOT platform IN_LIST own)
            list(APPEND excludes "^platform/${platform}/")
        endif()
    endforeach()
    set(${out} ${excludes} PARENT_SCOPE)
endfunction()

function(ags_add_header_checks)
    add_custom_target(header_check)

    ags_header_check_platform_excludes(platform_excludes)
    # Third-party code, tests, and the pieces of util/library.h, which that header selects by platform
    set(common_excludes "^libsrc/" "^libinclude/" "^test[0-9]*/" "^util/library_(windows|posix|psp|dummy)\\.h$")
    if(NOT WIN32)
        list(APPEND common_excludes "^util/smart_ptr\\.h$") # COM helper
        list(APPEND common_excludes "^gfx/gfxfilter_(aa)?d3d") # Direct3D filters, which #error elsewhere
    endif()

    if(AGS_BUILD_ENGINE)
        ags_add_header_check(header_check_common engine "${PROJECT_SOURCE_DIR}/Common" EXCLUDE ${common_excludes})
        ags_add_header_check(header_check_engine engine "${PROJECT_SOURCE_DIR}/Engine"
                             EXCLUDE ${common_excludes} ${platform_excludes})
    endif()
    if(AGS_BUILD_TOOLS)
        ags_add_header_check(header_check_tools libtools "${PROJECT_SOURCE_DIR}/Tools" EXCLUDE ${common_excludes})
    endif()
    if(AGS_BUILD_COMPILER)
        # script/ is the legacy compiler, which is going away (PLAN.md, 3.3)
        ags_add_header_check(header_check_compiler compiler "${PROJECT_SOURCE_DIR}/Compiler"
                             EXCLUDE ${common_excludes} "^script/")
    endif()
endfunction()
