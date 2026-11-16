//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

//
// Engine initialization
//

#include <cerrno>
#include <cstdio>
#include <stdexcept>

#include "platform/platform.h"
#if AGS_PLATFORM_OS_WINDOWS
#include <process.h> // _spawnl
#endif
#include <allegro.h> // allegro_install and _exit

#include "data/assetmanager.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "debug/out.h"
#include "device/mousew32.h"
#include "device/sys_events.h"
#include "font/agsfontrenderer.h"
#include "font/fonts.h"
#include "game/asset_helper.h"
#include "game/character_info.h"
#include "game/dialog/lipsync.h"
#include "game/dialog/speech.h"
#include "game/dynobj/script_objects.h"
#include "game/dynobj/script_system.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_init.h"
#include "game/game_state.h"
#include "game/input/joystick.h"
#include "game/navigation/route_finder.h"
#include "game/render/draw.h"
#include "game/script.h"
#include "game/system.h"
#include "game/translation.h"
#include "game/ui/gui.h"
#include "game/world/character.h"
#include "game/world/character_extras.h"
#include "game/world/object.h"
#include "game/world/room_context.h"
#include "game/world/room_status.h"
#include "game/world/view_frame.h"
#include "gfx/ddb.h"
#include "gfx/gfxdriverfactory.h"
#include "gfx/graphicsdriver.h"
#include "gfx/image_file.h"
#include "gfx/sprite_cache.h"
#include "main/config.h"
#include "main/engine.h"
#include "main/engine_setup.h"
#include "main/game_file.h"
#include "main/game_start.h"
#include "main/graphics_mode.h"
#include "main/main.h"
#include "media/audio/audio_core.h"
#include "media/audio/sound.h"
#include "platform/base/agsplatformdriver.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "platform/base/sys_main.h"
#include "script/script_runtime.h"
#include "util/common.h"
#include "util/directory.h"
#include "util/error.h"
#include "util/path.h"
#include "util/path_helper.h"
#include "util/string_utils.h"

using namespace AGS;
using namespace AGS::Common;
using namespace AGS::Engine;


t_engine_pre_init_callback engine_pre_init_callback = nullptr;

bool engine_init_backend()
{
    EngineServices &services = EngineServices::Current();
    set_our_eip(-200);
    services.Platform->PreBackendInit();
    // Initialize SDL
    Debug::Printf(kDbgMsg_Info, "Initializing backend libs");
    if (sys_main_init())
    {
        const char *err = SDL_GetError();
        const char *user_hint = services.Platform->GetBackendFailUserHint();
        services.Platform->DisplayAlert("Unable to initialize SDL library.\n%s\n\n%s",
                                        (err && err[0]) ? err : "SDL provided no further information on the problem.",
                                        user_hint);
        return false;
    }
    sys_evt_register_ags_events();

    // Initialize stripped allegro library
    if (install_allegro(SYSTEM_NONE, &errno, atexit))
    {
        services.Platform->DisplayAlert("Internal error: unable to initialize stripped Allegro 4 library.");
        return false;
    }

    services.Platform->PostBackendInit();
    return true;
}

// Called when the player closes the game window: makes the engine quit
void winclosehook()
{
    EngineServices &services = EngineServices::Current();
    services.WantExit = true;
    services.AbortEngine = true;
    services.CheckDynamicSpritesAtExit = 0;
    AbortGame();
}

void engine_setup_window()
{
    EngineServices &services = EngineServices::Current();
    Debug::Printf(kDbgMsg_Info, "Setting up window");

    set_our_eip(-198);
    services.Platform->SetWindowTitle(Game::Current().gamename);
    sys_window_set_icon();

    SystemConfig sys_cfg;
    sys_cfg.DisplayMode = static_cast<Size>(services.GfxDriver->GetDisplayMode());
    sys_cfg.MouseEnabled = services.Config.MouseEnabled;
    sys_set_config(sys_cfg);
    sys_evt_set_quit_callback(winclosehook);
    set_our_eip(-197);
}

// Fills map with game settings, to e.g. let setup application(s)
// display correct properties to the user
static void fill_game_properties(StringOrderMap &map)
{
    Game &game = Game::Current();
    map["title"] = game.gamename;
    map["guid"] = game.guid;
    map["legacy_uniqueid"] = StrUtil::IntToString(game.uniqueid);
    map["resolution_width"] = StrUtil::IntToString(game.GetGameRes().Width);
    map["resolution_height"] = StrUtil::IntToString(game.GetGameRes().Height);
    map["resolution_bpp"] = StrUtil::IntToString(game.GetColorDepth());
    map["render_at_screenres"] =
        StrUtil::IntToString(game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_UserDefined
                                 ? -1
                                 : (game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_Enabled ? 1 : 0));
}

// Runs the setup application, if the platform has one built in (Windows only).
// There the result is always FALSE: when the player chose to run the game, it is started
// in a new process, and this one should end either way.
// Other platforms have no setup application, and the result is TRUE (continue running the game).
bool engine_run_setup(const ConfigTree &cfg, const ConfigTree &def_cfg)
{
#if AGS_PLATFORM_OS_WINDOWS
    {
        Debug::Printf(kDbgMsg_Info, "Running Setup");

        ConfigTree def_cfg_with_meta = def_cfg;
        fill_game_properties(def_cfg_with_meta["gameproperties"]);
        ConfigTree cfg_out;
        SetupReturnValue res = EngineServices::Current().Platform->RunSetup(cfg, def_cfg_with_meta, cfg_out);
        if (res != kSetup_Cancel)
        {
            String cfg_file = PreparePathForWriting(GetGameUserConfigDir(), DefaultConfigFileName);
            if (cfg_file.IsEmpty())
            {
                EngineServices::Current().Platform->DisplayAlert(
                    "Unable to write into directory '%s'.\n%s", GetGameUserConfigDir().FullDir.GetCStr(),
                    EngineServices::Current().Platform->GetDiskWriteAccessTroubleshootingText());
            }
            else if (!IniUtil::Merge(cfg_file, cfg_out))
            {
                EngineServices::Current().Platform->DisplayAlert(
                    "Unable to write to the configuration file (error code 0x%08X).\n%s",
                    EngineServices::Current().Platform->GetLastSystemError(),
                    EngineServices::Current().Platform->GetDiskWriteAccessTroubleshootingText());
            }
        }
        if (res != kSetup_RunGame)
            return false;

        // Start the game in the new process, and close the current one afterwards
        String args = String::FromFormat("\"%s\"", EngineServices::Current().AppPath.GetCStr());
        _spawnl(_P_NOWAIT, EngineServices::Current().AppPath.GetCStr(), args.GetCStr(), NULL);
        return false;
    }
#else
    (void)cfg;
    (void)def_cfg;
#endif
    return true;
}

