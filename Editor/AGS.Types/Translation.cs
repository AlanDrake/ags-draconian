using System;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;

namespace AGS.Types
{
    public class Translation
    {
        public const string TRANSLATION_SOURCE_FILE_EXTENSION = ".po";
        public const string TRANSLATION_COMPILED_FILE_EXTENSION = ".tra";

        public const char   OPTION_SEPARATOR = '=';
        public const string GAMEID_TAG = "GameID";
        public const string GAMENAME_TAG = "GameName";
        private const string NORMAL_FONT_TAG = "NormalFont";
        private const string SPEECH_FONT_TAG = "SpeechFont";
        private const string TEXT_DIRECTION_TAG = "TextDirection";
        private const string AUTO_PARSERSAID_TAG = "AutoTranslateParserSaid";
        private const string ENCODING_TAG = "Encoding";
        private const string LANGUAGE_TAG = "Language";
        private const string FONT_OVERRIDE_TAG = "Font";
        private const string TAG_DEFAULT = "DEFAULT";
        private const string TAG_DIRECTION_LEFT = "LEFT";
        private const string TAG_DIRECTION_RIGHT = "RIGHT";
        private const string TAG_ON = "ON";
        private const string TAG_OFF = "OFF";
        private const string ANNOTATE_OBSOLETE = "OBSOLETE";

        private string _name;
        private string _fileName;
        private bool _modified;
        private int _gameID = 0;
        private string _gameName = string.Empty;
        private int? _normalFont;
        private int? _speechFont;
        private bool? _rightToLeftText;
        private bool _autoTranslateParserSaid = false;
        private string _encodingHint;
        private Encoding _encoding;
        private string _baseLanguage;
        private string _language;
        private Dictionary<int, FontOverride> _fontOverrides = new Dictionary<int, FontOverride>();
        private Dictionary<string, TranslationEntry> _translatedEntries = new Dictionary<string, TranslationEntry>();
        // PO header fields, in the order found in the source file; values are kept escaped
        private List<KeyValuePair<string, string>> _headerFields = new List<KeyValuePair<string, string>>();

        public Translation(string name, string baseLanguage)
        {
            this.Name = name;
            _modified = false;
            _normalFont = null;
            _speechFont = null;
            _rightToLeftText = null;
            EncodingHint = "UTF-8";
            _baseLanguage = baseLanguage;
            _language = "en_US";
        }

        public Translation(XmlNode node, string baseLanguage)
        {
            this.Name = SerializeUtils.GetElementString(node, "Name");
            _modified = false;
            _normalFont = null;
            _speechFont = null;
            _rightToLeftText = null;
            _encodingHint = null;
            _encoding = Encoding.Default;
            _baseLanguage = baseLanguage;
            try
            {
                LoadData();
            }
            catch (Exception)
            {
                _translatedEntries.Clear(); // clear on failure
            }
        }

        public string Name
        {
            get { return _name; }
            set
            {
                _name = value;
                _fileName = _name + TRANSLATION_SOURCE_FILE_EXTENSION;
            }
        }
        
        public string FileName
        {
            get { return _fileName; }
        }

        public string CompiledFileName
        {
            get { return _name + TRANSLATION_COMPILED_FILE_EXTENSION; }
        }

        public int GameID
        {
            get { return _gameID; }
            set { _gameID = value; }
        }

        public string GameName
        {
            get { return _gameName; }
            set { _gameName = value; }
        }

        // FIXME: this storage does not fully matches the PO format capability:
        // the PO format allows to have a combined key made of a msgid + msgctxt pair,
        // meaning there may be two identical untranslated lines with different context.
        public Dictionary<string, TranslationEntry> TranslatedEntries
        {
            get { return _translatedEntries; }
            set { _translatedEntries = value; }
        }

        /// <summary>
        /// TranslatedLines is a deprecated API, now a stub that prevents older plugins
        /// and tools from throwing exceptions.
        /// TODO: consider what to do with this in the future:
        /// - remove completely?
        /// - fill returned dictionary with actual values?
        /// </summary>
        [Obsolete]
        public Dictionary<string, string> TranslatedLines
        {
            get { return new Dictionary<string, string>(); }
        }

