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
#include "data/tra_utils.h"

#include <cctype>
#include <map>
#include <set>
#include <unordered_map>

#include "ac/game_version.h"
#include "ac/wordsdictionary.h"
#include "data/data_ext.h"
#include "data/data_helpers.h"
#include "util/string_utils.h"
#include "util/textstreamreader.h"
#include "util/textstreamwriter.h"

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

//-----------------------------------------------------------------------------
// The special tags of translation sources, read from PO and written to the PO that po2tra -u makes
//-----------------------------------------------------------------------------

const char OPTION_SEPARATOR = '=';
const String GAMEID_TAG = "GameID";
const String GAMENAME_TAG = "GameName";
const String NORMAL_FONT_TAG = "NormalFont";
const String SPEECH_FONT_TAG = "SpeechFont";
const String TEXT_DIRECTION_TAG = "TextDirection";
const String AUTO_PARSERSAID_TAG = "AutoTranslateParserSaid";
const String ENCODING_TAG = "Encoding";
const String GAMEENCODING_TAG = "GameEncoding";
const String LANGUAGE_TAG = "Language";
const String FONT_OVERRIDE_TAG = "Font";
const String TAG_DEFAULT = "DEFAULT";
const String TAG_DIRECTION_LEFT = "LEFT";
const String TAG_DIRECTION_RIGHT = "RIGHT";
const String TAG_ON = "ON";
const String TAG_OFF = "OFF";
const char ANNOTATE_SEPARATOR = ':';
const String ANNOTATE_PARSERWORD = "PARSERWORD";


static int ReadOptionalInt(const String &text)
{
    if (text == TAG_DEFAULT)
        return -1;
    return StrUtil::StringToInt(text, -1);
}

static String WriteOptionalInt(int value)
{
    if (value < 0)
        return TAG_DEFAULT;
    return String::FromFormat("%d", value);
}

static int ParseFontN(const String &line)
{
    if (line.StartsWith(FONT_OVERRIDE_TAG))
    {
        return StrUtil::StringToInt(line.Mid(FONT_OVERRIDE_TAG.GetLength()), -1);
    }
    return -1;
}

static bool ParseFontOverride(const String &line, FontOverride &fover)
{
    // Format:
    //    [FontN;]Property1=Value1;Property2=Value2;Property3=Value3;...
    const auto sections = line.Split(';');
    // This is a new font generation
    std::vector<std::pair<String, String>> options;
    for (const auto &sec : sections)
    {
        options.push_back(StrUtil::GetKeyValue(sec, OPTION_SEPARATOR));
    }
    FontInfo finfo;
    int fields = kFontField_None;
    for (const auto &opt : options)
    {
        const String &key = opt.first;
        const String &value = opt.second;

        if (key == "Font")
        {
            finfo.FontID = StrUtil::StringToInt(value);
        }
        else if (key.StartsWith("Font"))
        {
            int re_font_number = ParseFontN(key);
            if (re_font_number >= 0)
                finfo.FontID = re_font_number;
        }
        else if (key == "File")
        {
            finfo.FileName = value;
        }
        else if (key == "Size")
        {
            finfo.Size = StrUtil::StringToInt(value);
            fields |= kFontField_Size;
        }
        else if (key == "SizeMultiplier")
        {
            finfo.SizeMultiplier = StrUtil::StringToInt(value);
            fields |= kFontField_Size;
        }
        else if (key == "Outline")
        {
            if (value == "NONE")
            {
                finfo.Outline = FONT_OUTLINE_NONE;
            }
            else if (value == "AUTO")
            {
                finfo.Outline = FONT_OUTLINE_AUTO;
            }
            else
            {
                int out_font_id = ParseFontN(value);
                if (out_font_id >= 0)
                {
                    finfo.Outline = out_font_id;
                }
            }
            fields |= kFontField_OutlineStyle;
        }
        else if (key == "AutoOutline")
        {
            if (value == "SQUARED")
            {
                finfo.AutoOutlineStyle = FontInfo::kSquared;
            }
            else if (value == "ROUND")
            {
                finfo.AutoOutlineStyle = FontInfo::kRounded;
            }
            fields |= kFontField_AutoOutlineStyle;
        }
        else if (key == "AutoOutlineThickness")
        {
            finfo.AutoOutlineThickness = StrUtil::StringToInt(value);
            fields |= kFontField_AutoOutlineThickness;
        }
        else if (key == "HeightDefinition")
        {
            if (value == "NOMINAL")
            {
                finfo.SetHeightFlags(FFLG_LOGICALNOMINALHEIGHT);
            }
            else if (value == "REAL")
            {
                finfo.SetHeightFlags(0 /* use real height */);
            }
            else if (value == "CUSTOM")
            {
                finfo.SetHeightFlags(FFLG_LOGICALCUSTOMHEIGHT);
            }
            fields |= kFontField_HeightDefinition;
        }
        else if (key == "CustomHeight")
        {
            finfo.CustomHeight = StrUtil::StringToInt(value);
            fields |= kFontField_HeightDefinition;
        }
        else if (key == "VerticalOffset")
        {
            finfo.YOffset = StrUtil::StringToInt(value);
            fields |= kFontField_VerticalOffset;
        }
        else if (key == "LineSpacing")
        {
            finfo.LineSpacing = StrUtil::StringToInt(value);
            fields |= kFontField_LineSpacing;
        }
        else if (key == "CharacterSpacing")
        {
            finfo.CharacterSpacing = StrUtil::StringToInt(value);
            fields |= kFontField_CharacterSpacing;
        }
    }

    // Adjust font flags based on the read parameters
    if (finfo.Size == 0)
        finfo.Flags |= FFLG_SIZEMULTIPLIER;

    fover = FontOverride(finfo, static_cast<FontFieldFlags>(fields));
    return true;
}