// Scans given directory for the AGS game config. If such config exists
// and it contains data file name, then returns one.
// Otherwise returns empty string.
static String find_game_data_in_config(const String &path)
{
    // First look for config
    ConfigTree cfg;
    String def_cfg_file = Path::ConcatPaths(path, DefaultConfigFileName);
    if (IniUtil::Read(def_cfg_file, cfg))
    {
        String data_file = CfgReadString(cfg, "misc", "datafile");
        Debug::Printf("Found game config: %s", def_cfg_file.GetCStr());
        Debug::Printf(" Cfg: data file: %s", data_file.GetCStr());
        // Only accept if it's a relative path
        if (!data_file.IsEmpty() && Path::IsRelativePath(data_file))
            return Path::ConcatPaths(path, data_file);
    }
    return ""; // not found in config
}

// Scans for game data in several common locations.
// When it does so, it first looks for game config file, which contains
// explicit directions to game data in its settings.
// If such config is not found, it scans same location for *any* game data instead.
String search_for_game_data_file(String &was_searching_in)
{
    EngineServices &services = EngineServices::Current();
    Debug::Printf("Looking for the game data.\n Cwd: %s\n Path arg: %s", Directory::GetCurrentDirectory().GetCStr(),
                  services.CmdGameDataPath.GetCStr());
    // 1. From command line argument, which may be a directory or actual file
    if (!services.CmdGameDataPath.IsEmpty())
    {
        if (File::IsFile(services.CmdGameDataPath))
            return services.CmdGameDataPath; // this path is a file
        if (!File::IsDirectory(services.CmdGameDataPath))
            return ""; // path is neither file nor directory
        was_searching_in = services.CmdGameDataPath;
        Debug::Printf("Searching in (cmd arg): %s", was_searching_in.GetCStr());
        // first scan for config
        String data_path = find_game_data_in_config(services.CmdGameDataPath);
        if (!data_path.IsEmpty())
            return data_path;
        // if not found in config, lookup for data in same dir
        return FindGameData(services.CmdGameDataPath);
    }

    // 2. Look in other known locations
    // 2.1. Look for attachment in the running executable
    if (!services.AppPath.IsEmpty() && Common::AssetManager::IsDataFile(services.AppPath))
    {
        Debug::Printf("Found game data embedded in executable");
        was_searching_in = Path::GetDirectoryPath(services.AppPath);
        return services.AppPath;
    }

    // 2.2 Look in current working directory
    String cur_dir = Directory::GetCurrentDirectory();
    was_searching_in = cur_dir;
    Debug::Printf("Searching in (cwd): %s", was_searching_in.GetCStr());
    // first scan for config
    String data_path = find_game_data_in_config(cur_dir);
    if (!data_path.IsEmpty())
        return data_path;
    // if not found in config, lookup for data in same dir
    data_path = FindGameData(cur_dir);
    if (!data_path.IsEmpty())
        return data_path;

    // 2.3 Look in executable's directory (if it's different from current dir)
    if (Path::ComparePaths(services.AppDirectory, cur_dir) == 0)
        return ""; // no luck
    was_searching_in = services.AppDirectory;
    Debug::Printf("Searching in (exe dir): %s", was_searching_in.GetCStr());
    // first scan for config
    data_path = find_game_data_in_config(services.AppDirectory);
    if (!data_path.IsEmpty())
        return data_path;
    // if not found in config, lookup for data in same dir
    return FindGameData(services.AppDirectory);
}

void engine_init_fonts()
{
    Debug::Printf(kDbgMsg_Info, "Initializing TTF renderer");

    init_font_renderer(EngineServices::Current().Assets.get());
}

void engine_init_mouse()
{
    int res = Mouse::GetButtonCount();
    if (res < 0)
        Debug::Printf(kDbgMsg_Info, "Initializing mouse: failed");
    else
        Debug::Printf(kDbgMsg_Info, "Initializing mouse: number of buttons reported is %d", res);
    Mouse::SetSpeed(EngineServices::Current().Config.MouseSpeed);
}

void engine_locate_speech_pak()
{
    init_voicepak("");
}

void engine_locate_audio_pak()
{
    EngineServices &services = EngineServices::Current();
    String music_file = Game::Current().GetAudioVOXName();
    String music_filepath = find_assetlib(music_file);
    if (!music_filepath.IsEmpty())
    {
        if (services.Assets->AddLibrary(music_filepath) == kAssetNoError)
        {
            Debug::Printf(kDbgMsg_Info, "%s found and initialized.", music_file.GetCStr());
            services.ResPaths.AudioPak.Name = music_file;
            services.ResPaths.AudioPak.Path = music_filepath;
        }
        else
        {
            services.Platform->DisplayAlert(
                "Unable to initialize digital audio pack '%s', file could be corrupt or of unsupported format.",
                music_file.GetCStr());
        }
        return;
    }

    for (const auto &opt_dir : services.ResPaths.OptDataDirs)
    {
        if (opt_dir.second.FindSection("audio", ',') != String::NoIndex)
        {
            Debug::Printf(kDbgMsg_Info, "Audio pack was not found, but explicit audio directory is defined.");
            return;
        }
    }
}

// Assign asset locations to the AssetManager
void engine_assign_assetpaths()
{
    EngineServices &services = EngineServices::Current();
    services.Assets->AddLibrary(services.ResPaths.GamePak.Path, ",audio"); // main pack may have audio bundled too
    // The asset filters are currently a workaround for limiting search to certain locations;
    // this is both an optimization and to prevent unexpected behavior.
    // - empty filter is for regular files
    // audio - audio clips
    // voice - voice-over clips
    // NOTE: we add extra optional directories first because they should have higher priority
    // TODO: maybe change AssetManager library order to stack-like later (last added = top priority)?
    for (const auto &opt_dir : services.ResPaths.OptDataDirs)
    {
        if (Path::ComparePaths(opt_dir.first, services.ResPaths.DataDir) != 0)
            services.Assets->AddLibrary(opt_dir.first, opt_dir.second);
    }

    services.Assets->AddLibrary(services.ResPaths.DataDir, ",audio,voice"); // dir may have anything
    if (!services.ResPaths.AudioPak.Path.IsEmpty())
        services.Assets->AddLibrary(services.ResPaths.AudioPak.Path, "audio");
    if (!services.ResPaths.SpeechPak.Path.IsEmpty())
        services.Assets->AddLibrary(services.ResPaths.SpeechPak.Path, "voice");

    // Print all the registered asset libraries
    Debug::Printf("Registered asset libraries:");
    for (size_t i = 0; i < services.Assets->GetLibraryCount(); ++i)
    {
        const auto lib = services.Assets->GetLibraryEntry(i);
        Debug::Printf(String::FromFormat("- %s:\t%s", lib.IsDirectory ? "DIR" : "LIB", lib.Path.GetCStr()));
        Debug::Printf("\tFilters: %s", lib.Filters.GetCStr());
        if (lib.LibFiles.size() > 1)
        {
            Debug::Printf("\tSub-files:");
            for (size_t j = 1; j < lib.LibFiles.size(); ++j)
                Debug::Printf("\t%zu:\t%s", j, lib.LibFiles[j].GetCStr());
        }
    }
}

void engine_init_keyboard()
{
    /* do nothing */
}