        public int? NormalFont
        {
            get { return _normalFont; }
            set { _normalFont = value; }
        }

        public int? SpeechFont
        {
            get { return _speechFont; }
            set { _speechFont = value; }
        }

        public bool? RightToLeftText
        {
            get { return _rightToLeftText; }
            set { _rightToLeftText = value; }
        }

        public bool AutoTranslateParserSaid
        {
            get { return _autoTranslateParserSaid; }
            set { _autoTranslateParserSaid = value; }
        }

        public string EncodingHint
        {
            get { return _encodingHint; }
            set
            {
                _encodingHint = value;
                _encoding = Encoding.Default;
                if (!string.IsNullOrEmpty(value))
                {
                    if (string.Compare(_encodingHint, "UTF-8", true) == 0)
                        _encoding = new UTF8Encoding(false); // UTF-8 w/o BOM
                }
            }
        }

        public Encoding Encoding
        {
            get { return _encoding; }
        }

        public string TextLanguage
        {
            get { return _language; }
            set { _language = value; }
        }

        /// <summary>
        /// Contains definitions of font overrides provided for the given font indexes;
        /// these have to be used when this translation is loaded at runtime.
        /// If the Font object contains a real ID, that means that the override
        /// should replace a font with another existing font (so long as it's valid).
        /// Otherwise, this instructs that the new font should be generated with
        /// the given set of properties.
        /// </summary>
        public Dictionary<int, FontOverride> FontOverrides
        {
            get { return _fontOverrides; }
            set { _fontOverrides = value; }
        }

        public bool Modified
        {
            get { return _modified; }
            set { _modified = value; }
        }

        public string BaseLanguage
        {
            get { return _baseLanguage; }
            set { _baseLanguage = value; }
        }

        public void ToXml(XmlTextWriter writer)
        {
            writer.WriteStartElement("Translation");
            writer.WriteElementString("Name", _name);
            writer.WriteEndElement();
        }

        public void SaveData()
        {
            using (StreamWriter sw = new StreamWriter(FileName, false, _encoding))
            {
                SaveDataImpl(sw);
            }
            Modified = false;
        }

        public void SaveData(StreamWriter sw)
        {
            SaveDataImpl(sw);
            Modified = false;
        }

        /// <summary>
        /// Writes translation source file into the provided stream.
        /// </summary>
        private void SaveDataImpl(StreamWriter sw)
        {
            string encoding = _encodingHint ?? "ASCII";
            sw.WriteLine("# AGS TRANSLATION SOURCE FILE");
            sw.WriteLine("# This is a PO file generated according to the gettext specifications.");
            sw.WriteLine("# Special characters such as %%s symbolise things within the game,");
            sw.WriteLine("# so should be left in an appropriate place in the message.");
            sw.WriteLine("# ");
            sw.WriteLine("# Game identification: this lets game to detect a translation meant");
            sw.WriteLine("# for another game. These may be left blank too.");
            sw.WriteLine($"# $GameID={(_gameID != 0 ? _gameID.ToString() : string.Empty)}");
            sw.WriteLine($"# $GameName={_gameName}");
            sw.WriteLine("# ** Translation settings are below");
            sw.WriteLine("# ** Leave them as \"DEFAULT\" to use the game settings");
            sw.WriteLine("# The normal font to use - DEFAULT or font number");
            sw.WriteLine("# $NormalFont=" + _normalFont.NullableToString(TAG_DEFAULT));
            sw.WriteLine("# The speech font to use - DEFAULT or font number");
            sw.WriteLine("# $SpeechFont=" + _speechFont.NullableToString(TAG_DEFAULT));
            sw.WriteLine("# Text direction - DEFAULT, LEFT or RIGHT");
            sw.WriteLine("# $TextDirection=" + ((_rightToLeftText == true) ? TAG_DIRECTION_RIGHT : ((_rightToLeftText == null) ? TAG_DEFAULT : TAG_DIRECTION_LEFT)));
            sw.WriteLine("# Text encoding hint - ASCII or UTF-8");
            sw.WriteLine("# $Encoding=" + encoding);
            sw.WriteLine("# Text language, use standard locale strings, like 'en', 'en_US', etc");
            sw.WriteLine($"# $Language={( _language != null ? _language.Replace('-', '_') : string.Empty )}");
            sw.WriteLine("# Whether engine should translate Parser.Said strings automatically - ON or OFF");
            sw.WriteLine($"# $AutoTranslateParserSaid={(_autoTranslateParserSaid ? TAG_ON : TAG_OFF)}");
            if (_fontOverrides.Count != 0)
            {
                WriteFontOverrides(sw);
            }
            sw.WriteLine("#  ");
            sw.WriteLine("# ** IT IS SUGGESTED TO USE A THIRD-PARTY TOOL TO EDIT THIS FILE");
            // PO metadata
            sw.WriteLine("msgid \"\"");
            sw.WriteLine("msgstr \"\"");
            foreach (var field in MakeHeaderFields(encoding))
                sw.WriteLine($"\"{field.Key}: {field.Value}\\n\"");

            foreach (string key in _translatedEntries.Keys)
            {
                TranslationEntry entry = _translatedEntries[key];
                sw.WriteLine("");
                // Comments are written in the order used by gettext tools
                foreach (string comment in entry.TranslatorComments)
                    sw.WriteLine(string.IsNullOrEmpty(comment) ? "#" : $"# {comment}");
                foreach (string comment in entry.ExtractedComments)
                    sw.WriteLine($"#. {comment}");
                WriteSourceReferences(sw, entry.SourceReferences);
                if (entry.Flags.Count > 0)
                    sw.WriteLine($"#, {string.Join(", ", entry.Flags)}");
                foreach (string metadata in entry.Metadata)
                    sw.WriteLine(metadata);
                if (entry.Context != null)
                    sw.WriteLine($"msgctxt \"{entry.Context}\"");
                sw.WriteLine($"msgid \"{entry.Key}\"");
                sw.WriteLine($"msgstr \"{entry.Value}\"");
            }
            if (_translatedEntries.Count > 0)
                sw.WriteLine("");
        }