static void ReadSpecialTags(TranslationSource &src, const String &line)
{
    TranslationOptions &tra = src.Options;
    const auto key_value = StrUtil::GetKeyValue(line, OPTION_SEPARATOR);
    const String key = key_value.first;
    const String value = key_value.second;
    if (key == GAMEID_TAG)
    {
        tra.GameUid = StrUtil::StringToInt(value);
    }
    else if (key == GAMENAME_TAG)
    {
        tra.GameName = value;
    }
    else if (key == NORMAL_FONT_TAG)
    {
        tra.NormalFont = ReadOptionalInt(value);
    }
    else if (key == SPEECH_FONT_TAG)
    {
        tra.SpeechFont = ReadOptionalInt(value);
    }
    else if (key == TEXT_DIRECTION_TAG)
    {
        String directionText = value;
        if (directionText == TAG_DIRECTION_LEFT)
        {
            tra.RightToLeft = kTextDirection_LTR;
        }
        else if (directionText == TAG_DIRECTION_RIGHT)
        {
            tra.RightToLeft = kTextDirection_RTL;
        }
        else
        {
            tra.RightToLeft = kTextDirection_Default;
        }
    }
    else if (key == AUTO_PARSERSAID_TAG)
    {
        if (value == TAG_ON)
            tra.OptFlags |= kTraOpt_AutoTranslateSaid;
        else
            tra.OptFlags &= ~kTraOpt_AutoTranslateSaid;
    }
    else if (key == ENCODING_TAG)
    {
        tra.EncodingHint = value;
        tra.StrOptions["encoding"] = value;
    }
    else if (key == GAMEENCODING_TAG)
    {
        tra.GameEncodingHint = value;
        tra.StrOptions["gameencoding"] = value;
    }
    else if (key == LANGUAGE_TAG)
    {
        tra.LanguageHint = value;
        tra.StrOptions["language"] = value;
    }
    else if (key.StartsWith(FONT_OVERRIDE_TAG))
    {
        int font_id = ParseFontN(key);
        if ((font_id >= 0) && (tra.FontOverrides.count(font_id) == 0))
        {
            FontOverride fover;
            if (ParseFontOverride(value, fover))
            {
                tra.FontOverrides[font_id] = fover;
                src.FontOverrideOrder.push_back(font_id);
            }
        }
    }
}