void engine_init_audio()
{
    EngineServices &services = EngineServices::Current();
    if (services.Config.AudioEnabled)
    {
        Debug::Printf("Initializing audio");
        bool res = sys_audio_init(services.Config.AudioDriverID);
        if (res)
        {
            try
            {
                audio_core_init(); // audio core system
            }
            catch (std::runtime_error &ex)
            {
                Debug::Printf(kDbgMsg_Error, "Failed to initialize audio system: %s", ex.what());
                res = false;
            }
        }
        services.Config.AudioEnabled = res;
    }

    if (services.Config.AudioEnabled)
    {
        soundcache_set_rules(services.Config.SoundLoadAtOnceSize * 1024, services.Config.SoundCacheSize * 1024);
    }
    else
    {
        // all audio is disabled
        Debug::Printf(kDbgMsg_Info, "Audio is disabled");
    }
}

void engine_init_debug()
{
    EngineServices &services = EngineServices::Current();
    if (services.Config.ShowFps)
        services.Debug.DisplayFps = kFPS_Forced;
    if ((services.Debug.Flags & (~(DBG_DEBUGMODE | DBG_DBGSCRIPT))) > 0)
    {
        services.Platform->DisplayAlert("Engine debugging mode enabled.\n"
                                        "\nNOTE: You have selected to enable one or more engine debugging options. "
                                        "These options cause many parts of the game to behave abnormally, and you "
                                        "may not see the game as you are used to it. The point is to test whether "
                                        "the engine passes a point where it is crashing on you normally.\n"
                                        "[Debug flags enabled: 0x%02X]",
                                        services.Debug.Flags);
    }
}

// Reports an exit that did not go through the engine's own quit procedure
void atexit_handler()
{
    EngineServices &services = EngineServices::Current();
    if (services.ProperExit == 0)
    {
        services.Platform->DisplayAlert("Error: the program has exited without requesting it.\n"
                                        "Program pointer: %+03d  (write this number down), engine version %s\n"
                                        "If you see a list of numbers above, please write them down and contact\n"
                                        "developers. Otherwise, note down any other information displayed.",
                                        get_our_eip(), services.Version.LongString.GetCStr());
    }
}

void engine_init_exit_handler()
{
    Debug::Printf(kDbgMsg_Info, "Install exit handler");

    atexit(atexit_handler);
}

int engine_load_game_data()
{
    Debug::Printf("Load game data");
    set_our_eip(-17);
    HError err = load_game_file();
    if (!err)
    {
        EngineServices::Current().ProperExit = 1;
        display_game_file_error(err);
        return EXIT_ERROR;
    }
    // Assign the default bitmap color depth, equals this game's color depth
    Bitmap::SetColorDepth(Game::Current().GetColorDepth());

    // Checks and warnings
    if (!is_any_font_loaded())
    {
        Debug::Printf(kDbgMsg_Warn, "WARNING: No game fonts were loaded on startup. The game won't be able to display "
                                    "any text unless fonts are initialized at runtime later (e.g. by a plugin).");
    }
    return 0;
}

// Replace special tokens inside a user path option
static void resolve_configured_path(String &option)
{
    option.Replace("$GAMENAME$", Game::Current().gamename);
}

// Setup paths and directories that may be affected by user configuration
void engine_init_user_directories()
{
    EngineServices &services = EngineServices::Current();
    resolve_configured_path(services.Config.UserSaveDir);
    resolve_configured_path(services.Config.AppDataDir);
    if (!services.Config.UserConfDir.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "User config directory: %s", services.Config.UserConfDir.GetCStr());
    if (!services.Config.UserSaveDir.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "User data directory: %s", services.Config.UserSaveDir.GetCStr());
    if (!services.Config.AppDataDir.IsEmpty())
        Debug::Printf(kDbgMsg_Info, "Shared data directory: %s", services.Config.AppDataDir.GetCStr());

    // Initialize default save directory early, for we'll need it to set restart point
    SetDefaultSaveDirectory();
}

// Checks that the save game directory has at least 2 MB of free space and that
// a file can be created and deleted in it. Returns 1 if so, otherwise 0.
int check_write_access()
{
    set_our_eip(-1895);

    // The Save Game Dir is the only place that we should write to
    String svg_dir = get_save_game_directory();
    if (EngineServices::Current().Platform->GetDiskFreeSpaceMB(svg_dir) < 2)
        return 0;
    String temp_path = String::FromFormat("%s"
                                          "tmptest.tmp",
                                          svg_dir.GetCStr());
    auto temp_s = File::CreateFile(temp_path);
    if (!temp_s)
        return 0;

    set_our_eip(-1896);

    temp_s->Write("just to test the drive free space", 30);
    temp_s.reset();

    set_our_eip(-1897);

    if (!File::DeleteFile(temp_path))
        return 0;

    return 1;
}

int engine_check_disk_space()
{
    EngineServices &services = EngineServices::Current();
    Debug::Printf(kDbgMsg_Info, "Checking for disk space");

    if (check_write_access() == 0)
    {
        services.Platform->DisplayAlert("Unable to write in the savegame directory.\n%s",
                                        services.Platform->GetDiskWriteAccessTroubleshootingText());
        services.ProperExit = 1;
        return EXIT_ERROR;
    }

    return 0;
}

// Shows the "preload" image from the game assets, if there is one, for half a second
void show_preload()
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    UStream stream;
    String f_ext;
    for (const auto &ext : ImageFile::GetSupportedImageExts())
    {
        f_ext = ext;
        const auto asset_name = String::FromFormat("preload.%s", f_ext.GetCStr());
        stream = services.Assets->OpenAsset(asset_name);
        if (stream)
            break;
    }
    if (!stream)
        return;

    RGB temp_pal[256];
    Bitmap *splash_bmp = BitmapHelper::LoadBitmap(stream.get(), f_ext.GetCStr(), nullptr, temp_pal);
    if (splash_bmp != nullptr)
    {
        Debug::Printf("Displaying preload image");
        if (splash_bmp->GetColorDepth() == 8)
            set_palette_range(temp_pal, 0, 255, 0);
        if (services.GfxDriver->UsesMemoryBackBuffer())
            services.GfxDriver->GetMemoryBackBuffer()->Clear();

        const Rect &view = game.Viewports.GetMainViewport();
        Bitmap *preload_bmp = BitmapHelper::CreateBitmapCopy(splash_bmp, game.GetColorDepth());
        if (!services.GfxDriver->HasAcceleratedTransform() && view.GetSize() != preload_bmp->GetSize())
        {
            Bitmap *stretched = new Bitmap(view.GetWidth(), view.GetHeight(), preload_bmp->GetColorDepth());
            stretched->StretchBlt(preload_bmp, RectWH(0, 0, view.GetWidth(), view.GetHeight()));
            delete preload_bmp;
            preload_bmp = stretched;
        }
        IDriverDependantBitmap *ddb = services.GfxDriver->CreateDDBFromBitmap(preload_bmp, kTxFlags_Opaque);
        ddb->SetStretch(view.GetWidth(), view.GetHeight());
        services.GfxDriver->ClearDrawLists();
        services.GfxDriver->BeginSpriteBatch(view, SpriteTransform());
        services.GfxDriver->DrawSprite(0, 0, ddb);
        services.GfxDriver->EndSpriteBatch();
        render_to_screen();
        services.GfxDriver->DestroyDDB(ddb);
        delete splash_bmp;
        delete preload_bmp;
        services.Platform->Delay(500);
    }
}