        /// <summary>
        /// Writes source references, wrapping them on multiple lines similar to gettext tools.
        /// </summary>
        private static void WriteSourceReferences(StreamWriter sw, List<string> references)
        {
            const int MaxLineWidth = 79;
            StringBuilder sb = new StringBuilder();
            foreach (var reference in references)
            {
                if (sb.Length > 0 && sb.Length + 1 + reference.Length > MaxLineWidth)
                {
                    sw.WriteLine(sb.ToString());
                    sb.Clear();
                }
                sb.Append(sb.Length == 0 ? "#: " : " ").Append(reference);
            }
            if (sb.Length > 0)
                sw.WriteLine(sb.ToString());
        }

        /// <summary>
        /// Generates PO header fields: keeps the ones found in the loaded source file
        /// (e.g. written by translator's tools), and updates the ones that AGS controls.
        /// </summary>
        private List<KeyValuePair<string, string>> MakeHeaderFields(string encoding)
        {
            var fields = new List<KeyValuePair<string, string>>(_headerFields);
            if (fields.Count == 0)
            {
                fields.Add(new KeyValuePair<string, string>("Last-Translator", string.Empty));
                fields.Add(new KeyValuePair<string, string>("Language-Team", string.Empty));
            }
            SetHeaderField(fields, "Language", EscapeText(_language != null ? _language.Replace('-', '_') : string.Empty));
            SetHeaderField(fields, "X-Source-Language", EscapeText(_baseLanguage ?? string.Empty));
            SetHeaderField(fields, "MIME-Version", "1.0");
            SetHeaderField(fields, "Content-Type", "text/plain; charset=" + (encoding == "ASCII" ? "ISO-8859-1" : encoding));
            SetHeaderField(fields, "Content-Transfer-Encoding", "8bit");
            SetHeaderField(fields, "X-Generator", "AGS " + Version.AGS_EDITOR_VERSION);
            return fields;
        }

        private static void SetHeaderField(List<KeyValuePair<string, string>> fields, string key, string value)
        {
            int index = fields.FindIndex(f => string.Compare(f.Key, key, true) == 0);
            if (index >= 0)
                fields[index] = new KeyValuePair<string, string>(key, value);
            else
                fields.Add(new KeyValuePair<string, string>(key, value));
        }

