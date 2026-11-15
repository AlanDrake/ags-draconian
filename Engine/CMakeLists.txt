# Engine
# -----------------------------------------------------------------------------

if (NOT ANDROID)
add_library(engine STATIC)
else ()
add_library(engine SHARED)
endif ()


set_target_properties(engine PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS NO
    C_STANDARD 11
    C_EXTENSIONS YES
)

target_include_directories(engine PUBLIC .)

target_sources(engine
    PRIVATE
    debug/agseditordebugger.h
    debug/debug.cpp
    debug/debug_log.h
    debug/debugger.h
    debug/dummyagsdebugger.h
    debug/logfile.cpp
    debug/logfile.h
    debug/memory_inspect.cpp
    debug/memory_inspect.h
    device/mousew32.cpp
    device/mousew32.h
    device/sys_events.cpp
    device/sys_events.h
    game/asset_helper.h
    game/audio/audio.cpp
    game/audio/audio.h
    game/audio/audio_channel.cpp
    game/audio/audio_channel.h
    game/audio/audio_clip.cpp
    game/audio/audio_clip.h
    game/dialog/dialog.cpp
    game/dialog/dialog.h
    game/dialog/dialog_options_rendering.cpp
    game/dialog/dialog_options_rendering.h
    game/dialog/display.cpp
    game/dialog/display.h
    game/dialog/lipsync.h
    game/dialog/parser.cpp
    game/dialog/parser.h
    game/dialog/speech.cpp
    game/dialog/speech.h
    game/dynobj/all_dynamic_classes.h
    game/dynobj/all_script_classes.h
    game/dynobj/cc_audio_channel.cpp
    game/dynobj/cc_audio_channel.h
    game/dynobj/cc_audio_clip.cpp
    game/dynobj/cc_audio_clip.h
    game/dynobj/cc_character.cpp
    game/dynobj/cc_character.h
    game/dynobj/cc_dialog.cpp
    game/dynobj/cc_dialog.h
    game/dynobj/cc_gui.cpp
    game/dynobj/cc_gui.h
    game/dynobj/cc_gui_control.cpp
    game/dynobj/cc_gui_control.h
    game/dynobj/cc_hotspot.cpp
    game/dynobj/cc_hotspot.h
    game/dynobj/cc_inventory.cpp
    game/dynobj/cc_inventory.h
    game/dynobj/cc_object.cpp
    game/dynobj/cc_object.h
    game/dynobj/cc_region.cpp
    game/dynobj/cc_region.h
    game/dynobj/cc_serializer.cpp
    game/dynobj/cc_serializer.h
    game/dynobj/cc_walk_behind.cpp
    game/dynobj/cc_walk_behind.h
    game/dynobj/cc_walkable_area.cpp
    game/dynobj/cc_walkable_area.h
    game/dynobj/script_camera.cpp
    game/dynobj/script_camera.h
    game/dynobj/script_date_time.cpp
    game/dynobj/script_date_time.h
    game/dynobj/script_dialog_options_rendering.cpp
    game/dynobj/script_dialog_options_rendering.h
    game/dynobj/script_dict.cpp
    game/dynobj/script_dict.h
    game/dynobj/script_drawing_surface.cpp
    game/dynobj/script_drawing_surface.h
    game/dynobj/script_dynamic_sprite.cpp
    game/dynobj/script_dynamic_sprite.h
    game/dynobj/script_file.cpp
    game/dynobj/script_file.h
    game/dynobj/script_game.cpp
    game/dynobj/script_game.h
    game/dynobj/script_joystick.cpp
    game/dynobj/script_joystick.h
    game/dynobj/script_motion_path.cpp
    game/dynobj/script_motion_path.h
    game/dynobj/script_mouse.cpp
    game/dynobj/script_mouse.cpp
    game/dynobj/script_mouse.h
    game/dynobj/script_objects.h
    game/dynobj/script_overlay.cpp
    game/dynobj/script_overlay.h
    game/dynobj/script_pathfinder.cpp
    game/dynobj/script_pathfinder.h
    game/dynobj/script_restored_save_info.h
    game/dynobj/script_set.cpp
    game/dynobj/script_set.h
    game/dynobj/script_shader.cpp
    game/dynobj/script_shader.h
    game/dynobj/script_string.cpp
    game/dynobj/script_string.h
    game/dynobj/script_system.cpp
    game/dynobj/script_system.cpp
    game/dynobj/script_system.h
    game/dynobj/script_touch_pointer.cpp
    game/dynobj/script_touch_pointer.h
    game/dynobj/script_video_player.cpp
    game/dynobj/script_video_player.h
    game/dynobj/script_view_frame.cpp
    game/dynobj/script_view_frame.h
    game/dynobj/script_viewport.cpp
    game/dynobj/script_viewport.h
    game/event.cpp
    game/event.h
    game/file.cpp
    game/file.h
    game/game.cpp
    game/game.h
    game/game_init.cpp
    game/game_init.h
    game/game_state.cpp
    game/game_state.h
    game/input/joystick.cpp
    game/input/joystick.h
    game/input/keyboard.cpp
    game/input/keyboard.h
    game/input/touch.cpp
    game/input/touch.h
    game/lib/date_time.cpp
    game/lib/date_time.h
    game/lib/math.cpp
    game/lib/math.h
    game/lib/script_containers.cpp
    game/lib/script_containers.h
    game/lib/string.cpp
    game/lib/string.h
    game/lib/utils_script.cpp
    game/lib/utils_script.h
    game/navigation/pathfinder_script.cpp
    game/navigation/pathfinder_script.h
    game/navigation/route_finder.cpp
    game/navigation/route_finder.h
    game/navigation/route_finder_impl.cpp
    game/navigation/route_finder_impl.h
    game/navigation/route_finder_jps.inl
    game/render/draw.cpp
    game/render/draw.h
    game/render/draw_software.cpp
    game/render/draw_software.h
    game/render/drawing_surface.cpp
    game/render/drawing_surface.h
    game/render/dynamic_sprite.cpp
    game/render/dynamic_sprite.h
    game/render/overlay.cpp
    game/render/overlay.h
    game/render/screen.cpp
    game/render/screen.h
    game/render/screen_overlay.cpp
    game/render/screen_overlay.h
    game/render/shader_script.cpp
    game/render/shader_script.h
    game/render/viewport.cpp
    game/render/viewport.h
    game/render/viewport_script.cpp
    game/render/viewport_script.h
    game/runtime_defines.h
    game/save/savegame.cpp
    game/save/savegame.h
    game/save/savegame_components.cpp
    game/save/savegame_components.h
    game/save/savegame_internal.h
    game/script.cpp
    game/script.h
    game/script_api/audio_channel_api.cpp
    game/script_api/audio_clip_api.cpp
    game/script_api/bindings.h
    game/script_api/button_api.cpp
    game/script_api/character_api.cpp
    game/script_api/containers_api.cpp
    game/script_api/date_time_api.cpp
    game/script_api/dialog_api.cpp
    game/script_api/dialog_options_rendering_api.cpp
    game/script_api/drawing_surface_api.cpp
    game/script_api/dynamic_array_api.cpp
    game/script_api/dynamic_sprite_api.cpp
    game/script_api/exports.cpp
    game/script_api/exports.h
    game/script_api/file_api.cpp
    game/script_api/game_api.cpp
    game/script_api/global_api.cpp
    game/script_api/gui_api.cpp
    game/script_api/gui_control_api.cpp
    game/script_api/hotspot_api.cpp
    game/script_api/inv_window_api.cpp
    game/script_api/inventory_item_api.cpp
    game/script_api/joystick_api.cpp
    game/script_api/label_api.cpp
    game/script_api/list_box_api.cpp
    game/script_api/math_api.cpp
    game/script_api/mouse_api.cpp
    game/script_api/object_api.cpp
    game/script_api/overlay_api.cpp
    game/script_api/parser_api.cpp
    game/script_api/pathfinder_api.cpp
    game/script_api/region_api.cpp
    game/script_api/room_api.cpp
    game/script_api/save_info_api.cpp
    game/script_api/screen_api.cpp
    game/script_api/shader_api.cpp
    game/script_api/slider_api.cpp
    game/script_api/speech_api.cpp
    game/script_api/string_api.cpp
    game/script_api/system_api.cpp
    game/script_api/text_box_api.cpp
    game/script_api/touch_api.cpp
    game/script_api/utils_api.cpp
    game/script_api/video_api.cpp
    game/script_api/view_frame_api.cpp
    game/script_api/viewport_api.cpp
    game/script_api/walk_behind_api.cpp
    game/script_api/walkable_area_api.cpp
    game/system.cpp
    game/system.h
    game/translation.cpp
    game/translation.h
    game/ui/animating_gui_button.cpp
    game/ui/animating_gui_button.h
    game/ui/button.cpp
    game/ui/button.h
    game/ui/csci_dialog.cpp
    game/ui/csci_dialog.h
    game/ui/gui.cpp
    game/ui/gui.h
    game/ui/gui_control.cpp
    game/ui/gui_control.h
    game/ui/gui_dialog.cpp
    game/ui/gui_dialog.h
    game/ui/gui_dialog_defines.h
    game/ui/gui_engine.cpp
    game/ui/gui_inv.cpp
    game/ui/inv_window.cpp
    game/ui/inv_window.h
    game/ui/label.cpp
    game/ui/label.h
    game/ui/list_box.cpp
    game/ui/list_box.h
    game/ui/mouse.cpp
    game/ui/mouse.h
    game/ui/my_controls.h
    game/ui/my_label.cpp
    game/ui/my_label.h
    game/ui/my_list_box.cpp
    game/ui/my_list_box.h
    game/ui/my_push_button.cpp
    game/ui/my_push_button.h
    game/ui/my_text_box.cpp
    game/ui/my_text_box.h
    game/ui/new_control.cpp
    game/ui/new_control.h
    game/ui/slider.cpp
    game/ui/slider.h
    game/ui/text_box.cpp
    game/ui/text_box.h
    game/video/video.cpp
    game/video/video.h
    game/video/video_script.cpp
    game/video/video_script.h
    game/world/character.cpp
    game/world/character.h
    game/world/character_extras.cpp
    game/world/character_extras.h
    game/world/character_info_engine.cpp
    game/world/hotspot.cpp
    game/world/hotspot.h
    game/world/inventory_item.cpp
    game/world/inventory_item.h
    game/world/move_list.cpp
    game/world/move_list.h
    game/world/object.cpp
    game/world/object.h
    game/world/properties.cpp
    game/world/properties.h
    game/world/region.cpp
    game/world/region.h
    game/world/room.cpp
    game/world/room.h
    game/world/room_object.cpp
    game/world/room_object.h
    game/world/room_status.cpp
    game/world/room_status.h
    game/world/view_frame.cpp
    game/world/view_frame.h
    game/world/walk_behind.cpp
    game/world/walk_behind.h
    game/world/walkable_area.cpp
    game/world/walkable_area.h
    gfx/ali3dexception.h
    gfx/ali3dogl.cpp
    gfx/ali3dogl.h
    gfx/ali3dsw.cpp
    gfx/ali3dsw.h
    gfx/ddb.h
    gfx/display_setup.h
    gfx/gfx_util.cpp
    gfx/gfx_util.h
    gfx/gfxdefines.h
    gfx/gfxdriverbase.cpp
    gfx/gfxdriverbase.h
    gfx/gfxdriverfactory.cpp
    gfx/gfxdriverfactory.h
    gfx/gfxdriverfactorybase.h
    gfx/gfxfilter.h
    gfx/gfxfilter_aad3d.cpp
    gfx/gfxfilter_aad3d.h
    gfx/gfxfilter_aaogl.cpp
    gfx/gfxfilter_aaogl.h
    gfx/gfxfilter_d3d.cpp
    gfx/gfxfilter_d3d.h
    gfx/gfxfilter_ogl.cpp
    gfx/gfxfilter_ogl.h
    gfx/gfxfilter_scaling.cpp
    gfx/gfxfilter_scaling.h
    gfx/gfxfilter_sdl_renderer.cpp
    gfx/gfxfilter_sdl_renderer.h
    gfx/gfxfilter_aa_sdl_renderer.cpp
    gfx/gfxfilter_aa_sdl_renderer.h
    gfx/gfxmodelist.h
    gfx/graphicsdriver.h
    gfx/ogl_headers.h
    main/config.cpp
    main/config.h
    main/engine.cpp
    main/engine.h
    main/engine_setup.cpp
    main/engine_setup.h
    main/game_file.cpp
    main/game_file.h
    main/game_run.cpp
    main/game_run.h
    main/game_start.cpp
    main/game_start.h
    main/graphics_mode.cpp
    main/graphics_mode.h
    main/main.cpp
    main/main.h
    main/quit.cpp
    main/quit.h
    main/update.cpp
    main/update.h
    media/audio/audio_core.cpp
    media/audio/audio_core.h
    media/audio/audio_system.h
    media/audio/audiodefines.h
    media/audio/audioplayer.cpp
    media/audio/audioplayer.h
    media/audio/sdldecoder.cpp
    media/audio/sdldecoder.h
    media/audio/openalsource.cpp
    media/audio/openalsource.h
    media/audio/openal.h
    media/audio/queuedaudioitem.cpp
    media/audio/queuedaudioitem.h
    media/audio/sound.cpp
    media/audio/sound.h
    media/audio/soundclip.cpp
    media/audio/soundclip.h
    media/video/flic_player.cpp
    media/video/flic_player.h
    media/video/theora_player.cpp
    media/video/theora_player.h
    media/video/video_core.cpp
    media/video/video_core.h
    media/video/videoplayer.cpp
    media/video/videoplayer.h
    platform/base/agsplatformdriver.cpp
    platform/base/agsplatformdriver.h
    platform/base/engine_config.h
    platform/base/engine_defines.h
    platform/base/engine_services.cpp
    platform/base/engine_services.h
    platform/base/agsplatform_xdg_unix.cpp
    platform/base/agsplatform_xdg_unix.h
    platform/base/sys_main.cpp
    platform/base/sys_main.h
    platform/util/libc.c
    plugin/agsplugin.cpp
    plugin/agsplugin.h
    plugin/plugin_builtin.h
    plugin/plugin_engine.h
    plugin/plugin_stubs.cpp
    resource/resource.h
    script/cc_reflecthelper.cpp
    script/cc_reflecthelper.h
    script/dynobj/cc_ags_dynamic_object.cpp
    script/dynobj/cc_ags_dynamic_object.h
    script/dynobj/cc_dynamic_array.cpp
    script/dynobj/cc_dynamic_array.h
    script/dynobj/cc_plugin_object.h
    script/dynobj/cc_script_object.h
    script/dynobj/cc_static_array.cpp
    script/dynobj/cc_static_array.h
    script/dynobj/dynobj_manager.cpp
    script/dynobj/dynobj_manager.h
    script/dynobj/managed_object_pool.cpp
    script/dynobj/managed_object_pool.h
    script/dynobj/script_user_object.cpp
    script/dynobj/script_user_object.h
    script/executingscript.cpp
    script/executingscript.h
    script/runtimescript.cpp
    script/runtimescript.h
    script/runtimescriptvalue.cpp
    script/runtimescriptvalue.h
    script/script_api.cpp
    script/script_api.h
    script/script_binder.h
    script/script_runtime.cpp
    script/script_runtime.h
    script/scriptexecutor.cpp
    script/scriptexecutor.h
    script/systemimports.cpp
    script/systemimports.h
    util/library.h
    util/library_dummy.h
    util/library_posix.h
    util/path_helper.h
    util/threading.h
    util/timer.cpp
    util/timer.h

    platform/windows/acplwin.cpp
    platform/windows/debug/namedpipesagsdebugger.cpp
    platform/windows/gfx/ali3dd3d.cpp
    platform/windows/gfx/ali3dd3d11.cpp
    platform/windows/setup/winsetup.cpp
    platform/windows/setup/winsetup.h
    platform/windows/setup/winapihelpers.cpp
    platform/windows/setup/winapihelpers.h
    platform/windows/setup/windialog.cpp
    platform/windows/setup/windialog.h
    platform/windows/setup/winpagedialog.cpp
    platform/windows/setup/winpagedialog.h
    platform/windows/setup/basicpagedialog.cpp
    platform/windows/setup/basicpagedialog.h
    platform/windows/setup/advancedpagedialog.cpp
    platform/windows/setup/advancedpagedialog.h
    platform/windows/minidump.cpp
    platform/windows/win_ex_handling.cpp

    platform/linux/acpllnx.cpp

    platform/osx/acplmac.cpp

    platform/android/acpland.cpp

    platform/ios/acplios.cpp

    platform/emscripten/acpemscripten.cpp

    platform/bsd/acplbsd.cpp

    platform/windows/win_ex_handling.cpp
    platform/windows/win_ex_handling.h
    platform/windows/minidump.cpp
    platform/windows/minidump.h
    platform/windows/winapi_exclusive.h

    platform/base/mobile_base.cpp
    platform/base/mobile_base.h
)