static void WriteFontOverrides(const TranslationOptions &tra, TextStreamWriter &sw)
{
    // in the order of the fonts, the translation keeps them in a hash map
    const std::map<int, FontOverride> sorted_overrides(tra.FontOverrides.begin(), tra.FontOverrides.end());
    for (const auto &font_over : sorted_overrides)
    {
        String font_line;
        int fontIndex = font_over.first;
        const FontOverride &fover = font_over.second;
        const FontInfo &finfo = fover.Finfo;
        const int fields = fover.Fields;
        font_line.AppendFmt("# $Font%d=", fontIndex);
        // The font override's source is defined either as another font's index, or font file
        if (finfo.FontID >= 0)
            font_line.AppendFmt("Font%d;", finfo.FontID);
        else if (!finfo.FileName.IsEmpty())
            font_line.AppendFmt("File=%s;", finfo.FileName.GetCStr());

        //
        // Use fields value to know which properties have been specified
        //

        if ((fields & kFontField_Size) && finfo.Size > 0)
            font_line.AppendFmt("Size=%d;", finfo.Size);
        if ((fields & kFontField_Size) && finfo.SizeMultiplier > 1)
            font_line.AppendFmt("SizeMultiplier=%d;", finfo.SizeMultiplier);

        if (fields & kFontField_OutlineStyle)
        {
            if (finfo.Outline == FONT_OUTLINE_NONE)
                font_line.Append("Outline=NONE;");
            else if (finfo.Outline == FONT_OUTLINE_AUTO)
                font_line.Append("Outline=AUTO;");
            else if (finfo.Outline >= 0)
                font_line.AppendFmt("Outline=Font%d;", finfo.Outline);
        }

        if (fields & kFontField_AutoOutlineStyle)
        {
            if (finfo.AutoOutlineStyle == FontInfo::kSquared)
                font_line.Append("AutoOutline=SQUARED;");
            else if (finfo.AutoOutlineStyle == FontInfo::kRounded)
                font_line.Append("AutoOutline=ROUND;");
        }

        if (fields & kFontField_AutoOutlineThickness)
            font_line.AppendFmt("AutoOutlineThickness=%d;", finfo.AutoOutlineThickness);

        if (fields & kFontField_HeightDefinition)
        {
            if ((finfo.Flags & FFLG_LOGICALCUSTOMHEIGHT) != 0)
                font_line.Append("HeightDefinition=CUSTOM;");
            else if ((finfo.Flags & FFLG_LOGICALNOMINALHEIGHT) != 0)
                font_line.Append("HeightDefinition=NOMINAL;");
            else if ((finfo.Flags & FFLG_LOGICALNOMINALHEIGHT) == 0)
                font_line.Append("HeightDefinition=REAL;");

            if ((finfo.Flags & FFLG_LOGICALCUSTOMHEIGHT) != 0)
            {
                font_line.AppendFmt("CustomHeight=%d;", finfo.CustomHeight);
            }
        }

        if ((fields & kFontField_VerticalOffset) && finfo.YOffset != 0)
            font_line.AppendFmt("VerticalOffset=%d;", finfo.YOffset);
        if ((fields & kFontField_LineSpacing) && finfo.LineSpacing != 0)
            font_line.AppendFmt("LineSpacing=%d;", finfo.LineSpacing);
        if ((fields & kFontField_CharacterSpacing) && finfo.CharacterSpacing != 0)
            font_line.AppendFmt("CharacterSpacing=%d;", finfo.CharacterSpacing);

        sw.WriteLine(font_line);
    }
}

//-----------------------------------------------------------------------------
// PO - the translation source of AGS 4 projects (gettext format); the editor converts the legacy
// TRS sources to PO when it upgrades a project
//-----------------------------------------------------------------------------

// Adds a translated text; a text found again keeps its place and takes the later entry, as the
// editor's dictionary of entries does
static void AddEntry(TranslationSource &src, std::unordered_map<String, size_t> &index, TranslationSourceEntry &&entry)
{
    const auto it = index.find(entry.Key);
    if (it != index.end())
    {
        src.Entries[it->second] = std::move(entry);
        return;
    }
    index[entry.Key] = src.Entries.size();
    src.Entries.push_back(std::move(entry));
}

const String PO_CONTEXT_OBSOLETE = "OBSOLETE";
const String PO_FLAG_FUZZY = "fuzzy";