        /// <summary>
        /// Parses PO header fields from the (escaped) msgstr of the header entry.
        /// </summary>
        private void ReadHeaderFields(string header)
        {
            _headerFields.Clear();
            foreach (var line in header.Split(new string[] { "\\n" }, StringSplitOptions.RemoveEmptyEntries))
            {
                int sep = line.IndexOf(':');
                if (sep <= 0)
                    continue;
                _headerFields.Add(new KeyValuePair<string, string>(
                    line.Substring(0, sep).Trim(), line.Substring(sep + 1).Trim()));
            }
        }

        private enum ParseState
        {
            NewEntry,
            ParsingMeta,
            ParsingContext,
            ParsingId,
            ParsingString
        }

        /// <summary>
        /// Loads translation data from the source file (TRS).
        /// Throws IO exceptions.
        /// </summary>
        public void LoadData()
        {
            TryLoadData();
        }

        /// <summary>
        /// Loads translation data from the source file (PO).
        /// Suppresses exceptions and returns error messages.
        /// </summary>
        public CompileMessages TryLoadData()
        {
            CompileMessages errors = new CompileMessages();
            try
            {
                bool result = false;
                string old_encoding = _encodingHint;
                using (StreamReader sr = new StreamReader(FileName, _encoding))
                {
                    result = LoadDataImpl(sr, errors);
                }
                if (!result && (string.Compare(old_encoding, _encodingHint) != 0))
                {
                    // try again with the new encoding
                    using (StreamReader sr = new StreamReader(FileName, _encoding))
                    {
                        result = LoadDataImpl(sr, errors);
                    }
                }
            }
            catch (Exception e)
            {
                errors.Add(new CompileError(string.Format("Failed to load translation from {0}: \n{1}", FileName, e.Message)));
                _translatedEntries.Clear(); // clear on failure
            }
            return errors;
        }

        public CompileMessages TryLoadData(StreamReader sr)
        {
            CompileMessages errors = new CompileMessages();
            try
            {
                if (!LoadDataImpl(sr, errors))
                {
                    return errors;
                }
            }
            catch (Exception e)
            {
                errors.Add(new CompileError(string.Format("Failed to load translation: \n{0}", e.Message)));
                _translatedEntries.Clear(); // clear on failure
            }
            return errors;
        }

