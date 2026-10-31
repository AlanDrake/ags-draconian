using AGS.Editor.Components;
using AGS.Types;
using NUnit.Framework;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using static System.Net.Mime.MediaTypeNames;
using static System.Windows.Forms.VisualStyles.VisualStyleElement.TaskbarClock;

namespace AGS.Editor
{
    public class TranslationTests
    {
        private const string GeneratedTranslationHeader = @"# AGS TRANSLATION SOURCE FILE
# This is a PO file generated according to the gettext specifications.
# Special characters such as %%s symbolise things within the game,
# so should be left in an appropriate place in the message.
# 
# Game identification: this lets game to detect a translation meant
# for another game. These may be left blank too.
# $GameID=
# $GameName=
# ** Translation settings are below
# ** Leave them as ""DEFAULT"" to use the game settings
# The normal font to use - DEFAULT or font number
# $NormalFont=4
# The speech font to use - DEFAULT or font number
# $SpeechFont=8
# Text direction - DEFAULT, LEFT or RIGHT
# $TextDirection=RIGHT
# Text encoding hint - ASCII or UTF-8
# $Encoding=UTF-8
# Text language, use standard locale strings, like 'en', 'en_US', etc
# $Language=fr_FR
# Whether engine should translate Parser.Said strings automatically - ON or OFF
# $AutoTranslateParserSaid=ON
#  
# ** IT IS SUGGESTED TO USE A THIRD-PARTY TOOL TO EDIT THIS FILE
msgid """"
msgstr """"
""Last-Translator: \n""
""Language-Team: \n""
""Language: fr_FR\n""
""X-Source-Language: en_US\n""
""MIME-Version: 1.0\n""
""Content-Type: text/plain; charset=UTF-8\n""
""Content-Transfer-Encoding: 8bit\n""
""X-Generator: AGS " + AGS.Types.Version.AGS_EDITOR_VERSION + @"\n""
";

        /// <summary>
        /// Unify line endings in the input string, in case they depend on a enviroment.
        /// </summary>
        string UniformTestString(string s)
        {
            return s.Replace("\r\n", "\n");
        }

        private Translation CreateTranslationForTest()
        {
            Translation translation = new Translation("NewLanguage", "en_US");
            translation.AutoTranslateParserSaid = true;
            translation.EncodingHint = "UTF-8";
            translation.Modified = true;
            translation.NormalFont = 4;
            translation.RightToLeftText = true;
            translation.SpeechFont = 8;
            translation.TextLanguage = "fr_FR";
            return translation;
        }

        [Test]
        public void TranslationOptions_Test()
        {
            Translation translation = CreateTranslationForTest();

            Assert.AreEqual(true, translation.AutoTranslateParserSaid);
            Assert.AreEqual("NewLanguage.tra", translation.CompiledFileName);
            Assert.IsTrue(translation.Encoding != null);
            if (translation.Encoding != null)
                Assert.AreEqual("utf-8", translation.Encoding.WebName);
            Assert.AreEqual("UTF-8", translation.EncodingHint);
            Assert.AreEqual("NewLanguage.po", translation.FileName);
            Assert.AreEqual(true, translation.Modified);
            Assert.AreEqual(4, translation.NormalFont);
            Assert.AreEqual(true, translation.RightToLeftText);
            Assert.AreEqual(8, translation.SpeechFont);
            Assert.AreEqual("fr_FR", translation.TextLanguage);
        }

        private TranslationEntry CreateEntry(string text, string translation = null, string context = null, string sourceRef = null)
        {
            TranslationEntry entry = new TranslationEntry(text, translation);
            entry.Context = context;
            entry.SourceReference = sourceRef;
            return entry;
        }