// Opens the sprite file and sets up the sprite cache
HError engine_init_sprites()
{
    EngineServices &services = EngineServices::Current();
    services.Sprites.Reset();
    Debug::Printf(kDbgMsg_Info, "Initialize sprites");
    auto sprite_file = services.Assets->OpenAsset(SpriteFile::DefaultSpriteFileName);
    if (!sprite_file)
    {
        return new Error("Failed to open spriteset file '%s'.", SpriteFile::DefaultSpriteFileName.GetCStr());
    }
    auto index_file = services.Assets->OpenAsset(SpriteFile::DefaultSpriteIndexName);
    HError err = services.Sprites.InitFile(std::move(sprite_file), std::move(index_file));
    if (!err)
    {
        return err;
    }

    const char *compress_desc =
        StrUtil::SelectCStr<kNumSprCompressTypes>(CstrArr<kNumSprCompressTypes>{"none", "rle", "lzw", "deflate"},
                                                  services.Sprites.GetSpriteCompression(), "unknown");
    Debug::Printf("Sprite file info: compression: %s, storage flags: 0x%08x, total sprites: %zu", compress_desc,
                  services.Sprites.GetStoreFlags(), services.Sprites.GetSpriteSlotCount());
    if (services.Config.SpriteCacheSize > 0)
        services.Sprites.SetMaxCacheSize(services.Config.SpriteCacheSize * 1024);
    Debug::Printf("Sprite cache set: %zu KB", services.Sprites.GetMaxCacheSize() / 1024);
    return HError::None();
}