        // TODO: frankly I am not convinced that the TRS file reading/writing should be
        // in this Translation class. It might be more convenient to have them elsewhere,
        // in a translation file parser/serializer.
        private bool LoadDataImpl(StreamReader sr, CompileMessages errors)
        {
            _fontOverrides = new Dictionary<int, FontOverride>();
            _translatedEntries = new Dictionary<string, TranslationEntry>();
            _headerFields.Clear();
            string old_encoding = _encodingHint;

            ParseState state = ParseState.NewEntry;
            TranslationEntry entry = new TranslationEntry();

            // Stores the currently parsed entry, if it's complete
            Action finishEntry = () =>
            {
                if (state == ParseState.ParsingString)
                {
                    if (!string.IsNullOrEmpty(entry.Key))
                    {
                        // Fix any unescaped quotes left by the older Editor versions
                        entry.Key = EscapeUnescapedQuotes(entry.Key);
                        entry.Value = EscapeUnescapedQuotes(entry.Value);
                        _translatedEntries[entry.Key] = entry;
                    }
                    else
                    {
                        // an entry with an empty msgid is the PO header
                        ReadHeaderFields(entry.Value);
                    }
                }
                state = ParseState.NewEntry;
                entry = new TranslationEntry();
            };

            {
                string line;
                while ((line = sr.ReadLine()) != null)
                {
                    string extracted_string = POExtractString(line);

                    // Font overrides were mistakenly written in the legacy TRS format by some Editor versions
                    if (line.StartsWith("//#" + FONT_OVERRIDE_TAG))
                    {
                        ReadSpecialTags(line.Substring(3));
                        continue;
                    }

                    // Case 1: encountered metadata
                    // TODO: track different types of metadata (flags, comments, etc)
                    if (line.StartsWith("#"))
                    {
                        if (line.StartsWith("# $"))
                        {
                            ReadSpecialTags(line.Substring(3));
                            if (string.Compare(old_encoding, _encodingHint) != 0)
                            {
                                // Source file requires different encoding
                                return false;
                            }
                        }
                        else if (line.StartsWith("#:"))
                        {
                            entry.SourceReferences.AddRange(TranslationEntry.SplitReferences(line.Substring(2)));
                        }
                        else if (line.StartsWith("#."))
                        {
                            entry.ExtractedComments.Add(StripCommentPrefix(line, 2));
                        }
                        else if (line.StartsWith("#,"))
                        {
                            foreach (var flag in line.Substring(2).Split(','))
                            {
                                string f = flag.Trim();
                                if (f.Length > 0 && !entry.Flags.Contains(f))
                                    entry.Flags.Add(f);
                            }
                        }
                        else if (line.Length == 1 || line[1] == ' ' || line[1] == '\t')
                        {
                            entry.TranslatorComments.Add(StripCommentPrefix(line, 1));
                        }
                        else
                        {
                            // Any other metadata, for which we do not have a explicit field yet
                            entry.Metadata.Add(line);
                        }
                        continue;
                    }

                    // Case 2: we got a string, figure out what to do with it
                    if (extracted_string != null)
                    {
                        // start of a new type
                        if (line.StartsWith("msgctxt")) // context
                        {
                            state = ParseState.ParsingContext;
                            entry.Context = extracted_string;
                            continue;
                        }
                        if (line.StartsWith("msgid")) // string id, untranslated
                        {
                            state = ParseState.ParsingId;
                            entry.Key = extracted_string;
                            continue;
                        }
                        if (line.StartsWith("msgstr")) // translated string
                        {
                            state = ParseState.ParsingString;
                            entry.Value = extracted_string;
                            continue;
                        }

                        // strings can be split on multiple lines, usually the first will be empty, but no guarantee
                        // an external tool may have done that, so let's append
                        switch (state)
                        {
                            case ParseState.ParsingContext:
                                entry.Context += extracted_string;
                                continue;

                            case ParseState.ParsingId:
                                entry.Key += extracted_string;
                                continue;

                            case ParseState.ParsingString:
                                entry.Value += extracted_string;
                                continue;

                            default: continue; // ignore orphaned strings since we can't throw errors
                        }
                    }

                    // Case 3: we were processing an entry and encountered an empty line
                    if (state != ParseState.NewEntry && line.Trim().Length == 0)
                    {
                        finishEntry();
                    }
                } // while

                // The last entry is not necessarily followed by an empty line
                finishEntry();
            }
            return true;
        }

        // Returns the comment's text, without the comment mark and a single separating space
        private static string StripCommentPrefix(string line, int prefixLength)
        {
            string text = line.Substring(prefixLength);
            return text.StartsWith(" ") ? text.Substring(1) : text;
        }

        private readonly Regex POString = new Regex("^(?:msgctxt |msgid |msgstr |)+\"(.*)\"$", RegexOptions.Compiled | RegexOptions.IgnoreCase);

        // utility to extract a string between quotes, but only if it meets the format
        private string POExtractString(string line)
        {
            if (line == null) return null;

            Match match = POString.Match(line);
            if (match.Success)
                return (match.Groups[1].Value);

            return null;
        }