        [Test]
        public void WriteEmptyTranslation_Test()
        {
            Translation translation = CreateTranslationForTest();

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader;

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void WriteTranslationWithEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour"));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir"));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader + @"
msgid ""first line""
msgstr ""première réplique""

msgid ""second line""
msgstr ""deuxième réplique""

msgid ""Don't translate this""
msgstr """"

msgid ""Good morning""
msgstr ""bonjour""

msgid ""Good evening""
msgstr ""bonsoir""

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void WriteTranslationWithExtendedEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique", "context", "source.code: 10"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique", "context", "source.code: 20"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this", "", null, "non-translated source"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour", "Good day", null));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir", "Good day", null));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader + @"
#: source.code: 10
msgctxt ""context""
msgid ""first line""
msgstr ""première réplique""

#: source.code: 20
msgctxt ""context""
msgid ""second line""
msgstr ""deuxième réplique""

#: non-translated source
msgid ""Don't translate this""
msgstr """"

msgctxt ""Good day""
msgid ""Good morning""
msgstr ""bonjour""

msgctxt ""Good day""
msgid ""Good evening""
msgstr ""bonsoir""

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void LoadTranslationBackWithEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour"));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir"));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage", "en_US");
            var errors = new CompileMessages();
            using (StreamReader sr = new StreamReader(new MemoryStream(buffer), translationEncoding))
            {
                errors = translation.TryLoadData(sr);
            }

            Assert.That(!errors.HasErrors);
            Assert.AreEqual(true, translation.AutoTranslateParserSaid);
            Assert.AreEqual("NewLanguage.tra", translation.CompiledFileName);
            Assert.IsTrue(translation.Encoding != null);
            if (translation.Encoding != null)
                Assert.AreEqual("utf-8", translation.Encoding.WebName);
            Assert.AreEqual("UTF-8", translation.EncodingHint);
            Assert.AreEqual("NewLanguage.po", translation.FileName);
            Assert.AreEqual(false, translation.Modified);
            Assert.AreEqual(4, translation.NormalFont);
            Assert.AreEqual(true, translation.RightToLeftText);
            Assert.AreEqual(8, translation.SpeechFont);
            Assert.AreEqual("fr_FR", translation.TextLanguage);

            Assert.AreEqual(5, translation.TranslatedEntries.Count);
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("first line"));
            {
                var entry = translation.TranslatedEntries["first line"];
                Assert.AreEqual("first line", entry.Key);
                Assert.AreEqual("première réplique", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("second line"));
            {
                var entry = translation.TranslatedEntries["second line"];
                Assert.AreEqual("second line", entry.Key);
                Assert.AreEqual("deuxième réplique", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Don't translate this"));
            {
                var entry = translation.TranslatedEntries["Don't translate this"];
                Assert.AreEqual("Don't translate this", entry.Key);
                Assert.AreEqual("", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Good morning"));
            {
                var entry = translation.TranslatedEntries["Good morning"];
                Assert.AreEqual("Good morning", entry.Key);
                Assert.AreEqual("bonjour", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Good evening"));
            {
                var entry = translation.TranslatedEntries["Good evening"];
                Assert.AreEqual("Good evening", entry.Key);
                Assert.AreEqual("bonsoir", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
        }

        [Test]
        public void LoadTranslationBackWithExtendedEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique", "context", "source.code: 10"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique", "context", "source.code: 20"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this", "", null, "non-translated source"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour", "Good day", null));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir", "Good day", null));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage", "en_US");
            var errors = new CompileMessages();
            using (StreamReader sr = new StreamReader(new MemoryStream(buffer), translationEncoding))
            {
                errors = translation.TryLoadData(sr);
            }

            Assert.That(!errors.HasErrors);
            Assert.AreEqual(true, translation.AutoTranslateParserSaid);
            Assert.AreEqual("NewLanguage.tra", translation.CompiledFileName);
            Assert.IsTrue(translation.Encoding != null);
            if (translation.Encoding != null)
                Assert.AreEqual("utf-8", translation.Encoding.WebName);
            Assert.AreEqual("UTF-8", translation.EncodingHint);
            Assert.AreEqual("NewLanguage.po", translation.FileName);
            Assert.AreEqual(false, translation.Modified);
            Assert.AreEqual(4, translation.NormalFont);
            Assert.AreEqual(true, translation.RightToLeftText);
            Assert.AreEqual(8, translation.SpeechFont);
            Assert.AreEqual("fr_FR", translation.TextLanguage);

            Assert.AreEqual(5, translation.TranslatedEntries.Count);
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("first line"));
            {
                var entry = translation.TranslatedEntries["first line"];
                Assert.AreEqual("first line", entry.Key);
                Assert.AreEqual("première réplique", entry.Value);
                Assert.AreEqual("context", entry.Context);
                Assert.AreEqual("source.code: 10", entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("second line"));
            {
                var entry = translation.TranslatedEntries["second line"];
                Assert.AreEqual("second line", entry.Key);
                Assert.AreEqual("deuxième réplique", entry.Value);
                Assert.AreEqual("context", entry.Context);
                Assert.AreEqual("source.code: 20", entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Don't translate this"));
            {
                var entry = translation.TranslatedEntries["Don't translate this"];
                Assert.AreEqual("Don't translate this", entry.Key);
                Assert.AreEqual("", entry.Value);
                Assert.AreEqual(null, entry.Context);
                Assert.AreEqual("non-translated source", entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Good morning"));
            {
                var entry = translation.TranslatedEntries["Good morning"];
                Assert.AreEqual("Good morning", entry.Key);
                Assert.AreEqual("bonjour", entry.Value);
                Assert.AreEqual("Good day", entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("Good evening"));
            {
                var entry = translation.TranslatedEntries["Good evening"];
                Assert.AreEqual("Good evening", entry.Key);
                Assert.AreEqual("bonsoir", entry.Value);
                Assert.AreEqual("Good day", entry.Context);
                Assert.AreEqual(null, entry.SourceReference);
            }
        }

        [Test]
        public void UpdateTranslationWithEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour"));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir"));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage", "en_US");
            var errors = new CompileMessages();
            using (StreamReader sr = new StreamReader(new MemoryStream(buffer), translationEncoding))
            {
                errors = translation.TryLoadData(sr);
            }

            Assert.That(!errors.HasErrors);

            var gameTexts = new GameTextLine[] { 
                new GameTextLine("first line"),
                // "second line" is deprecated
                // "Don't translate this" is deprecated
                new GameTextLine("Good morning"),
                new GameTextLine("Good evening"),
                new GameTextLine("This is a new line"),
                new GameTextLine("This is another new line"),
            };
            errors = TranslationsComponent.UpdateTranslation(translation, gameTexts);

            ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader + @"
msgid ""first line""
msgstr ""première réplique""

msgid ""This is a new line""
msgstr """"

msgid ""Good morning""
msgstr ""bonjour""

msgid ""Good evening""
msgstr ""bonsoir""

msgid ""This is another new line""
msgstr """"

#~ msgid ""second line""
#~ msgstr ""deuxième réplique""

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        [Test]
        public void UpdateTranslationWithExtendedEntries_Test()
        {
            Translation translation = CreateTranslationForTest();

            // WARNING: this test is tricky, because TranslatedEntries is a hash-based Dictionary,
            // which means that the order of entries may or *not* match the order of addition.
            translation.TranslatedEntries.Add("first line", CreateEntry("first line", "première réplique", "context", "source.code: 10"));
            translation.TranslatedEntries.Add("second line", CreateEntry("second line", "deuxième réplique", "context", "source.code: 20"));
            translation.TranslatedEntries.Add("Don't translate this", CreateEntry("Don't translate this", "", null, "non-translated source"));
            translation.TranslatedEntries.Add("Good morning", CreateEntry("Good morning", "bonjour", "Good day", null));
            translation.TranslatedEntries.Add("Good evening", CreateEntry("Good evening", "bonsoir", "Good day", null));

            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            var buffer = ms.GetBuffer();
            ms.Dispose();
            var translationEncoding = translation.Encoding;

            translation = new Translation("NewLanguage", "en_US");
            var errors = new CompileMessages();
            using (StreamReader sr = new StreamReader(new MemoryStream(buffer), translationEncoding))
            {
                errors = translation.TryLoadData(sr);
            }

            Assert.That(!errors.HasErrors);

            // NOTE: unfortunately, there's no way to add custom annotations into update atm,
            // but the existing custom annotations found in the source file should be kept!
            var gameTexts = new GameTextLine[] {
                new GameTextLine("first line", "source.code: 15"), // source ref changed
                // "second line" is deprecated
                // "Don't translate this" is deprecated
                new GameTextLine("Good morning", "good.source.code: 10"), // source ref added
                new GameTextLine("Good evening", "good.source.code: 20"), // source ref added
                new GameTextLine("This is a new line", "source.code: 30"),
                new GameTextLine("This is another new line", "source.code: 40"),
            };
            errors = TranslationsComponent.UpdateTranslation(translation, gameTexts);

            ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }

            var expectedResult = GeneratedTranslationHeader + @"
#: source.code: 15
msgctxt ""context""
msgid ""first line""
msgstr ""première réplique""

#: source.code: 30
msgid ""This is a new line""
msgstr """"

#: good.source.code: 10
msgctxt ""Good day""
msgid ""Good morning""
msgstr ""bonjour""

#: good.source.code: 20
msgctxt ""Good day""
msgid ""Good evening""
msgstr ""bonsoir""

#: source.code: 40
msgid ""This is another new line""
msgstr """"

#: source.code: 20
#~ msgctxt ""context""
#~ msgid ""second line""
#~ msgstr ""deuxième réplique""

";

            var result = translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer());

            Assert.AreEqual(UniformTestString(expectedResult), UniformTestString(result));
        }

        private Translation LoadTranslationFromString(string text, out CompileMessages errors)
        {
            Translation translation = new Translation("NewLanguage", "en_US");
            using (StreamReader sr = new StreamReader(new MemoryStream(Encoding.UTF8.GetBytes(text)), Encoding.UTF8))
            {
                errors = translation.TryLoadData(sr);
            }
            return translation;
        }

        private string SaveTranslationToString(Translation translation)
        {
            MemoryStream ms = new MemoryStream();
            using (StreamWriter sw = new StreamWriter(ms, translation.Encoding))
            {
                translation.SaveData(sw);
            }
            return UniformTestString(translation.Encoding.GetDecoder().GetAsString(ms.GetBuffer()));
        }

        [Test]
        public void LoadLastEntryWithoutTrailingEmptyLine_Test()
        {
            var text = "msgid \"\"\nmsgstr \"\"\n\"Language: fr\\n\"\n\nmsgid \"first\"\nmsgstr \"premier\"\n\nmsgid \"last\"\nmsgstr \"dernier\"";
            CompileMessages errors;
            Translation translation = LoadTranslationFromString(text, out errors);

            Assert.That(!errors.HasErrors);
            Assert.AreEqual(2, translation.TranslatedEntries.Count);
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("last"));
            Assert.AreEqual("dernier", translation.TranslatedEntries["last"].Value);
        }

        [Test]
        public void KeepCustomHeaderFields_Test()
        {
            var text = @"# $Encoding=UTF-8
# $Language=fr_FR
msgid """"
msgstr """"
""Project-Id-Version: My Game\n""
""Last-Translator: Jean <jean@example.com>\n""
""Language-Team: French\n""
""Language: fr\n""
""Plural-Forms: nplurals=2; plural=(n > 1);\n""
""X-Generator: Poedit 3.4\n""

msgid ""first""
msgstr ""premier""
";
            CompileMessages errors;
            Translation translation = LoadTranslationFromString(text, out errors);
            Assert.That(!errors.HasErrors);

            var result = SaveTranslationToString(translation);
            var expectedHeader = @"msgid """"
msgstr """"
""Project-Id-Version: My Game\n""
""Last-Translator: Jean <jean@example.com>\n""
""Language-Team: French\n""
""Language: fr_FR\n""
""Plural-Forms: nplurals=2; plural=(n > 1);\n""
""X-Generator: AGS " + AGS.Types.Version.AGS_EDITOR_VERSION + @"\n""
""X-Source-Language: en_US\n""
""MIME-Version: 1.0\n""
""Content-Type: text/plain; charset=UTF-8\n""
""Content-Transfer-Encoding: 8bit\n""
";
            StringAssert.Contains(UniformTestString(expectedHeader), result);
        }

        [Test]
        public void FontOverridesRoundTrip_Test()
        {
            Translation translation = CreateTranslationForTest();
            var font = new AGS.Types.Font();
            font.ID = 2;
            font.PointSize = 14;
            translation.FontOverrides.Add(0, new FontOverride(font, FontFields.Size));

            CompileMessages errors;
            translation = LoadTranslationFromString(SaveTranslationToString(translation), out errors);

            Assert.That(!errors.HasErrors);
            Assert.AreEqual(1, translation.FontOverrides.Count);
            Assert.IsTrue(translation.FontOverrides.ContainsKey(0));
            Assert.AreEqual(2, translation.FontOverrides[0].Font.ID);
            Assert.AreEqual(14, translation.FontOverrides[0].Font.PointSize);
            Assert.AreEqual(FontFields.Size, translation.FontOverrides[0].Fields);
        }

        [Test]
        public void LoadLegacyFontOverrides_Test()
        {
            var text = "//#Font1=Font3;Size=10;\nmsgid \"\"\nmsgstr \"\"\n\nmsgid \"first\"\nmsgstr \"premier\"\n";
            CompileMessages errors;
            Translation translation = LoadTranslationFromString(text, out errors);

            Assert.That(!errors.HasErrors);
            Assert.IsTrue(translation.FontOverrides.ContainsKey(1));
            Assert.AreEqual(3, translation.FontOverrides[1].Font.ID);
            Assert.AreEqual(10, translation.FontOverrides[1].Font.PointSize);
        }

        [Test]
        public void FixUnescapedQuotesOnLoad_Test()
        {
            var text = "msgid \"\"\nmsgstr \"\"\n\nmsgid \"He said \"hi\"\"\nmsgstr \"Il a dit \\\"salut\\\"\"\n";
            CompileMessages errors;
            Translation translation = LoadTranslationFromString(text, out errors);

            Assert.That(!errors.HasErrors);
            Assert.IsTrue(translation.TranslatedEntries.ContainsKey("He said \\\"hi\\\""));
            Assert.AreEqual("Il a dit \\\"salut\\\"", translation.TranslatedEntries["He said \\\"hi\\\""].Value);
        }

        [Test]
        public void EscapeUnescapeText_Test()
        {
            Assert.AreEqual("say \\\"hi\\\"\\n\\tC:\\\\dir", Translation.EscapeText("say \"hi\"\n\tC:\\dir"));
            Assert.AreEqual("say \"hi\"\n\tC:\\dir", Translation.UnescapeText("say \\\"hi\\\"\\n\\tC:\\\\dir"));
            Assert.AreEqual("a\\nb\\nc\\n\\n", Translation.EscapeText("a\r\nb\rc\n\r\n"));
            Assert.AreEqual("A\u001bB?'", Translation.UnescapeText("\\x41\\eB\\?\\'"));
            Assert.AreEqual("\x01" + "8\x7f", Translation.UnescapeText("\\0018\\177"));
            Assert.AreEqual("\\q", Translation.UnescapeText("\\q")); // unknown escape is kept
            Assert.AreEqual("a\\\"b\\\"", Translation.EscapeUnescapedQuotes("a\"b\\\""));
            Assert.AreEqual("a\\\\\\\"", Translation.EscapeUnescapedQuotes("a\\\\\""));
        }

        [Test]
        public void SourceReferencesRoundTrip_Test()
        {
            Translation translation = CreateTranslationForTest();
            var entry = CreateEntry("common line", "réplique commune");
            for (int i = 1; i <= 8; ++i)
                entry.SourceReferences.Add($"Rooms/{i}/room{i}.asc:{i * 100}");
            translation.TranslatedEntries.Add(entry.Key, entry);

            var result = SaveTranslationToString(translation);
            var expectedEntry = @"
#: Rooms/1/room1.asc:100 Rooms/2/room2.asc:200 Rooms/3/room3.asc:300
#: Rooms/4/room4.asc:400 Rooms/5/room5.asc:500 Rooms/6/room6.asc:600
#: Rooms/7/room7.asc:700 Rooms/8/room8.asc:800
msgid ""common line""
msgstr ""réplique commune""
";
            StringAssert.Contains(UniformTestString(expectedEntry), result);

            CompileMessages errors;
            translation = LoadTranslationFromString(result, out errors);
            Assert.That(!errors.HasErrors);
            CollectionAssert.AreEqual(entry.SourceReferences, translation.TranslatedEntries["common line"].SourceReferences);
        }

        [Test]
        public void MakeSourceReference_Test()
        {
            var line = GameTextLine.MakeSpeechLine(0, "text", "Room:1", 12);
            Assert.AreEqual("Room:1", line.MakeSourceReference());
            line.ScriptFileName = "Rooms\\1\\room1.asc";
            Assert.AreEqual("Rooms/1/room1.asc:12", line.MakeSourceReference());
            Assert.AreEqual("My_Plugin_data", new GameTextLine("text", " My Plugin\tdata ").MakeSourceReference());
            Assert.AreEqual(string.Empty, new GameTextLine("text").MakeSourceReference());
        }

        [Test]
        public void GatherScriptSourceReferences_Test()
        {
            var script = "function a()\n{\n  Display(\"Hello\");\n  // \"Commented\"\n  Display(\"Bye\");\n}\nfunction b()\n{\n  Display(\"Hello\");\n}\n";
            var processor = new TranslationSourceProcessor(new Game(), new CompileMessages());
            processor.ProcessText(GameTextLine.MakeScript(script, "GlobalScript.asc"), GameTextType.Script);
            processor.ProcessText(GameTextLine.MakeScript("Display(\"Hello\");", "Rooms\\2\\room2.asc", "Room:2"), GameTextType.Script);
            processor.ProcessText(new GameTextLine("Say \"Hello\"", "GUI:gMain/lblTitle"), GameTextType.ItemDescription);

            var lines = new Dictionary<string, GameTextLine>();
            foreach (var l in processor.LinesForTranslation)
                lines.Add(l.Text, l);

            Assert.AreEqual(3, lines.Count);
            Assert.AreEqual("GlobalScript.asc:3 GlobalScript.asc:9 Rooms/2/room2.asc:1", lines["Hello"].SourceRef);
            Assert.AreEqual("GlobalScript.asc:5", lines["Bye"].SourceRef);
            Assert.AreEqual("GUI:gMain/lblTitle", lines["Say \\\"Hello\\\""].SourceRef);
        }

        [Test]
        public void CommentKindsRoundTrip_Test()
        {
            // Comments are given in non-canonical order, and must be written in the gettext order
            var text = @"msgid """"
msgstr """"

#, fuzzy, c-format
#: GlobalScript.asc:10
#| msgid ""Old text""
# Translator's note
#
#. Speaker: cEgo
#: GlobalScript.asc:20
msgid ""Some text""
msgstr ""Du texte""
";
            CompileMessages errors;
            Translation translation = LoadTranslationFromString(text, out errors);
            Assert.That(!errors.HasErrors);

            var entry = translation.TranslatedEntries["Some text"];
            CollectionAssert.AreEqual(new string[] { "Translator's note", "" }, entry.TranslatorComments);
            CollectionAssert.AreEqual(new string[] { "Speaker: cEgo" }, entry.ExtractedComments);
            CollectionAssert.AreEqual(new string[] { "GlobalScript.asc:10", "GlobalScript.asc:20" }, entry.SourceReferences);
            CollectionAssert.AreEqual(new string[] { "fuzzy", "c-format" }, entry.Flags);
            CollectionAssert.AreEqual(new string[] { "#| msgid \"Old text\"" }, entry.Metadata);
            Assert.IsTrue(entry.IsFuzzy);

            var expectedEntry = @"
# Translator's note
#
#. Speaker: cEgo
#: GlobalScript.asc:10 GlobalScript.asc:20
#, fuzzy, c-format
#| msgid ""Old text""
msgid ""Some text""
msgstr ""Du texte""
";
            StringAssert.Contains(UniformTestString(expectedEntry), SaveTranslationToString(translation));

            entry.IsFuzzy = false;
            CollectionAssert.AreEqual(new string[] { "c-format" }, entry.Flags);
            entry.IsFuzzy = true;
            CollectionAssert.AreEqual(new string[] { "fuzzy", "c-format" }, entry.Flags);
        }

        [Test]
        public void UpdateRegeneratesExtractedComments_Test()
        {
            Translation translation = CreateTranslationForTest();
            var entry = CreateEntry("first line", "première réplique");
            entry.TranslatorComments.Add("keep this");
            entry.ExtractedComments.Add("old hint");
            translation.TranslatedEntries.Add(entry.Key, entry);

            var line = new GameTextLine("first line", "Character:cEgo");
            line.Comment = "Character name\nGUI label, 100 px wide";
            TranslationsComponent.UpdateTranslation(translation, new GameTextLine[] { line });

            CollectionAssert.AreEqual(new string[] { "keep this" }, entry.TranslatorComments);
            CollectionAssert.AreEqual(new string[] { "Character name", "GUI label, 100 px wide" }, entry.ExtractedComments);
        }

        [Test]
        public void GatherComments_Test()
        {
            var processor = new TranslationSourceProcessor(new Game(), new CompileMessages());
            processor.ProcessText(TextProcessingHelper.MakeItemLine("Ego", "Character:cEgo", "Character name"), GameTextType.ItemDescription);
            processor.ProcessText(TextProcessingHelper.MakeItemLine("Ego", "GUI:gMain/lblName", "GUI label, 80 px wide"), GameTextType.ItemDescription);
            processor.ProcessText(TextProcessingHelper.MakeItemLine("Ego", "Character:cEgo2", "Character name"), GameTextType.ItemDescription);

            var lines = new List<GameTextLine>(processor.LinesForTranslation);
            Assert.AreEqual(1, lines.Count);
            Assert.AreEqual("Character:cEgo GUI:gMain/lblName Character:cEgo2", lines[0].SourceRef);
            Assert.AreEqual("Character name\nGUI label, 80 px wide", lines[0].Comment);
        }

        [Test]
        public void ObsoleteEntriesRoundTrip_Test()
        {
            // Both the standard "#~" form and the legacy "OBSOLETE" context are recognized
            var text = @"msgid """"
msgstr """"

#~ msgid ""old line""
#~ msgstr ""vieille réplique""

msgctxt ""OBSOLETE""
msgid ""legacy line""
msgstr ""réplique ancienne""

# translator note
#: TextParser:5
#~ msgctxt ""PARSERWORD:5""
#~ msgid "",word,""
#~ msgstr "",mot,""

#~| msgid ""older""
#~ msgid ""multi""
#~ ""line""
#~ msgstr """"
#~ ""multi""
#~ ""ligne""

msgid ""active line""
msgstr ""réplique active""
";
            CompileMessages errors;
            Translation translation = LoadTranslationFromString(text, out errors);
            Assert.That(!errors.HasErrors);
            Assert.AreEqual(5, translation.TranslatedEntries.Count);

            Assert.IsTrue(translation.TranslatedEntries["old line"].IsObsolete);
            Assert.AreEqual("vieille réplique", translation.TranslatedEntries["old line"].Value);
            var legacy = translation.TranslatedEntries["legacy line"];
            Assert.IsTrue(legacy.IsObsolete);
            Assert.AreEqual(null, legacy.Context);
            var word = translation.TranslatedEntries[",word,"];
            Assert.IsTrue(word.IsObsolete);
            Assert.AreEqual(5, word.ParserWordID);
            var multi = translation.TranslatedEntries["multiline"];
            Assert.IsTrue(multi.IsObsolete);
            Assert.AreEqual("multiligne", multi.Value);
            CollectionAssert.AreEqual(new string[] { "#~| msgid \"older\"" }, multi.Metadata);
            Assert.IsFalse(translation.TranslatedEntries["active line"].IsObsolete);

            var expectedEntries = @"
msgid ""active line""
msgstr ""réplique active""

#~ msgid ""old line""
#~ msgstr ""vieille réplique""

#~ msgid ""legacy line""
#~ msgstr ""réplique ancienne""

# translator note
#: TextParser:5
#~ msgctxt ""PARSERWORD:5""
#~ msgid "",word,""
#~ msgstr "",mot,""

#~| msgid ""older""
#~ msgid ""multiline""
#~ msgstr ""multiligne""

";
            StringAssert.EndsWith(UniformTestString(expectedEntries), SaveTranslationToString(translation).TrimEnd(' '));
        }

        [Test]
        public void LoadEntriesWithoutEmptyLines_Test()
        {
            // Entries are not separated by empty lines, and some lines have extra whitespace
            var text = "msgid \"\"\nmsgstr \"\"\n" +
                "msgid \"first\"\nmsgstr \"premier\"\n" +
                "# translator note\n#: GlobalScript.asc:1\n" +
                "  msgid   \"second\"  \nmsgstr \"deux\"\n\"ième\"\n" +
                "msgctxt \"ctx\"\nmsgid \"third\"\nmsgstr \"troisième\"\n" +
                "#~ msgid \"old\"\n#~ msgstr \"vieux\"\n" +
                "msgid \"fourth\"\nmsgstr \"quatrième\"";
            CompileMessages errors;
            Translation translation = LoadTranslationFromString(text, out errors);

            Assert.AreEqual(0, errors.Count);
            Assert.AreEqual(5, translation.TranslatedEntries.Count);
            Assert.AreEqual("premier", translation.TranslatedEntries["first"].Value);
            Assert.AreEqual(0, translation.TranslatedEntries["first"].TranslatorComments.Count);
            var second = translation.TranslatedEntries["second"];
            Assert.AreEqual("deuxième", second.Value);
            CollectionAssert.AreEqual(new string[] { "translator note" }, second.TranslatorComments);
            CollectionAssert.AreEqual(new string[] { "GlobalScript.asc:1" }, second.SourceReferences);
            Assert.AreEqual("ctx", translation.TranslatedEntries["third"].Context);
            Assert.AreEqual(null, translation.TranslatedEntries["second"].Context);
            Assert.IsTrue(translation.TranslatedEntries["old"].IsObsolete);
            Assert.IsFalse(translation.TranslatedEntries["fourth"].IsObsolete);
        }

        [Test]
        public void LoadMalformedEntriesWithWarnings_Test()
        {
            var lines = new string[] {
                "msgid \"\"",                       // 1
                "msgstr \"\"",                      // 2
                "",                                 // 3
                "\"orphaned string\"",              // 4: string outside of entry
                "msgid \"no translation\"",         // 5: incomplete entry
                "",                                 // 6
                "msgid \"good\"",                   // 7
                "msgstr \"bon\"",                   // 8
                "msgstr \"bien\"",                  // 9: duplicate msgstr
                "\"continued\"",                    // 10: skipped along with duplicate msgstr
                "",                                 // 11
                "msgid \"unterminated",             // 12: unrecognized line
                "",                                 // 13
                "msgid \"apple\"",                  // 14
                "msgid_plural \"apples\"",          // 15: plural forms
                "msgstr[0] \"pomme\"",              // 16
                "msgstr[1] \"pommes\"",             // 17
                "",                                 // 18
                "msgid \"good\"",                   // 19: duplicate entry
                "msgstr \"très bon\"",              // 20
                "",                                 // 21
                "msgstr \"lost\"",                  // 22: msgstr without msgid
            };
            CompileMessages errors;
            Translation translation = LoadTranslationFromString(string.Join("\n", lines), out errors);

            Assert.IsFalse(errors.HasErrors);
            var warnings = new List<int>();
            foreach (var msg in errors)
            {
                Assert.IsInstanceOf<CompileWarning>(msg);
                Assert.AreEqual("NewLanguage.po", msg.ScriptName);
                warnings.Add(msg.LineNumber);
            }
            CollectionAssert.AreEqual(new int[] { 4, 5, 9, 12, 15, 19, 22 }, warnings);

            Assert.AreEqual(2, translation.TranslatedEntries.Count);
            Assert.AreEqual("très bon", translation.TranslatedEntries["good"].Value);
            Assert.AreEqual("pomme", translation.TranslatedEntries["apple"].Value);
        }

        [Test]
        public void UpdateRevivesObsoleteEntry_Test()
        {
            Translation translation = CreateTranslationForTest();
            translation.TranslatedEntries.Add("old line", CreateEntry("old line", "vieille réplique", "OBSOLETE"));
            translation.TranslatedEntries.Add("word", CreateEntry("word", "mot", "PARSERWORD:5"));
            Assert.IsTrue(translation.TranslatedEntries["old line"].IsObsolete);

            var gameTexts = new GameTextLine[] {
                new GameTextLine("old line"),
                new GameTextLine("word"), // no longer a parser word
            };
            TranslationsComponent.UpdateTranslation(translation, gameTexts);

            var oldLine = translation.TranslatedEntries["old line"];
            Assert.IsFalse(oldLine.IsObsolete);
            Assert.AreEqual(null, oldLine.Context);
            var word = translation.TranslatedEntries["word"];
            Assert.IsFalse(word.IsParserDictionary);
            Assert.AreEqual(null, word.Context);

            // Obsolete state does not affect the context anymore
            word.ParserWordID = 7;
            Assert.AreEqual("PARSERWORD:7", word.Context);
            word.IsObsolete = true;
            Assert.AreEqual(7, word.ParserWordID);
            Assert.AreEqual("PARSERWORD:7", word.Context);
            word.IsObsolete = false;
            Assert.AreEqual("PARSERWORD:7", word.Context);
        }
    }
}
