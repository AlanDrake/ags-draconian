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
#include "game/translation.h"

#include <cstdio>

#include "data/assetmanager.h"
#include "data/tra_file.h"
#include "debug/out.h"
#include "font/fonts.h"
#include "game/asset_helper.h"
#include "game/dialog/display.h"
#include "game/dialog/parser.h"
#include "game/game.h"
#include "game/game_context.h"
#include "game/game_def.h"
#include "game/game_state.h"
#include "game/runtime_defines.h"
#include "gui/guimain.h"
#include "platform/base/engine_config.h"
#include "platform/base/engine_services.h"
#include "plugin/plugin_engine.h"
#include "util/common.h"
#include "util/stream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

String trans_name;
String trans_filename;
Translation trans;

FontInfo construct_font_override(FontInfo *base_finfo, const FontInfo &finfo, int fields)
{
    if (base_finfo)
    {
        FontInfo out_info = *base_finfo;
        if (fields & kFontField_Size)
        {
            out_info.Size = finfo.Size;
            out_info.SizeMultiplier = finfo.SizeMultiplier;
        }
        if (fields & kFontField_OutlineStyle)
            out_info.Outline = finfo.Outline;
        if (fields & kFontField_AutoOutlineStyle)
            out_info.AutoOutlineStyle = finfo.AutoOutlineStyle;
        if (fields & kFontField_AutoOutlineThickness)
            out_info.AutoOutlineThickness = finfo.AutoOutlineThickness;
        if (fields & kFontField_VerticalOffset)
            out_info.YOffset = finfo.YOffset;
        if (fields & kFontField_LineSpacing)
            out_info.LineSpacing = finfo.LineSpacing;
        if (fields & kFontField_CharacterSpacing)
            out_info.CharacterSpacing = finfo.CharacterSpacing;
        if (fields & kFontField_TTFMetricsFixup)
            out_info.Flags = (out_info.Flags & ~FFLG_ASCENDERFIXUP) | (finfo.Flags & FFLG_ASCENDERFIXUP);
        if (fields & kFontField_HeightDefinition)
        {
            out_info.Flags = (out_info.Flags & ~FFLG_FONTHEIGHTMASK) | (finfo.Flags & FFLG_FONTHEIGHTMASK);
            out_info.CustomHeight = finfo.CustomHeight;
        }
        // FIXME: this is hacky, figure out a better method to update internal fields based on flags
        out_info.SetFlags(out_info.Flags);
        return out_info;
    }
    else
    {
        return finfo;
    }
}

void init_font_overrides(const Translation &trans)
{
    Game &game = Game::Current();
    for (const auto &font_override : trans.FontOverrides)
    {
        const int font_id = font_override.first;
        const auto &fover = font_override.second;
        const auto &finfo = fover.Finfo;
        const auto &fields = fover.Fields;
        // If FontID is available, this means we should copy another existing font,
        // otherwise use FontInfo properties to load a new font
        const int use_font_id = finfo.FontID;
        if (use_font_id >= 0)
        {
            if (static_cast<uint32_t>(use_font_id) < game.fonts.size())
            {
                Debug::Printf("Init font's %d override using replacement font %d and provided parameters", font_id,
                              use_font_id);
                FontInfo use_finfo = construct_font_override(&game.fonts[use_font_id], finfo, fields);
                load_game_font(font_id, use_finfo, loaded_game_file_version);
            }
            else
            {
                Debug::Printf(kDbgMsg_Error, "ERROR: can't init font's %d override: replacement font %d does not exist",
                              font_id, use_font_id);
            }
        }
        else if (!finfo.FileName.IsEmpty())
        {
            Debug::Printf("Init font's %d override using file %s and provided parameters", font_id,
                          finfo.FileName.GetCStr());
            FontInfo use_finfo = construct_font_override(nullptr, finfo, fields);
            load_game_font(font_id, use_finfo, loaded_game_file_version);
        }
        else
        {
            Debug::Printf(kDbgMsg_Error,
                          "ERROR: can't init font's %d override: neither replacement font ID nor filename provided",
                          font_id);
        }
    }

    // Additional fixups - after all the fonts are registered
    fixup_game_fonts();
}

void restore_game_fonts(const Translation &old_trans)
{
    Game &game = Game::Current();
    for (const auto &font_override : trans.FontOverrides)
    {
        const int font_id = font_override.first;
        if (font_id >= 0 && static_cast<uint32_t>(font_id) < game.fonts.size())
            load_game_font(font_id, game.fonts[font_id], loaded_game_file_version);
    }

    // Additional fixups - after all the fonts are registered
    fixup_game_fonts();
}

void close_translation()
{
    Game &game = Game::Current();
    game.options[OPT_AUTOTRANSPARSERSAID] = 0;
    if (trans.FontOverrides.size() > 0)
    {
        restore_game_fonts(trans);
    }

    trans = Translation();
    trans_name = "";
    trans_filename = "";

    SetGameTextLanguage(game.GameTextLanguage);
    SetTranslationTextParser(CreateTextParser(game.dict.get(), true, EngineServices::Current().TextLocaleName));
}