        /// <summary>
        /// Converts escape sequences in the PO string into the actual characters.
        /// Follows the same rules as the AGS script compiler, because the translation
        /// keys must match the strings compiled into the game.
        /// Unknown escape sequences are kept as-is.
        /// </summary>
        public static string UnescapeText(string text)
        {
            if (string.IsNullOrEmpty(text) || text.IndexOf('\\') < 0)
                return text;

            StringBuilder builder = new StringBuilder(text.Length);
            for (int i = 0; i < text.Length; i++)
            {
                char c = text[i];
                if (c != '\\' || i + 1 >= text.Length)
                {
                    builder.Append(c);
                    continue;
                }

                c = text[++i];
                switch (c)
                {
                    case '\'':
                    case '"':
                    case '?':
                    case '\\': builder.Append(c); continue;
                    case 'a': builder.Append('\a'); continue;
                    case 'b': builder.Append('\b'); continue;
                    case 'e': builder.Append((char)27); continue;
                    case 'f': builder.Append('\f'); continue;
                    case 'n': builder.Append('\n'); continue;
                    case 'r': builder.Append('\r'); continue;
                    case 't': builder.Append('\t'); continue;
                    case 'v': builder.Append('\v'); continue;
                }

                if (c >= '0' && c < '8')
                {
                    // up to 3 octal digits, max value 255
                    int value = c - '0';
                    for (int n = 0; n < 2 && i + 1 < text.Length; ++n)
                    {
                        int digit = text[i + 1] - '0';
                        if (digit < 0 || digit >= 8 || value * 8 + digit > 255)
                            break;
                        value = value * 8 + digit;
                        i++;
                    }
                    builder.Append((char)value);
                    continue;
                }

                if (c == 'x' && i + 1 < text.Length && Uri.IsHexDigit(text[i + 1]))
                {
                    // up to 2 hex digits
                    int value = 0;
                    for (int n = 0; n < 2 && i + 1 < text.Length && Uri.IsHexDigit(text[i + 1]); ++n)
                    {
                        value = value * 16 + Uri.FromHex(text[i + 1]);
                        i++;
                    }
                    builder.Append((char)value);
                    continue;
                }

                // unknown escape sequence
                builder.Append('\\').Append(c);
            }
            return builder.ToString();
        }

        /// <summary>
        /// Converts the text into a escaped form suitable for a PO string.
        /// </summary>
        public static string EscapeText(string text)
        {
            if (string.IsNullOrEmpty(text))
                return text;

            StringBuilder builder = new StringBuilder(text.Length);
            for (int i = 0; i < text.Length; i++)
            {
                char c = text[i];
                switch (c)
                {
                    case '\\': builder.Append("\\\\"); continue;
                    case '"': builder.Append("\\\""); continue;
                    case '\a': builder.Append("\\a"); continue;
                    case '\b': builder.Append("\\b"); continue;
                    case '\f': builder.Append("\\f"); continue;
                    case '\n': builder.Append("\\n"); continue;
                    case '\r':
                        // The engine only uses "\n" for line breaks, so "\r\n" and "\r" are both
                        // converted to "\n"; this also keeps PO content platform-independent
                        if (i + 1 < text.Length && text[i + 1] == '\n')
                            i++;
                        builder.Append("\\n");
                        continue;
                    case '\t': builder.Append("\\t"); continue;
                    case '\v': builder.Append("\\v"); continue;
                }

                if (c < ' ')
                    builder.Append('\\').Append(Convert.ToString(c, 8).PadLeft(3, '0'));
                else
                    builder.Append(c);
            }
            return builder.ToString();
        }

        /// <summary>
        /// Escapes double quotes which are not escaped yet; this is meant for fixing up
        /// strings which are supposed to be in the escaped form, but are not necessarily.
        /// </summary>
        public static string EscapeUnescapedQuotes(string text)
        {
            if (string.IsNullOrEmpty(text) || text.IndexOf('"') < 0)
                return text;

            StringBuilder builder = new StringBuilder(text.Length + 8);
            int backslashes = 0;
            foreach (char c in text)
            {
                if (c == '"' && (backslashes % 2) == 0)
                    builder.Append('\\');
                backslashes = (c == '\\') ? backslashes + 1 : 0;
                builder.Append(c);
            }
            return builder.ToString();
        }

        private void ReadSpecialTags(string line)
        {
            var keyValue = Utilities.ParseKeyValue(line);
            var key = keyValue.Key;
            var value = keyValue.Value;

            if (key == GAMEID_TAG)
            {
                int.TryParse(value, out _gameID);
            }
            else if (key == GAMENAME_TAG)
            {
                _gameName = value;
            }
            else if (key == NORMAL_FONT_TAG)
            {
                _normalFont = Utilities.ParseNullableInt(value, TAG_DEFAULT);
            }
            else if (key == SPEECH_FONT_TAG)
            {
                _speechFont = Utilities.ParseNullableInt(value, TAG_DEFAULT);
            }
            else if (key == TEXT_DIRECTION_TAG)
            {
                string directionText = value;
                if (directionText == TAG_DIRECTION_LEFT)
                {
                    _rightToLeftText = false;
                }
                else if (directionText == TAG_DIRECTION_RIGHT)
                {
                    _rightToLeftText = true;
                }
                else
                {
                    _rightToLeftText = null;
                }
            }
            else if (key == ENCODING_TAG)
            {
                EncodingHint = value;
            }
            else if (key == LANGUAGE_TAG)
            {
                TextLanguage = value;
            }
            else if (key == AUTO_PARSERSAID_TAG)
            {
                if (value == TAG_ON)
                    _autoTranslateParserSaid = true;
                else
                    _autoTranslateParserSaid = false;
            }
            else if (key.StartsWith(FONT_OVERRIDE_TAG))
            {
                int fontIndex = ParseFontN(key);
                if (fontIndex >= 0)
                {
                    if (!FontOverrides.ContainsKey(fontIndex))
                    {
                        FontOverrides.Add(fontIndex, ParseFontOverride(value));
                    }
                }
            }
        }