// Sets the runtime game state defaults and applies the game options to it
// TODO: this should not be a part of "engine_" function group,
// move this elsewhere (InitGameState?).
void engine_init_game_settings()
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    Room &room = Room::Current();
    set_our_eip(-7);
    Debug::Printf("Initialize game settings");

    // Initialize randomizer
    game.Play.randseed = time(nullptr);
    srand(game.Play.randseed);

    if (services.Config.AudioEnabled)
    {
        services.SeparateMusicLib = !services.ResPaths.AudioPak.Name.IsEmpty();
        services.VoiceAvail = services.ResPaths.VoiceAvail;
    }
    else
    {
        services.VoiceAvail = false;
        services.SeparateMusicLib = false;
    }

    // Since 4.0 AGS engine always works in UTF-8 mode
    set_uformat(U_UTF8);
    String game_encoding;
    if (game.options[OPT_GAMETEXTENCODING] == 65001) // utf-8 codepage number
        game_encoding = "UTF-8";
    else
        game_encoding = "ASCII";
    Debug::Printf("Game text encoding: %s, text language: %s", game_encoding.GetCStr(),
                  game.GameTextLanguage.IsEmpty() ? "not set" : game.GameTextLanguage.GetCStr());
    if (game.options[OPT_GAMETEXTENCODING] != 65001)
        Debug::Printf(kDbgMsg_Warn, "WARNING: game's text encoding is not UTF-8, and may be displayed incorrectly");

    int i;

    for (i = 0; i < 256; i++)
    {
        if (game.paluses[i] != PAL_BACKGROUND)
            game.Palette[i] = game.defpal[i];
    }
    // Allegro's palette starts all black; colours are matched against it (makecol8)
    setpal();

    for (i = 0; i < game.numcursors; i++)
    {
        // The cursor graphics are assigned to mousecurs[] and so cannot
        // be removed from memory
        if (game.mcurs[i].pic >= 0)
            services.Sprites.PrecacheSprite(game.mcurs[i].pic);

        // just in case they typed an invalid view number in the editor
        if (game.mcurs[i].view >= game.numviews)
            game.mcurs[i].view = -1;

        if (game.mcurs[i].view >= 0)
            precache_view(game.mcurs[i].view);
    }
    // may as well preload the character gfx
    if (game.PlayerChar->view >= 0)
        precache_view(game.PlayerChar->view, 0, Character_GetDiagonalWalking(game.PlayerChar) ? 8 : 4);

    set_our_eip(-6);

    for (i = 0; i < MAX_ROOM_OBJECTS; i++)
    {
        game.ScObjects[i].id = i;
    }

    for (i = 0; i < game.numcharacters; i++)
    {
        game.Characters[i].activeinv = -1;
        game.Characters[i].idleleft = game.Characters[i].idledelay;
        game.Characters[i].walkwaitcounter = 0;
        game.Characters[i].z = 0;
        game.CharExtras[i].xwas = INVALID_X;
        game.CharExtras[i].zoom = 100;
        if (game.Characters[i].view >= 0)
        {
            // set initial loop to 0
            game.Characters[i].loop = 0;
            // or to 1 if they don't have up/down frames
            if (game.Views[game.Characters[i].view].loops[0].numFrames < 1)
                game.Characters[i].loop = 1;
        }
        game.CharExtras[i].process_idle_this_time = 0;
        game.CharExtras[i].slow_move_counter = 0;
        game.CharExtras[i].animwait = 0;
    }

    set_our_eip(-5);
    // Add all inventory items with "starts with" flag to the player character
    for (i = 0; i < game.numinvitems; i++)
    {
        if (game.invinfo[i].flags & IFLG_STARTWITH)
            game.PlayerChar->set_item_quantity(i, 1);
    }

    //
    // TODO: following big initialization sequence could be in GameState ctor
    // copy the value set by the editor
    if (game.options[OPT_GLOBALTALKANIMSPD] >= 0)
    {
        game.Play.talkanim_speed = game.options[OPT_GLOBALTALKANIMSPD];
        game.options[OPT_GLOBALTALKANIMSPD] = 1;
    }
    else
    {
        game.Play.talkanim_speed = -game.options[OPT_GLOBALTALKANIMSPD] - 1;
        game.options[OPT_GLOBALTALKANIMSPD] = 0;
    }
    game.Play.inv_item_wid = 40;
    game.Play.inv_item_hit = 22;
    game.Play.messagetime = -1;
    game.Play.disabled_user_interface = 0;
    game.Play.gscript_timer = -1;
    game.Play.debug_mode = game.options[OPT_DEBUGMODE];
    game.Play.text_speed = 15;
    game.Play.text_min_display_time_ms = 1000;
    game.Play.ignore_user_input_after_text_timeout_ms = 500;
    game.Play.ClearIgnoreInput();
    game.Play.lipsync_speed = 15;
    game.Play.close_mouth_speech_time = 10;
    game.Play.disable_antialiasing = 0;
    game.Play.rtint_enabled = false;
    game.Play.rtint_level = 0;
    game.Play.rtint_light = 0;
    game.Play.text_speed_modifier = 0;
    game.Play.text_align = kHAlignLeft;
    game.Play.speech_text_align = kHAlignCenter;
    game.Play.dialog_options_textalign = kHAlignLeft;
    // Make the default text alignment to the right with right-to-left text;
    // except speech text, which is centered by default
    if (game.options[OPT_RIGHTLEFTWRITE])
    {
        game.Play.text_align = kHAlignRight;
        if (game.Play.GetRBSwitches()[kRBO_ApplyDialogOptionTextAlignment])
            game.Play.dialog_options_textalign = kHAlignRight;
    }

    game.Play.speech_bubble_width = 100;
    room.BgFrame = 0;
    room.BgFrameLocked = 0;
    game.Play.bg_anim_delay = 0;
    room.AnimBackgroundSpeed = 0;
    game.Play.mouse_input_enabled = true;
    game.Play.mouse_cursor_shown = true;
    game.Play.skip_until_char_stops = -1;
    game.Play.get_loc_name_last_time = {};
    game.Play.get_loc_name_save_cursor = {};
    game.Play.restore_cursor_mode_to = -1;
    game.Play.restore_cursor_image_to = -1;
    game.Play.ground_level_areas_disabled = 0;
    game.Play.next_screen_transition = -1;
    game.Play.temporarily_turned_off_character = -1;
    game.Play.gamma_adjustment = 100;
    game.Play.shakesc_length = 0;
    game.Play.wait_counter = 0;
    game.Play.SetWaitSkipResult(SKIP_NONE);
    game.Play.key_skip_wait = SKIP_NONE;
    game.Play.audio_master_volume = 100;
    game.Play.screen_flipped = 0;
    game.Play.speech_mode = kSpeech_VoiceText;
    game.Play.speech_skip_style = user_to_internal_skip_speech((SkipSpeechStyle)game.options[OPT_NOSKIPTEXT]);
    game.Play.speech_volume = 255;
    game.Play.normal_font = 0;
    game.Play.speech_font = 1;
    game.Play.speech_text_shadow = GUI::GetStandardColor(16);
    game.Play.screen_tint = -1;
    game.Play.bad_parsed_word.Empty();
    game.Play.swap_portrait_side = 0;
    game.Play.swap_portrait_lastchar = -1;
    game.Play.swap_portrait_lastlastchar = -1;
    game.Play.in_conversation = 0;
    game.Play.skip_display = kSkipSpeechKeyMouse;
    game.Play.no_multiloop_repeat = 0;
    game.Play.in_cutscene = 0;
    game.Play.fast_forward = 0;
    room.ScriptFinished = 0;
    game.Play.no_textbg_when_voice = 0;
    game.Play.max_dialogoption_width = 180;
    game.Play.no_hicolor_fadein = 0;
    game.Play.bgspeech_game_speed = 0;
    game.Play.bgspeech_stay_on_display = 0;
    game.Play.unfactor_speech_from_textlength = 0;
    game.Play.speech_music_drop = 60;
    game.Play.room_changes = 0;
    game.Play.check_interaction_only = 0;
    game.Play.replay_hotkey_unused = -1; // StartRecording: not supported.
    game.Play.dialog_options_pad_x = TEXTWINDOW_PADDING_DEFAULT;
    game.Play.dialog_options_pad_y = 0; // no Y padding by default
    game.Play.dialog_options_font = FONT_UNDEFINED;
    game.Play.min_dialogoption_width = 0;
    game.Play.disable_dialog_parser = 0;
    game.Play.screen_is_faded_out = false;
    game.Play.player_on_region = 0;
    game.Play.top_bar_backcolor = GUI::GetStandardColor(8);
    game.Play.top_bar_textcolor = GUI::GetStandardColor(16);
    game.Play.top_bar_bordercolor = GUI::GetStandardColor(8);
    game.Play.top_bar_borderwidth = 1;
    game.Play.top_bar_ypos = 25;
    game.Play.top_bar_font = -1;
    game.Play.screenshot_width = 160;
    game.Play.screenshot_height = 100;
    game.Play.auto_use_walkto_points = 1;
    game.Play.inventory_greys_out = 0;
    game.Play.skip_speech_specific_key = 0;
    game.Play.abort_key = 324; // Alt+X
    game.Play.fade_to_red = 0;
    game.Play.fade_to_green = 0;
    game.Play.fade_to_blue = 0;
    game.Play.show_single_dialog_option = game.options[OPT_DISPLAYSINGLEDIALOGOPTION];
    game.Play.keep_screen_during_instant_transition = 0;
    game.Play.read_dialog_option_colour = -1;
    game.Play.stop_dialog_at_end = DIALOG_NONE;
    game.Play.speech_portrait_placement = 0;
    game.Play.speech_portrait_x = 0;
    game.Play.speech_portrait_y = 0;
    game.Play.speech_display_post_time_ms = 0;
    game.Play.dialog_options_highlight_color = GUI::GetStandardColor(DIALOG_OPTIONS_HIGHLIGHT_COLOR_DEFAULT);
    game.Play.speech_has_voice = false;
    game.Play.speech_voice_blocking = false;
    game.Play.speech_in_post_state = false;
    game.Play.complete_overlay_on = 0;
    game.Play.text_overlay_on = 0;
    game.Play.narrator_speech = game.playercharacter;
    game.Play.crossfading_out_channel = 0;
    game.Play.speech_textwindow_gui = game.options[OPT_TWCUSTOM];
    if (game.Play.speech_textwindow_gui == 0)
        game.Play.speech_textwindow_gui = -1;
    game.Play.game_name = game.gamename;
    game.Play.lastParserEntry[0] = 0;
    game.Play.follow_change_room_timer = 150;
    game.Play.game_speed_modifier = 0;
    if (services.Debug.Flags & DBG_DEBUGMODE)
        game.Play.debug_mode = 1;
    game.Play.shake_screen_yoff = 0;
    game.Play.face_dir_ratio = game.faceDirectionRatio;

    GUI::DataVersion = game.LoadedDataVersion;
    GUI::Options.DisabledStyle = static_cast<GuiDisableStyle>(game.options[OPT_DISABLEOFF]);
    GUI::Options.ClipControls = game.options[OPT_CLIPGUICONTROLS] != 0;
    // Force GUI metrics recalculation, accomodating for loaded fonts
    GUIE::MarkForFontUpdate(-1);

    memset(&room.WalkableAreasOn[0], 1, MAX_WALK_AREAS);
    game.Play.default_audio_type_volumes.resize(game.audioClipTypes.size(), -1);

    if (!services.Config.Translation.IsEmpty())
        Game_ChangeTranslation(services.Config.Translation.GetCStr());

    UpdateInventory();
    game.DisplayedRoom = -10;

    set_our_eip(-4);
    services.Input.MouseY = 100; // stop icon bar popping up

    set_game_speed(game.options[OPT_GAMEFPS]);

    // We use same variable to read config and be used at runtime for now,
    // so update it here with regards to game design option
    services.Config.RenderAtScreenRes =
        (game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_UserDefined && services.Config.RenderAtScreenRes) ||
        game.options[OPT_RENDERATSCREENRES] == kRenderAtScreenRes_Enabled;

    // TODO: InitGameState already applies these once, but the defaults set above come after it
    // and overwrite part of them; the default game settings are spread across several
    // functions, and this repeated call stays until they are gathered in one place.
    ApplyAccessibilityOptions(game.Play, services.Config);
    // Applies behavior options, also hacks and backwards compatibility fixes
    ApplyBehaviorOptions(game, game.Play, services.Config);
    PrintBehaviorOptions(game, game.Play);
}