void SetGameTextLanguage(const String &language)
{
    EngineServices &services = EngineServices::Current();
    services.TextLanguage = language;
    services.TextLocaleName = StrUtil::FindCompatibleUTF8LocaleName(language.GetCStr());
    GUI::Context.TextLocaleName = services.TextLocaleName;
    Debug::Printf("Set game text language: %s", services.TextLanguage.GetCStr());
    Debug::Printf("Set locale: %s", services.TextLocaleName.GetCStr());
}

bool init_translation(const String &lang, const String &fallback_lang)
{
    EngineServices &services = EngineServices::Current();
    Game &game = Game::Current();
    if (lang.IsEmpty())
        return false;
    trans_name = lang;
    trans_filename = String::FromFormat("%s.tra", lang.GetCStr());

    auto in = services.Assets->OpenAsset(trans_filename);
    if (in == nullptr)
    {
        Debug::Printf(kDbgMsg_Error, "Cannot open translation: %s", trans_filename.GetCStr());
        return false;
    }

    trans = Translation();

    // First test if the translation is meant for this game
    HError err = TestTraGameID(game.uniqueid, game.gamename, std::move(in));
    if (err)
    {
        // If successful, then read translation data fully
        in = services.Assets->OpenAsset(trans_filename);
        err = ReadTraData(trans, std::move(in));
    }

    // Process errors
    if (!err)
    {
        Debug::Printf(kDbgMsg_Error, "Failed to read translation file %s:\n\t%s", trans_filename.GetCStr(),
                      err->FullMessage().GetCStr());
        close_translation();
        if (!fallback_lang.IsEmpty())
        {
            Debug::Printf("Fallback to translation: %s", fallback_lang.GetCStr());
            init_translation(fallback_lang, "");
        }
        return false;
    }

    // Translation read successfully
    Debug::Printf("Translation loaded: %s", trans_filename.GetCStr());
    // Configure new game settings
    if (trans.NormalFont >= 0)
        Game_SetNormalFont(trans.NormalFont);
    if (trans.SpeechFont >= 0)
        Game_SetSpeechFont(trans.SpeechFont);
    if (trans.RightToLeft != kTextDirection_Default)
    {
        game.options[OPT_RIGHTLEFTWRITE] = trans.RightToLeft == kTextDirection_LTR ? 0 : 1;
        HorAlignment align = game.options[OPT_RIGHTLEFTWRITE] ? kHAlignRight : kHAlignLeft;
        game.Play.text_align = (game.Play.text_align != kHAlignCenter) ? align : kHAlignCenter;
        game.Play.speech_text_align = (game.Play.speech_text_align != kHAlignCenter) ? align : kHAlignCenter;
        if (game.Play.GetRBSwitches()[kRBO_ApplyDialogOptionTextAlignment])
            game.Play.dialog_options_textalign =
                (game.Play.dialog_options_textalign != kHAlignCenter) ? align : kHAlignCenter;
    }
    game.options[OPT_AUTOTRANSPARSERSAID] = (trans.OptFlags & kTraOpt_AutoTranslateSaid) != 0;

    // Font overrides
    if (trans.FontOverrides.size() > 0)
    {
        init_font_overrides(trans);
    }

    // Setup a text encoding mode depending on the translation data hint
    String encoding = trans.StrOptions["encoding"];
    String language = trans.StrOptions["language"];
    String encoding_msg = !encoding.IsEmpty() ? encoding : "presume ASCII";
    Debug::Printf("Translation's encoding: %s, language: %s", encoding_msg.GetCStr(), language.GetCStr());
    if (encoding.CompareNoCase("utf-8") != 0)
        Debug::Printf(kDbgMsg_Warn,
                      "WARNING: translation's text encoding is not UTF-8, and may be displayed incorrectly");

    SetGameTextLanguage(language);
    if (trans.ParserDict.GetWords().size() > 0)
    {
        // The parser dictionary must contain all expected word groups, so if any are not
        // found in the translated dict, then add base ones directly there.
        if (game.dict.get())
            MergeParserDictionary(&trans.ParserDict, game.dict.get());
        SetTranslationTextParser(CreateTextParser(&trans.ParserDict, true, services.TextLocaleName));
    }
    else
    {
        SetTranslationTextParser(CreateTextParser(game.dict.get(), true, services.TextLocaleName));
    }

    Debug::Printf(kDbgMsg_Info, "Translation initialized: %s (format: %s)", trans_name.GetCStr(),
                  encoding_msg.GetCStr());
    return true;
}

String get_translation_name()
{
    return trans_name;
}

const StringMap &get_translation_tree()
{
    return trans.Dict;
}

const char *get_translation(const char *text)
{
    if (!text)
        text = "";
    // FIXME: dont use global variable, remake into the function parameter!
    source_text_length = GetTextDisplayLength(text);
    if (text[0] == 0)
        return ""; // don't try translating an empty line

    // check if a plugin wants to translate it - if so, return that
    const char *pl_result =
        reinterpret_cast<const char *>(pl_run_plugin_hooks(kPluginEvt_TranslateText, reinterpret_cast<intptr_t>(text)));
    if (pl_result)
        return pl_result;

    const auto &transtree = get_translation_tree();
    const auto it = transtree.find(String::Wrapper(text));
    if (it != transtree.end())
        return it->second.GetCStr();
    // return the original text
    return text;
}

int IsTranslationAvailable()
{
    if (get_translation_tree().size() > 0)
        return 1;
    return 0;
}