        /// <summary>
        /// Writes font overrides into the translation source file.
        /// </summary>
        private void WriteFontOverrides(StreamWriter sw)
        {
            foreach (var fontOverride in _fontOverrides)
            {
                StringBuilder sb = new StringBuilder();
                int fontIndex = fontOverride.Key;
                Font font = fontOverride.Value.Font;
                FontFields fields = fontOverride.Value.Fields;
                sb.Append($"# ${FONT_OVERRIDE_TAG}{fontIndex}{OPTION_SEPARATOR}");
                // The font override's source is defined either as another font's index, or font file
                if (font.ID >= 0)
                    sb.Append($"Font{font.ID};");
                else if (!string.IsNullOrEmpty(font.FontFileName))
                    sb.Append($"File={font.FontFileName};");

                //
                // Use fields value to know which properties have been specified
                //

                if (fields.HasFlag(FontFields.Size) && font.PointSize > 0)
                    sb.Append($"Size={font.PointSize.ToString()};");
                if (fields.HasFlag(FontFields.Size) && font.SizeMultiplier > 0)
                    sb.Append($"SizeMultiplier={font.SizeMultiplier.ToString()};");

                if (fields.HasFlag(FontFields.OutlineStyle))
                {
                    if (font.OutlineStyle == FontOutlineStyle.None)
                        sb.Append($"Outline=NONE;");
                    else if (font.OutlineStyle == FontOutlineStyle.Automatic)
                        sb.Append($"Outline=AUTO;");
                    else if (font.OutlineStyle == FontOutlineStyle.UseOutlineFont)
                        sb.Append($"Outline=Font{font.OutlineFont};");
                }

                if (fields.HasFlag(FontFields.AutoOutlineStyle))
                {
                    if (font.AutoOutlineStyle == FontAutoOutlineStyle.Squared)
                        sb.Append($"AutoOutline=SQUARED;");
                    else if (font.AutoOutlineStyle == FontAutoOutlineStyle.Rounded)
                        sb.Append($"AutoOutline=ROUND;");
                }

                if (fields.HasFlag(FontFields.AutoOutlineThickness) && font.AutoOutlineThickness > 0)
                    sb.Append($"AutoOutlineThickness={font.AutoOutlineThickness};");

                if (fields.HasFlag(FontFields.HeightDefinition))
                {
                    if (font.HeightDefinedBy == FontHeightDefinition.CustomValue)
                        sb.Append($"HeightDefinition=CUSTOM;");
                    else if (font.HeightDefinedBy == FontHeightDefinition.NominalHeight)
                        sb.Append($"HeightDefinition=NOMINAL;");
                    else if (font.HeightDefinedBy == FontHeightDefinition.PixelHeight)
                        sb.Append($"HeightDefinition=REAL;");

                    if (font.HeightDefinedBy == FontHeightDefinition.CustomValue && font.CustomHeightValue > 0)
                    {
                        sb.Append($"CustomHeight={font.CustomHeightValue};");
                    }
                }

                if (fields.HasFlag(FontFields.VerticalOffset) && font.VerticalOffset != 0)
                    sb.Append($"VerticalOffset={font.VerticalOffset};");
                if (fields.HasFlag(FontFields.LineSpacing) && font.LineSpacing != 0)
                    sb.Append($"LineSpacing={font.LineSpacing};");
                if (fields.HasFlag(FontFields.CharacterSpacing) && font.CharacterSpacing != 0)
                    sb.Append($"CharacterSpacing={font.CharacterSpacing};");

                sw.WriteLine(sb.ToString());
            }
        }