void engine_setup_scsystem_auxiliary()
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    if (services.Config.Override.ScriptOS > eOS_Unknown)
    {
        game.ScSystem.os = services.Config.Override.ScriptOS;
    }
    else
    {
        game.ScSystem.os = services.Platform->GetSystemOSID();
    }
}

void engine_prepare_to_start_game()
{
    EngineServices &services = EngineServices::Current();
    Debug::Printf("Prepare to start game");

    engine_setup_scsystem_auxiliary();

    if (services.Config.LoadLatestSave)
    {
        int slot = GetLastSaveSlot();
        if (slot >= 0)
            services.LoadSaveGameOnStartup = get_save_game_path(slot);
    }
}

// Define location of the game data either using direct settings or searching
// for the available resource packs in common locations.
// Returns two paths:
// - startup_dir: this is where engine found game config and/or data;
// - data_path: full path of the main data pack;
// data_path's directory (may or not be eq to startup_dir) should be considered data directory,
// and this is where engine look for all game data.
HError define_gamedata_location_checkall(String &data_path, String &startup_dir)
{
    EngineServices &services = EngineServices::Current();
    // First try if they provided a startup option
    if (!services.CmdGameDataPath.IsEmpty())
    {
        // If not a valid path - bail out
        if (!File::IsFileOrDir(services.CmdGameDataPath))
            return new Error("Provided game location is not a valid path.\n Cwd: %s\n Path: %s",
                             Directory::GetCurrentDirectory().GetCStr(), services.CmdGameDataPath.GetCStr());
        // If it's a file, then keep it and proceed
        if (File::IsFile(services.CmdGameDataPath))
        {
            Debug::Printf("Using provided game data path: %s", services.CmdGameDataPath.GetCStr());
            startup_dir = Path::GetDirectoryPath(services.CmdGameDataPath);
            data_path = services.CmdGameDataPath;
            return HError::None();
        }
    }

#if AGS_SEARCH_FOR_GAME_ON_LAUNCH
    // No direct filepath provided, search in common locations.
    data_path = search_for_game_data_file(startup_dir);
    if (data_path.IsEmpty())
    {
        if (startup_dir.IsEmpty())
            return new Error("Engine was not able to find any compatible game data.");
        else
            return new Error("Engine was not able to find any compatible game data.\nSearched in: %s",
                             startup_dir.GetCStr());
    }
    data_path = Path::MakeAbsolutePath(data_path);
    Debug::Printf(kDbgMsg_Info, "Located game data pak: %s", data_path.GetCStr());
    return HError::None();
#else
    data_path = services.Platform->GetGameDataFile();
    if (!data_path.IsEmpty())
    {
        return HError::None();
    }
    return new Error("The game location was not defined by startup settings.");
#endif
}

// Define location of the game data
bool define_gamedata_location()
{
    EngineServices &services = EngineServices::Current();
    String data_path, startup_dir;
    HError err = define_gamedata_location_checkall(data_path, startup_dir);
    if (!err)
    {
        services.Platform->DisplayAlert("ERROR: Unable to determine game data.\n%s", err->FullMessage().GetCStr());
        main_print_help();
        return false;
    }

    // On success: set all the necessary path and filename settings
    services.Config.StartupDir = startup_dir;
    services.Config.MainDataFile = data_path;
    services.Config.MainDataDir = Path::GetDirectoryPath(data_path);
    return true;
}

// Find and preload main game data
bool engine_init_gamedata()
{
    EngineServices &services = EngineServices::Current();
    Debug::Printf(kDbgMsg_Info, "Initializing game data");
    // First, find data location
    if (!define_gamedata_location())
        return false;

    // Try init game lib
    AssetError asset_err = services.Assets->AddLibrary(services.Config.MainDataFile);
    if (asset_err != kAssetNoError)
    {
        services.Platform->DisplayAlert(
            "ERROR: The game data is missing, is of unsupported format or corrupt.\nFile: '%s'",
            services.Config.MainDataFile.GetCStr());
        return false;
    }

    // Pre-load game name and savegame folder names from data file
    // TODO: research if that is possible to avoid this step and just
    // read the full head game data at this point. This might require
    // further changes of the order of initialization.
    HError err = preload_game_data();
    if (!err)
    {
        display_game_file_error(err);
        return false;
    }

    // Setup ResPaths, so that we know our main locations further
    services.ResPaths.GamePak.Path = services.Config.MainDataFile;
    services.ResPaths.GamePak.Name = Path::GetFilename(services.Config.MainDataFile);
    services.ResPaths.DataDir = services.Config.OptInstallDir.IsEmpty()
                                    ? services.Config.MainDataDir
                                    : Path::MakeAbsolutePath(services.Config.OptInstallDir);
    for (const auto &opt_dir : services.Config.OptDataDirs)
    {
        services.ResPaths.OptDataDirs.emplace_back(Path::MakeAbsolutePath(opt_dir.first), opt_dir.second);
    }

    Debug::Printf(kDbgMsg_Info, "Startup directory: %s", services.Config.StartupDir.GetCStr());
    Debug::Printf(kDbgMsg_Info, "Data directory: %s", services.ResPaths.DataDir.GetCStr());
    for (const auto &opt_dir : services.ResPaths.OptDataDirs)
    {
        Debug::Printf(kDbgMsg_Info, "Opt data directory: %s\n\tasset filter: %s", opt_dir.first.GetCStr(),
                      opt_dir.second.GetCStr());
    }
    return true;
}

void engine_read_config(ConfigTree &cfg, ConfigTree *def_cfg)
{
    EngineServices &services = EngineServices::Current();
    if (!services.Config.UserConfPath.IsEmpty())
    {
        IniUtil::Read(services.Config.UserConfPath, cfg);
        if (def_cfg)
            *def_cfg = cfg;
        return;
    }

    // Read default configuration file
    String def_cfg_file = find_default_cfg_file();
    IniUtil::Read(def_cfg_file, cfg);
    if (def_cfg)
        *def_cfg = cfg;

    String user_global_cfg_file;
    // Read user global configuration file
    user_global_cfg_file = find_user_global_cfg_file();
    if (Path::ComparePaths(user_global_cfg_file, def_cfg_file) != 0)
        IniUtil::Read(user_global_cfg_file, cfg);

    // Handle directive to search for the user config inside the custom directory;
    // this option may come either from command line or default/global config.
    if (services.Config.UserConfDir.IsEmpty())
        services.Config.UserConfDir = CfgReadString(cfg, "misc", "user_conf_dir");
    if (services.Config.UserConfDir.IsEmpty()) // also try deprecated option
        services.Config.UserConfDir = CfgReadBoolInt(cfg, "misc", "localuserconf") ? "." : "";
    // Test if the file is writeable, if it is then both engine and setup
    // applications may actually use it fully as a user config, otherwise
    // fallback to default behavior.
    if (!services.Config.UserConfDir.IsEmpty())
    {
        resolve_configured_path(services.Config.UserConfDir);
        if (Path::IsRelativePath(services.Config.UserConfDir))
            services.Config.UserConfDir = Path::ConcatPaths(services.Config.StartupDir, services.Config.UserConfDir);
        if (!Directory::CreateDirectory(services.Config.UserConfDir) ||
            !File::TestWriteFile(Path::ConcatPaths(services.Config.UserConfDir, DefaultConfigFileName)))
        {
            Debug::Printf(kDbgMsg_Warn, "Write test failed at user config dir '%s', using default path.",
                          services.Config.UserConfDir.GetCStr());
            services.Config.UserConfDir = "";
        }
    }

    // Read user configuration file
    String user_cfg_file = find_user_cfg_file();
    if (Path::ComparePaths(user_cfg_file, def_cfg_file) != 0 &&
        Path::ComparePaths(user_cfg_file, user_global_cfg_file) != 0)
        IniUtil::Read(user_cfg_file, cfg);

    // Apply overriding options from platform settings
    // TODO: normally, those should be instead stored in the same config file in a uniform way
    override_config_ext(cfg);
}