// Parses a PO line: a keyword followed by a string, or a string alone, which continues the previous
// one (keyword is then empty); plural_index is set for msgstr[N], otherwise -1 (Translation.ParsePOLine)
static bool ParsePOLine(const String &line, String &keyword, String &text, int &plural_index)
{
    static const char *const kKeywords[] = {"msgctxt", "msgid_plural", "msgid", "msgstr"};
    keyword = "";
    plural_index = -1;
    size_t at = 0;
    for (const char *kw : kKeywords)
    {
        if (line.StartsWith(kw))
        {
            keyword = kw;
            at = keyword.GetLength();
            break;
        }
    }
    if (!keyword.IsEmpty() && at < line.GetLength() && line[at] == '[')
    {
        // only msgstr may have an index
        const size_t end = line.FindChar(']', at);
        if (end == String::NoIndex || end == at + 1 || keyword != "msgstr")
            return false;
        for (size_t i = at + 1; i < end; ++i)
        {
            if (line[i] < '0' || line[i] > '9')
                return false;
        }
        plural_index = StrUtil::StringToInt(line.Mid(at + 1, end - at - 1));
        at = end + 1;
    }
    if (!keyword.IsEmpty())
    {
        while (at < line.GetLength() && isspace(static_cast<unsigned char>(line[at])))
            ++at;
    }
    if (line.GetLength() < at + 2 || line[at] != '"' || line.GetLast() != '"')
        return false;
    text = line.Mid(at + 1, line.GetLength() - at - 2);
    return true;
}

// A PO entry as it is parsed
struct POEntry
{
    String Context;
    TranslationSourceEntry Entry;
};

HError ReadPO(TranslationSource &src, std::unique_ptr<Stream> &&in)
{
    enum ParseState
    {
        kNewEntry,
        kParsingContext,
        kParsingId,
        kParsingString,
        kSkipping // ignoring unsupported or malformed data until the next keyword
    };

    TextStreamReader sr(std::move(in));
    std::unordered_map<String, size_t> index;
    ParseState state = kNewEntry;
    POEntry po;
    bool has_context = false, has_msgid = false, has_msgstr = false;

    // Stores the entry being parsed, if it's complete, and begins a new one
    auto finish_entry = [&]()
    {
        // an entry with an empty msgid is the PO header
        if (has_msgid && has_msgstr && !po.Entry.Key.IsEmpty())
        {
            TranslationSourceEntry &entry = po.Entry;
            // a hand-edited source may have quotes without their backslash, as in the editor
            entry.Key = StrUtil::EscapeUnescapedQuotes(entry.Key);
            entry.Value = StrUtil::EscapeUnescapedQuotes(entry.Value);
            // The context marks obsolete entries and the Text Parser's words (TranslationEntry.Context)
            if (po.Context == PO_CONTEXT_OBSOLETE)
            {
                entry.Obsolete = true;
            }
            else if (po.Context.StartsWith(ANNOTATE_PARSERWORD))
            {
                const auto key_value = StrUtil::GetKeyValue(po.Context, ANNOTATE_SEPARATOR);
                if (key_value.first == ANNOTATE_PARSERWORD)
                    entry.ParserWordID = StrUtil::StringToInt(key_value.second, -1);
            }
            AddEntry(src, index, std::move(entry));
        }
        state = kNewEntry;
        po = POEntry();
        has_context = has_msgid = has_msgstr = false;
    };

    while (!sr.EOS())
    {
        String line = sr.ReadLine();
        line.Trim();

        // Obsolete entries are commented out with "#~", but otherwise have the regular syntax;
        // "#~|" is a previous msgid of an obsolete entry, which is kept as a metadata
        bool is_obsolete_line = false;
        if (line.StartsWith("#~") && !line.StartsWith("#~|"))
        {
            line = line.Mid(2);
            line.TrimLeft();
            if (line.IsEmpty())
                continue;
            is_obsolete_line = true;
        }

        // An empty line separates entries
        if (line.IsEmpty())
        {
            if (state != kNewEntry)
                finish_entry();
            continue;
        }

        // Font overrides were mistakenly written in the legacy TRS format by some Editor versions
        if (line.StartsWith(String::FromFormat("//#%s", FONT_OVERRIDE_TAG.GetCStr())))
        {
            ReadSpecialTags(src, line.Mid(3));
            continue;
        }

        // Comments and annotations precede the entry, so they begin a new one
        if (line.StartsWith("#"))
        {
            if (has_msgstr)
                finish_entry();
            if (line.StartsWith("# $"))
            {
                ReadSpecialTags(src, line.Mid(3));
            }
            else if (line.StartsWith("#,"))
            {
                for (String flag : line.Mid(2).Split(','))
                {
                    flag.Trim();
                    if (flag == PO_FLAG_FUZZY)
                        po.Entry.Fuzzy = true;
                }
            }
            continue;
        }

        String keyword, text;
        int plural_index;
        if (!ParsePOLine(line, keyword, text, plural_index))
            continue; // the editor warns about it

        if (keyword == "msgctxt" || keyword == "msgid")
        {
            // Begins a new entry, unless msgid follows msgctxt
            if (has_msgstr || has_msgid || (keyword == "msgctxt" && has_context))
                finish_entry();
            if (keyword == "msgctxt")
            {
                state = kParsingContext;
                has_context = true;
                po.Context = text;
            }
            else
            {
                state = kParsingId;
                has_msgid = true;
                po.Entry.Key = text;
            }
        }
        else if (keyword == "msgstr")
        {
            if (!has_msgid || plural_index > 0 || has_msgstr)
            {
                state = kSkipping; // only the first plural form is used
            }
            else
            {
                state = kParsingString;
                has_msgstr = true;
                po.Entry.Value = text;
            }
        }
        else if (keyword == "msgid_plural")
        {
            state = kSkipping;
        }
        else
        {
            // Strings may be split on multiple lines, which are concatenated
            switch (state)
            {
            case kParsingContext: po.Context.Append(text); break;
            case kParsingId: po.Entry.Key.Append(text); break;
            case kParsingString: po.Entry.Value.Append(text); break;
            default: break;
            }
            continue; // continuation lines don't change the entry's obsolete state
        }

        if (is_obsolete_line)
            po.Entry.Obsolete = true;
    }
    // The last entry is not necessarily followed by an empty line
    finish_entry();
    return HError::None();
}