if(ANDROID)
    target_sources(engine PRIVATE
        platform/android/helper/jni_helper.cpp)
endif()

if(MACOS)
    target_sources(engine PRIVATE 
        platform/osx/alplmac.mm
    )
endif()

if (AGS_BUILTIN_PLUGINS)
    target_compile_definitions(engine PRIVATE BUILTIN_PLUGINS)

    target_sources(engine PRIVATE 
        ../Plugins/agsblend/agsblend/AGSBlend.cpp
        ../Plugins/agsblend/agsblend/agsblend.h

        ../Plugins/agsflashlight/agsflashlight/agsflashlight.cpp
        ../Plugins/agsflashlight/agsflashlight/agsflashlight.h

        ../Plugins/ags_parallax/ags_parallax/ags_parallax.cpp
        ../Plugins/ags_parallax/ags_parallax/ags_parallax.h

        ../Plugins/ags_snowrain/ags_snowrain/ags_snowrain.cpp
        ../Plugins/ags_snowrain/ags_snowrain/ags_snowrain.h

        ../Plugins/AGSSpriteFont/AGSSpriteFont/AGSSpriteFont.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/AGSSpriteFont.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/CharacterEntry.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/CharacterEntry.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFont.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFont.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFontRenderer.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFontRenderer.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFontRendererClifftopGames.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFontRendererClifftopGames.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthFont.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthFont.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthSpriteFont.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthSpriteFont.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthSpriteFontClifftopGames.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthSpriteFontClifftopGames.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/color.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/color.h

        ../Plugins/agstouch/agstouch/agstouch.cpp
        ../Plugins/agstouch/agstouch/agstouch.h

        ../Plugins/agspalrender/agspalrender/palrender.h
        ../Plugins/agspalrender/agspalrender/ags_palrender.cpp
        ../Plugins/agspalrender/agspalrender/raycast.cpp
        ../Plugins/agspalrender/agspalrender/raycast.h
    )