// Gathers settings from all available sources into single ConfigTree
void engine_prepare_config(ConfigTree &cfg, ConfigTree *def_cfg, const ConfigTree &startup_opts)
{
    Debug::Printf(kDbgMsg_Info, "Setting up game configuration");
    // Read configuration files
    engine_read_config(cfg, def_cfg);
    // Merge startup options in
    for (const auto &section : startup_opts)
        for (const auto &opt : section.second)
            cfg[section.first][opt.first] = opt.second;
}

// Applies configuration to the running game
void engine_set_config(const ConfigTree cfg)
{
    EngineServices &services = EngineServices::Current();
    config_defaults(services.Config);
    apply_config(cfg, services.Config);
    post_config(services.Config);
}

//
// --tell command support: printing engine/game info by request
//
static bool print_info_needs_game(const std::set<String> &keys)
{
    return keys.count("all") > 0 || keys.count("config") > 0 || keys.count("configpath") > 0 ||
           keys.count("data") > 0 || keys.count("filepath") > 0 || keys.count("gameproperties") > 0;
}

// Prints the requested engine and game info (the --tell keys) to stdout as an ini text;
// user_cfg is the game configuration, and the "config" key prints nothing without it
static void engine_print_info(const std::set<String> &keys, ConfigTree *user_cfg)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    const bool all = keys.count("all") > 0;
    ConfigTree data;
    if (all || keys.count("engine") > 0)
    {
        data["engine"]["name"] = get_engine_name();
        data["engine"]["version"] = get_engine_version();
    }
    if (all || keys.count("graphicdriver") > 0)
    {
        StringV drivers;
        AGS::Engine::GetGfxDriverFactoryNames(drivers);
        for (size_t i = 0; i < drivers.size(); ++i)
        {
            data["graphicdriver"][String::FromFormat("%zu", i)] = drivers[i];
        }
    }
    if (all || keys.count("configpath") > 0)
    {
        String def_cfg_file = find_default_cfg_file();
        String global_cfg_file = find_user_global_cfg_file();
        String user_cfg_file = find_user_cfg_file();
        data["configpath"]["default"] = def_cfg_file;
        data["configpath"]["global"] = global_cfg_file;
        data["configpath"]["user"] = user_cfg_file;
    }
    if ((all || keys.count("config") > 0) && user_cfg)
    {
        for (const auto &section : *user_cfg)
        {
            String cfg_section = String::FromFormat("config@%s", section.first.GetCStr());
            for (const auto &opt : section.second)
                data[cfg_section][opt.first] = opt.second;
        }
    }
    if (all || keys.count("data") > 0)
    {
        data["data"]["gamename"] = game.gamename;
        data["data"]["version"] = StrUtil::IntToString(game.LoadedDataVersion);
        data["data"]["compiledwith"] = game.compiled_with;
        data["data"]["basepack"] = services.ResPaths.GamePak.Path;
    }
    if (all || keys.count("gameproperties") > 0)
    {
        fill_game_properties(data["gameproperties"]);
    }
    if (all || keys.count("filepath") > 0)
    {
        data["filepath"]["exe"] = services.AppPath;
        data["filepath"]["cwd"] = Directory::GetCurrentDirectory();
        data["filepath"]["datadir"] = Path::MakePathNoSlash(services.ResPaths.DataDir);
        for (size_t i = 0; i < services.ResPaths.OptDataDirs.size(); ++i)
        {
            data["filepath"][String::FromFormat("datadir%zu", i)] = services.ResPaths.OptDataDirs[i].first;
            data["filepath"][String::FromFormat("datadir%zu_filter", i)] = services.ResPaths.OptDataDirs[i].second;
        }
        data["filepath"]["savegamedir"] = Path::MakePathNoSlash(GetGameUserDataDir().FullDir);
        data["filepath"]["appdatadir"] = Path::MakePathNoSlash(GetGameAppDataDir().FullDir);
    }
    String full;
    IniUtil::WriteToString(full, data);
    services.Platform->WriteStdOut("%s", full.GetCStr());
}

// Connects to the Editor's debugger, if the configuration asks for it, and waits for
// half a second for its initial messages (breakpoints)
void engine_init_editor_debugging(const ConfigTree &cfg)
{
    set_our_eip(-201);
    Debug::Printf(kDbgMsg_Info, "Try connect to the external debugger");
    if (!init_editor_debugging(cfg))
        return;

    auto wait_until = Clock::now() + std::chrono::milliseconds(500);
    while (wait_until > Clock::now())
    {
        // pick up any breakpoints in game_start
        check_for_messages_from_debugger();
    }

    ccSetDebugHook(scriptDebugHook);
}