//-----------------------------------------------------------------------------
// PO, decompiled from a TRA
//-----------------------------------------------------------------------------

// The texts are escaped for a PO string (Translation.EscapeText). Unlike the editor, a "\r" stays
// "\r": the texts come from the game, and must still match it once compiled again
static void WritePOEntry(TextStreamWriter &sw, const String &context, const String &key, const String &value)
{
    sw.WriteLine("");
    if (!context.IsEmpty())
        sw.WriteLineFormat("msgctxt \"%s\"", context.GetCStr());
    sw.WriteLineFormat("msgid \"%s\"", StrUtil::Escape(key).GetCStr());
    sw.WriteLineFormat("msgstr \"%s\"", StrUtil::Escape(value).GetCStr());
}

HError WritePO(const Translation &tra, std::unique_ptr<Stream> &&out)
{
    TextStreamWriter sw(std::move(out));

    // The header of the editor's PO (Translation.SaveDataImpl)
    const String encoding = tra.EncodingHint.IsEmpty() ? "ASCII" : tra.EncodingHint;
    String lang_hint = tra.LanguageHint;
    lang_hint.Replace('-', '_');
    sw.WriteLine("# AGS TRANSLATION SOURCE FILE");
    sw.WriteLine("# This is a PO file generated according to the gettext specifications.");
    sw.WriteLine("# Special characters such as %%s symbolise things within the game,");
    sw.WriteLine("# so should be left in an appropriate place in the message.");
    sw.WriteLine("# ");
    sw.WriteLine("# Game identification: this lets game to detect a translation meant");
    sw.WriteLine("# for another game. These may be left blank too.");
    sw.WriteLineFormat("# $GameID=%s", (tra.GameUid != 0 ? String::FromFormat("%d", tra.GameUid).GetCStr() : ""));
    sw.WriteLineFormat("# $GameName=%s", tra.GameName.GetCStr());
    sw.WriteLine("# ** Translation settings are below");
    sw.WriteLine("# ** Leave them as \"DEFAULT\" to use the game settings");
    sw.WriteLine("# The normal font to use - DEFAULT or font number");
    sw.WriteLineFormat("# $NormalFont=%s", WriteOptionalInt(tra.NormalFont).GetCStr());
    sw.WriteLine("# The speech font to use - DEFAULT or font number");
    sw.WriteLineFormat("# $SpeechFont=%s", WriteOptionalInt(tra.SpeechFont).GetCStr());
    sw.WriteLine("# Text direction - DEFAULT, LEFT or RIGHT");
    sw.WriteLineFormat(
        "# $TextDirection=%s",
        ((tra.RightToLeft == kTextDirection_RTL)
             ? TAG_DIRECTION_RIGHT.GetCStr()
             : ((tra.RightToLeft == kTextDirection_Default) ? TAG_DEFAULT.GetCStr() : TAG_DIRECTION_LEFT.GetCStr())));
    sw.WriteLine("# Text encoding hint - ASCII or UTF-8");
    sw.WriteLineFormat("# $Encoding=%s", encoding.GetCStr());
    sw.WriteLine("# Text language, use standard locale strings, like 'en', 'en_US', etc");
    sw.WriteLineFormat("# $Language=%s", lang_hint.GetCStr());
    sw.WriteLine("# Whether engine should translate Parser.Said strings automatically - ON or OFF");
    sw.WriteLineFormat("# $AutoTranslateParserSaid=%s",
                       ((tra.OptFlags & kTraOpt_AutoTranslateSaid) != 0) ? TAG_ON.GetCStr() : TAG_OFF.GetCStr());
    WriteFontOverrides(tra, sw);
    sw.WriteLine("#  ");
    sw.WriteLine("# ** IT IS SUGGESTED TO USE A THIRD-PARTY TOOL TO EDIT THIS FILE");
    // The source language is not in the TRA
    sw.WriteLine("msgid \"\"");
    sw.WriteLine("msgstr \"\"");
    sw.WriteLine("\"Last-Translator: \\n\"");
    sw.WriteLine("\"Language-Team: \\n\"");
    sw.WriteLineFormat("\"Language: %s\\n\"", StrUtil::Escape(lang_hint).GetCStr());
    sw.WriteLine("\"MIME-Version: 1.0\\n\"");
    sw.WriteLineFormat("\"Content-Type: text/plain; charset=%s\\n\"",
                       (encoding == "ASCII" ? "ISO-8859-1" : encoding.GetCStr()));
    sw.WriteLine("\"Content-Transfer-Encoding: 8bit\\n\"");
    sw.WriteLine("\"X-Generator: po2tra\\n\"");

    // The texts in their order, the translation keeps them in a hash map
    const std::map<String, String> sorted_dict(tra.Dict.begin(), tra.Dict.end());
    for (const auto &entry : sorted_dict)
        WritePOEntry(sw, "", entry.first, entry.second);

    // The Text Parser's words, one entry per word ID with its translations separated by commas, as the
    // editor reads them. The TRA has no game words: each entry gets a placeholder unique to its ID,
    // because a PO reader keeps one entry per text
    std::map<int, std::set<String>> words_by_id;
    for (const auto &word : tra.ParserDict.GetWords())
        words_by_id[word.second].insert(word.first);
    for (const auto &words : words_by_id)
    {
        String value;
        for (const auto &word : words.second)
        {
            if (!value.IsEmpty())
                value.Append(", ");
            value.Append(word);
        }
        WritePOEntry(sw, String::FromFormat("%s%c%d", ANNOTATE_PARSERWORD.GetCStr(), ANNOTATE_SEPARATOR, words.first),
                     String::FromFormat("??? (word %d)", words.first), value);
    }
    if (!sorted_dict.empty() || !words_by_id.empty())
        sw.WriteLine("");
    return HError::None();
}