endif()

if (AGS_BUILD_STR)
    target_compile_definitions(engine PUBLIC BUILD_STR=\"${AGS_BUILD_STR}\")
endif()


target_link_libraries(engine PUBLIC 
    AGS::Common 
    ${CMAKE_DL_LIBS} 
    Allegro::Allegro
    External::OpenAL
    ${SDL2_LIBRARY}
    SDL2_sound::SDL2_sound
)

if(NOT AGS_DISABLE_THREADS)
    target_link_libraries(engine PUBLIC Threads::Threads)
endif()

if (AGS_OPENGLES2)
    target_link_libraries(engine PUBLIC EGL GLESv2 Glad::GladGLES2)
else()
    target_link_libraries(engine PUBLIC Glad::Glad)
endif (AGS_OPENGLES2)

# common has a circular dependency on engine
# TODO: currently required for gcc support, will look into this.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
target_link_libraries(common PUBLIC engine)
endif ()

if (WIN32 OR LINUX)
    target_compile_definitions(engine PRIVATE AGS_HAS_CD_AUDIO)
endif ()

# Common sets this for itself; game/file.cpp in the engine needs it too (the Linux Makefile build had it
# for every file)
if (LINUX)
    target_compile_definitions(engine PRIVATE AGS_CASE_SENSITIVE_FILESYSTEM)
endif ()

if (AGS_NO_VIDEO_PLAYER)
    target_compile_definitions(engine PRIVATE AGS_NO_VIDEO_PLAYER)
else()
    target_link_libraries(engine PUBLIC Apeg::Apeg)
endif()

if (WIN32)
    target_link_libraries(engine PUBLIC shlwapi)
    target_link_libraries(engine PUBLIC D3DCompiler.lib)
endif()

if(ANDROID)
    find_library(ANDROID_LOG_LIB log)
    target_link_libraries(engine PUBLIC ${ANDROID_LOG_LIB})
endif()

get_target_property(ENGINE_SOURCES engine SOURCES)
# Set ROOT to parent directory to include Plugins/
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. PREFIX "Source Files" FILES ${ENGINE_SOURCES})