// TODO: this function is still a big mess, engine/system-related initialization
// is mixed with game-related data adjustments. Divide it in parts, move game
// data init into either InitGameState() or other game method as appropriate.
int initialize_engine(const ConfigTree &startup_opts)
{
    EngineServices &services = EngineServices::Current();
    // The sprite cache writes into the game's sprite table from now on; nothing has made or loaded a sprite yet
    init_sprite_cache();
    if (engine_pre_init_callback)
    {
        engine_pre_init_callback();
    }

    //-----------------------------------------------------
    // Install backend
    if (!engine_init_backend())
        return EXIT_ERROR;

    //-----------------------------------------------------
    // Connect to the external debugger, if required;
    // use only startup options here, the full config will be available
    // only after game files location is found
    if (services.Debug.EditorDebuggingEnabled && !(services.JustTellInfo || services.JustRunSetup))
    {
        engine_init_editor_debugging(startup_opts);
    }

    //-----------------------------------------------------
    // Locate game data and assemble game config
    if (services.JustTellInfo && !print_info_needs_game(services.TellInfoKeys))
    {
        engine_print_info(services.TellInfoKeys, nullptr);
        return EXIT_NORMAL;
    }

    if (!engine_init_gamedata())
        return EXIT_ERROR;

    // Test if need to run built-in setup program (where available)
    if (!services.JustTellInfo && services.JustRunSetup)
    {
        ConfigTree cfg, def_cfg;
        engine_prepare_config(cfg, &def_cfg, startup_opts);
        if (!engine_run_setup(cfg, def_cfg))
            return EXIT_NORMAL;
    }

    // Set up game options from user config
    ConfigTree cfg;
    engine_prepare_config(cfg, nullptr, startup_opts);
    engine_set_config(cfg);
    if (services.JustTellInfo)
    {
        engine_print_info(services.TellInfoKeys, &cfg);
        return EXIT_NORMAL;
    }

    set_our_eip(-190);

    //-----------------------------------------------------
    // Init auxiliary data files and other directories, initialize asset manager
    engine_init_user_directories();

    set_our_eip(-191);

    engine_locate_speech_pak();

    set_our_eip(-192);

    engine_locate_audio_pak();

    set_our_eip(-193);

    engine_assign_assetpaths();

    //-----------------------------------------------------
    // Begin setting up systems

    set_our_eip(-194);

    engine_init_fonts();

    set_our_eip(-195);

    init_joystick();

    engine_init_keyboard();

    set_our_eip(-196);

    engine_init_mouse();

    set_our_eip(-198);

    engine_init_audio();

    set_our_eip(-199);

    engine_init_debug();

    set_our_eip(-10);

    engine_init_exit_handler();

    set_our_eip(-20);
    set_our_eip(-19);

    int res = engine_load_game_data();
    if (res != 0)
        return res;

    set_our_eip(-189);

    res = engine_check_disk_space();
    if (res != 0)
        return res;

    set_our_eip(-179);

    engine_adjust_for_rotation_settings();

    // Attempt to initialize graphics mode
    if (!engine_try_set_gfxmode_any(services.Config.Display))
        return EXIT_ERROR;

    // Configure game window after renderer was initialized
    engine_setup_window();

    SetMultitasking(services.Config.RunInBackground);

    sys_window_show_cursor(false); // hide the system cursor

    show_preload();
    HError err = engine_init_sprites();
    if (!err)
    {
        services.Platform->DisplayAlert("Could not load sprite set file:\n%s", err->FullMessage().GetCStr());
        return EXIT_ERROR;
    }

    if ((services.Debug.Flags & DBG_DBGSCRIPT) != 0)
        ccSetDebugLogging(true);

    // TODO: move *init_game_settings to game init code unit
    engine_init_game_settings();
    engine_prepare_to_start_game();

    initialize_start_and_play_game(services.OverrideStartRoom, services.LoadSaveGameOnStartup);

    return EXIT_NORMAL;
}

bool engine_try_set_gfxmode_any(const DisplayModeSetup &setup)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    const DisplayMode old_dm = services.GfxDriver ? services.GfxDriver->GetDisplayMode() : DisplayMode();

    engine_shutdown_gfxmode();

    sys_renderer_set_output(services.Config.SoftwareRenderDriver);

    Size init_desktop;
    bool res = graphics_mode_init_any(GraphicResolution(game.GetGameRes(), game.color_depth * 8), setup,
                                      ColorDepthOption(game.GetColorDepth()), &init_desktop);

    if (res)
        engine_post_gfxmode_setup(init_desktop, old_dm);
    // Make sure that we don't receive window events queued during init
    sys_flush_events();
    return res;
}

bool engine_try_switch_windowed_gfxmode()
{
    EngineServices &services = EngineServices::Current();
    if (!services.GfxDriver || !services.GfxDriver->IsModeSet() || !services.Platform->FullscreenSupported())
        return false;

    // Keep previous mode in case we need to revert back
    DisplayMode old_dm = services.GfxDriver->GetDisplayMode();
    FrameScaleDef old_frame = graphics_mode_get_render_frame();

    // Release engine resources that depend on display mode
    engine_pre_gfxmode_release();

    Size init_desktop = get_desktop_size(old_dm.DisplayIndex);
    bool windowed = !old_dm.IsWindowed();
    ActiveDisplaySetting setting = graphics_mode_get_last_setting(windowed);
    DisplayMode last_opposite_mode = setting.Dm;
    FrameScaleDef frame = setting.Frame;

    // Apply vsync in case it has been toggled at runtime
    last_opposite_mode.Vsync = services.Config.Display.VSync;

    // If there are saved parameters for given mode (fullscreen/windowed),
    // *and* if the window is on the same display where it's been last time,
    // then use old params, otherwise - get default setup for the new mode.
    bool res;
    const int use_display_index = sys_get_window_display_index();
    if (last_opposite_mode.IsValid() && (setting.Dm.DisplayIndex == use_display_index))
    {
        res = graphics_mode_set_dm(last_opposite_mode);
    }
    else
    {
        WindowSetup window_setup = windowed ? services.Config.Display.WinSetup : services.Config.Display.FsSetup;
        frame = windowed ? services.Config.Display.WinGameFrame : services.Config.Display.FsGameFrame;
        res = graphics_mode_set_dm_any(
            Game::Current().GetGameRes(), window_setup, old_dm.ColorDepth, frame,
            DisplayParamsEx(use_display_index, services.Config.Display.RefreshRate, services.Config.Display.VSync));
    }

    // Apply corresponding frame render method
    if (res)
        res = graphics_mode_set_render_frame(frame);

    if (!res)
    {
        // If failed, try switching back to previous gfx mode
        res = graphics_mode_set_dm(old_dm) && graphics_mode_set_render_frame(old_frame);
        if (!res)
            quitprintf("Failed to restore graphics mode.");
    }

    // If succeeded (with any case), update engine objects that rely on
    // active display mode.
    if (!services.GfxDriver->GetDisplayMode().IsRealFullscreen())
        init_desktop = get_desktop_size();
    engine_post_gfxmode_setup(init_desktop, old_dm);
    // Make sure that we don't receive window events queued during init
    sys_flush_events();
    return res;
}

void engine_on_window_changed(const Size &size)
{
    graphics_mode_on_window_changed(size);
    on_coordinates_scaling_changed();
    invalidate_screen();
}

void engine_shutdown_gfxmode()
{
    if (!EngineServices::Current().GfxDriver)
        return;

    engine_pre_gfxsystem_shutdown();
    graphics_mode_shutdown();
}

const char *get_engine_name()
{
    return "Adventure Game Studio run-time engine";
}

const char *get_engine_version()
{
    return EngineServices::Current().Version.LongString.GetCStr();
}

String get_engine_version_and_build()
{
    const char *bitness = (AGS_PLATFORM_64BIT) ? "64-bit" : "32-bit";
    const char *endianness = (AGS_PLATFORM_ENDIAN_LITTLE) ? "LE" : "BE";
#ifdef BUILD_STR
    return String::FromFormat("%s (Build: %s), %s %s", EngineServices::Current().Version.LongString.GetCStr(),
                              EngineServices::Current().Version.BuildInfo.GetCStr(), bitness, endianness);
#else
    return String::FromFormat("%s, %s %s", EngineServices::Current().Version.LongString.GetCStr(), bitness, endianness);
#endif
}

void engine_set_pre_init_callback(t_engine_pre_init_callback callback)
{
    engine_pre_init_callback = callback;
}