        /// <summary>
        /// Parses "FontN" kind of string, where N is a font's ID.
        /// </summary>
        private int ParseFontN(string value)
        {
            int fontID;
            if (value.StartsWith("Font") && int.TryParse(value.Substring(4), out fontID))
                return fontID;
            return -1;
        }

        private FontOverride ParseFontOverride(string value)
        {
            // Format:
            //    [FontN;]Property1=Value1;Property2=Value2;Property3=Value3;...
            var options = value.Split(';');
            var font = new Font();
            FontFields fields = 0;
            var keyValues = options.Select(s =>
            {
                return Utilities.ParseKeyValue(s, OPTION_SEPARATOR);
            }).ToArray();
            foreach (var option in keyValues)
            {
                if (option.Key == "Font")
                {
                    font.ID = option.Value.ParseIntOrDefault();
                }
                else if (option.Key.StartsWith("Font"))
                {
                    int reFontNumber = ParseFontN(option.Key);
                    if (reFontNumber >= 0)
                        font.ID = reFontNumber;
                }
                else if (option.Key == "File")
                {
                     font.FontFileName = option.Value;
                }
                else if (option.Key == "Size")
                {
                    font.PointSize = option.Value.ParseIntOrDefault();
                    fields |= FontFields.Size;
                }
                else if (option.Key == "SizeMultiplier")
                {
                    font.SizeMultiplier = option.Value.ParseIntOrDefault();
                    fields |= FontFields.Size;
                }
                else if (option.Key == "Outline")
                {
                    if (option.Value == "NONE")
                    {
                        font.OutlineStyle = FontOutlineStyle.None;
                    }
                    else if (option.Value == "AUTO")
                    {
                        font.OutlineStyle = FontOutlineStyle.Automatic;
                    }
                    else
                    {
                        int outFontID = ParseFontN(option.Value);
                        if (outFontID >= 0)
                        {
                            font.OutlineStyle = FontOutlineStyle.UseOutlineFont;
                            font.OutlineFont = outFontID;
                        }
                    }
                    fields |= FontFields.OutlineStyle;
                }
                else if (option.Key == "AutoOutline")
                {
                    if (option.Value == "SQUARED")
                    {
                        font.AutoOutlineStyle = FontAutoOutlineStyle.Squared;
                    }
                    else if (option.Value == "ROUND")
                    {
                        font.AutoOutlineStyle = FontAutoOutlineStyle.Rounded;
                    }
                    fields |= FontFields.AutoOutlineStyle;
                }
                else if (option.Key == "AutoOutlineThickness")
                {
                    font.AutoOutlineThickness = option.Value.ParseIntOrDefault();
                    fields |= FontFields.AutoOutlineThickness;
                }
                else if (option.Key == "HeightDefinition")
                {
                    if (option.Value == "NOMINAL")
                    {
                        font.HeightDefinedBy = FontHeightDefinition.NominalHeight;
                    }
                    else if (option.Value == "REAL")
                    {
                        font.HeightDefinedBy = FontHeightDefinition.PixelHeight;
                    }
                    else if (option.Value == "CUSTOM")
                    {
                        font.HeightDefinedBy = FontHeightDefinition.CustomValue;
                    }
                    fields |= FontFields.HeightDefinition;
                }
                else if (option.Key == "CustomHeight")
                {
                    font.CustomHeightValue = option.Value.ParseIntOrDefault();
                    fields |= FontFields.HeightDefinition;
                }
                else if (option.Key == "VerticalOffset")
                {
                    font.VerticalOffset = option.Value.ParseIntOrDefault();
                    fields |= FontFields.VerticalOffset;
                }
                else if (option.Key == "LineSpacing")
                {
                    font.LineSpacing = option.Value.ParseIntOrDefault();
                    fields |= FontFields.LineSpacing;
                }
                else if (option.Key == "CharacterSpacing")
                {
                    font.CharacterSpacing = option.Value.ParseIntOrDefault();
                    fields |= FontFields.CharacterSpacing;
                }
            }
            return new FontOverride(font, fields);
        }
    }
}