# Standalone exe
# -----------------------------------------------------------------------------
if (NOT ANDROID)
    add_executable(ags)
else ()
    add_library(ags SHARED)
endif (NOT ANDROID)

if (AGS_DESKTOP)
    install(TARGETS ags RUNTIME DESTINATION bin)
endif ()

set_target_properties(ags PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS NO
)

target_include_directories(ags PRIVATE .)

target_sources(ags
    PRIVATE
    main/main_sdl2.cpp
)

target_link_libraries(ags PRIVATE engine ${SDL2MAIN_LIBRARY})

if (LINUX)
    # We may override `allegro_icon` and `load_midi_pf` depending on the version
    target_link_options(ags PRIVATE -Wl,--allow-multiple-definition)
endif ()

set_target_properties(ags PROPERTIES
    RUNTIME_OUTPUT_NAME ags
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if (WIN32)
    target_sources(ags PRIVATE resource/version.rc) # for setup dialog and d3d shaders
    set_target_properties(ags PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    if (CMAKE_SIZEOF_VOID_P EQUAL 4)
        # enable more than 2 GB in a 32-bit application
        if (MINGW)
            target_link_options(ags PUBLIC "-Wl,--large-address-aware")
        endif ()
        if (MSVC)
            target_link_options(ags PUBLIC "/LARGEADDRESSAWARE")
        endif ()
    endif ()
endif()

# Test
# -----------------------------------------------------------------------------
if(AGS_TESTS)
    add_executable(
        engine_test
        test/game_test.cpp
        test/movelist_test.cpp
        test/plugin_api_test.cpp
        test/route_finder_test.cpp
        test/scsprintf_test.cpp
        test/systemimports_test.cpp
    )
    target_compile_definitions(engine_test PRIVATE
        PLUGIN_API_HEADER="${CMAKE_CURRENT_SOURCE_DIR}/plugin/agsplugin.h"
        PLUGIN_ENGINE_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/plugin/agsplugin.cpp"
        PLUGIN_API_SNAPSHOT="${CMAKE_CURRENT_SOURCE_DIR}/test/data/plugin_api.txt")
    set_target_properties(engine_test PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS NO
        C_STANDARD 11
        C_EXTENSIONS NO
        INTERPROCEDURAL_OPTIMIZATION FALSE
        )
    target_link_libraries(
        engine_test
        engine
        common
        gtest_main
    )
    if(AGS_USE_LOCAL_GTEST)
        target_link_libraries(engine_test gtest)
    endif()

    # Tests that need the whole engine with its real globals (the script API table, savegame components):
    # engine_test replaces some engine symbols with stubs, so they cannot be linked there
    add_executable(
        engine_full_test
        test/character_turn_test.cpp
        test/gui_layout_test.cpp
        test/managed_pool_test.cpp
        test/overlay_test.cpp
        test/script_exports_test.cpp
        test/script_binder_test.cpp
        test/script_bytecode_test.cpp
        test/script_vm_test.cpp
        test/sprite_cache_test.cpp
        test/savegame_components_test.cpp
        test/savegame_file_test.cpp
        test/savegame_test.cpp
        test/speech_test.cpp
    )
    # The script VM tests compile their scripts in memory: the script compiler's own sources are
    # built in, without the compiler library's copies of Common, which the engine already links
    target_sources(engine_full_test PRIVATE
        ${CMAKE_SOURCE_DIR}/Compiler/script/cc_compiledscript.cpp
        ${CMAKE_SOURCE_DIR}/Compiler/script/cc_internallist.cpp
        ${CMAKE_SOURCE_DIR}/Compiler/script/cc_symboltable.cpp
        ${CMAKE_SOURCE_DIR}/Compiler/script/cs_compile_time.cpp
        ${CMAKE_SOURCE_DIR}/Compiler/script/cs_compiler.cpp
        ${CMAKE_SOURCE_DIR}/Compiler/script/cs_message_handler.cpp
        ${CMAKE_SOURCE_DIR}/Compiler/script/cs_parser.cpp
        ${CMAKE_SOURCE_DIR}/Compiler/script/cs_scanner.cpp)
    target_include_directories(engine_full_test PRIVATE ${CMAKE_SOURCE_DIR}/Compiler)
    set_target_properties(engine_full_test PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS NO
        C_STANDARD 11
        C_EXTENSIONS NO
        INTERPROCEDURAL_OPTIMIZATION FALSE
        )
    target_compile_definitions(engine_full_test PRIVATE
        SCRIPT_EXPORTS_SNAPSHOT="${CMAKE_CURRENT_SOURCE_DIR}/test/data/script_exports.txt"
        RELEASE_EXPORTS_4_0_0_30="${CMAKE_CURRENT_SOURCE_DIR}/test/data/script_exports_4.0.0.30.txt")
    target_link_libraries(
        engine_full_test
        engine
        common
        gtest_main
    )
    if(AGS_USE_LOCAL_GTEST)
        target_link_libraries(engine_full_test gtest)
    endif()

    include(GoogleTest)
    gtest_add_tests(TARGET engine_test)
    gtest_add_tests(TARGET engine_full_test)

    # Games and saves made by released AGS 4 versions (Common/test/data/compat/make_fixtures.py)
    if(NOT ANDROID AND NOT EMSCRIPTEN)
        foreach(release 4.0.0.27 4.0.0.30)
            add_test(NAME compat_restore_${release}
                COMMAND ${CMAKE_COMMAND}
                    -DENGINE=$<TARGET_FILE:ags>
                    -DFIXTURE=${CMAKE_SOURCE_DIR}/Common/test/data/compat/${release}
                    -DWORK=${CMAKE_CURRENT_BINARY_DIR}/compat/${release}
                    -P ${CMAKE_CURRENT_SOURCE_DIR}/test/compat_restore.cmake)
        endforeach()
    endif()
endif()

# macOS App Bundle
# -----------------------------------------------------------------------------

if (MACOS)
    add_executable(ags-app)

    set_target_properties(ags-app PROPERTIES
        CXX_STANDARD 17
        CXX_EXTENSIONS NO
    )

    target_include_directories(ags-app PRIVATE .)

    file(GLOB RESOURCE_FILES
        CONFIGURE_DEPENDS
        ../OSX/Resources/ags.icns
        ../OSX/Resources/*.ags
        ../OSX/Resources/acsetup.cfg
        ../OSX/Resources/audio.vox
        ../OSX/Resources/music.vox
        ../OSX/Resources/speech.vox
        ../OSX/Resources/*.vox
        ../OSX/Resources/PrivacyInfo.xcprivacy
    )

    target_sources(ags-app
        PRIVATE
        main/main_sdl2.cpp
        ${RESOURCE_FILES}
    )

    target_link_libraries(ags-app PRIVATE engine SDL2::SDL2main)

    set_target_properties(ags-app PROPERTIES
        RUNTIME_OUTPUT_NAME AGS
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        MACOSX_BUNDLE TRUE
        RESOURCE "${RESOURCE_FILES}"  # *must* be part of target sources too

        # Below properties will configure the default template MacOSXBundleInfo.plist.in
        # Refer to https://cmake.org/cmake/help/latest/prop_tgt/MACOSX_BUNDLE_INFO_PLIST.html
        # if you want to manage a custom plist template file.

        MACOSX_BUNDLE_GUI_IDENTIFIER        # CFBundleIdentifier
            uk.co.adventuregamestudio.ags
        MACOSX_BUNDLE_BUNDLE_NAME           # CFBundleName
            AGS
        MACOSX_BUNDLE_ICON_FILE             # CFBundleIconFile
            ags.icns
        MACOSX_BUNDLE_BUNDLE_VERSION        # CFBundleVersion
            1.0.0
        MACOSX_BUNDLE_LONG_VERSION_STRING   # CFBundleLongVersionString
            1.0.0
        MACOSX_BUNDLE_SHORT_VERSION_STRING  # CFBundleShortVersionString
            1.0.0
        MACOSX_BUNDLE_COPYRIGHT             # NSHumanReadableCopyright
            "Copyright (C) 1999-2011 Chris Jones and 2011-2026 others"
        MACOSX_BUNDLE_INFO_STRING           # CFBundleGetInfoString
            "Created by Adventure Game Studio"
    )
endif()