//-----------------------------------------------------------------------------
// TRA - the compiled translation, written as the editor writes it
//-----------------------------------------------------------------------------

// A text of the translation's dictionary or the game name: its length, and the encrypted text
// without a terminator
static void WriteEncryptedText(const String &text, Stream *out)
{
    std::vector<char> buf(text.GetCStr(), text.GetCStr() + text.GetLength());
    EncryptText(buf.data(), buf.size());
    out->WriteInt32(static_cast<int32_t>(buf.size()));
    if (!buf.empty())
        out->Write(buf.data(), buf.size());
}

HError WriteTRA(const TranslationSource &src, std::unique_ptr<Stream> &&out)
{
    const TranslationOptions &tra = src.Options;

    // Translated texts and Text Parser words, skipping the obsolete entries and those which
    // translators marked for review (as the Editor's own compiler did, up to the tag pre-6.7d).
    // The escapes are resolved as the script compiler resolves them (unknown ones stay), because
    // the translation's keys must match the game's texts (Translation.UnescapeText)
    std::vector<std::pair<String, String>> lines;
    std::vector<std::pair<String, uint16_t>> parser_words;
    for (const auto &entry : src.Entries)
    {
        if (entry.Value.IsEmpty() || entry.Obsolete || entry.Fuzzy)
            continue;
        if (entry.ParserWordID >= 0)
        {
            parser_words.emplace_back(StrUtil::Unescape(entry.Value, true), entry.ParserWordID);
        }
        else
        {
            lines.emplace_back(StrUtil::Unescape(entry.Key, true), StrUtil::Unescape(entry.Value, true));
        }
    }
    if (lines.empty())
        printf("WARNING: input translation did not appear to have any translated lines.\n");

    Stream *s = out.get();
    s->Write("AGSTranslation", sizeof("AGSTranslation")); // with the terminator
    const int flags = kDataExt_NumID32 | kDataExt_File32;
    WriteExtBlock(
        kTraFblk_GameID, "",
        [&](Stream *out)
        {
            out->WriteInt32(tra.GameUid);
            WriteEncryptedText(tra.GameName, out);
        },
        flags, s);
    WriteExtBlock(
        kTraFblk_Dict, "",
        [&](Stream *out)
        {
            for (const auto &line : lines)
            {
                WriteEncryptedText(line.first, out);
                WriteEncryptedText(line.second, out);
            }
            WriteEncryptedText("", out);
            WriteEncryptedText("", out);
        },
        flags, s);
    if (!parser_words.empty())
    {
        // In the format of the game data's words dictionary; an entry may list several words
        WriteExtBlock(
            "ext_parserdict",
            [&](Stream *out)
            {
                std::vector<std::pair<String, uint16_t>> words;
                for (const auto &entry : parser_words)
                {
                    for (String word : entry.first.Split(','))
                    {
                        word.Trim();
                        if (!word.IsEmpty())
                            words.push_back(std::make_pair(word, entry.second));
                    }
                }
                out->WriteInt32(static_cast<int32_t>(words.size()));
                for (const auto &word : words)
                {
                    WriteStringEncrypt(out, word.first.GetCStr());
                    out->WriteInt16(word.second);
                }
            },
            flags, s);
    }
    WriteExtBlock(
        kTraFblk_TextOpts, "",
        [&](Stream *out)
        {
            out->WriteInt32(tra.NormalFont);
            out->WriteInt32(tra.SpeechFont);
            out->WriteInt32(tra.RightToLeft);
            out->WriteInt32(tra.OptFlags);
        },
        flags, s);
    WriteExtBlock(
        "ext_sopts",
        [&](Stream *out)
        {
            out->WriteInt32(2);
            StrUtil::WriteString("encoding", out);
            StrUtil::WriteString(tra.EncodingHint, out);
            StrUtil::WriteString("language", out);
            StrUtil::WriteString(tra.LanguageHint, out);
        },
        flags, s);
    if (!src.FontOverrideOrder.empty())
    {
        WriteExtBlock(
            "ext_fonts",
            [&](Stream *out)
            {
                out->WriteInt32(static_cast<int32_t>(src.FontOverrideOrder.size()));
                // data version (matches current game data); determines which font info extensions are written
                out->WriteInt32(kGameVersion_Current);
                for (const int font_id : src.FontOverrideOrder)
                {
                    const FontOverride &fover = tra.FontOverrides.at(font_id);
                    const FontInfo &finfo = fover.Finfo;
                    out->WriteInt32(font_id);      // font to override
                    out->WriteInt32(finfo.FontID); // replacement font ID (or -1 if determined by its filename)
                    out->WriteInt32(static_cast<uint32_t>(fover.Fields)); // which font fields are valid
                    out->WriteInt32(0);                                   // reserved
                    // The font info format of the game data, as in Common/data/tra_file.cpp
                    out->WriteInt32(finfo.Flags);
                    if ((finfo.Flags & FFLG_SIZEMULTIPLIER) == 0)
                        out->WriteInt32(finfo.Size * finfo.SizeMultiplier);
                    else
                        out->WriteInt32(finfo.SizeMultiplier);
                    out->WriteInt32(finfo.Outline);
                    out->WriteInt32(finfo.YOffset);
                    out->WriteInt32(finfo.LineSpacing);
                    out->WriteInt32(finfo.AutoOutlineThickness);
                    out->WriteInt32(finfo.AutoOutlineStyle);
                    out->WriteInt32(finfo.CharacterSpacing);
                    out->WriteInt32(finfo.CustomHeight);
                    out->WriteInt32(0); // reserved
                    out->WriteInt32(0);
                    StrUtil::WriteString(finfo.FileName, out);
                }
            },
            flags, s);
    }
    s->WriteInt32(kTraFile_EOF);
    s->WriteInt32(0);
    return HError::None();
}

} // namespace DataUtil
} // namespace AGS
