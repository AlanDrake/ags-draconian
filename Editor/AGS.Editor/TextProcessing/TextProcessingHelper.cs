using AGS.Types;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace AGS.Editor
{
    public class TextProcessingHelper
    {
        public static void ProcessAllGameText(IGameTextProcessor processor, Game game, CompileMessages errors)
        {
            // Game info
            // NOTE: source references are made in "Type:Owner/Member" form, as they may not contain whitespace
            game.Settings.GameName = processor.ProcessText(game.Settings.GameName, "Game:GameName", GameTextType.ItemDescription);
            game.Settings.Description = processor.ProcessText(game.Settings.Description, "Game:Description", GameTextType.ItemDescription);
            game.Settings.DeveloperErrorText = processor.ProcessText(game.Settings.DeveloperErrorText, "Game:DeveloperErrorText", GameTextType.ItemDescription);
            game.Settings.DeveloperName = processor.ProcessText(game.Settings.DeveloperName, "Game:DeveloperName", GameTextType.ItemDescription);
            game.Settings.DeveloperURL = processor.ProcessText(game.Settings.DeveloperURL, "Game:DeveloperURL", GameTextType.ItemDescription);
            game.Settings.Genre = processor.ProcessText(game.Settings.Genre, "Game:Genre", GameTextType.ItemDescription);

            foreach (Dialog dialog in game.Dialogs)
            {
                string sourceRef = $"Dialog:{NameOrID(dialog.ScriptName, dialog.ID)}";
                foreach (DialogOption option in dialog.Options)
                {
                    option.Text = processor.ProcessText(GameTextLine.MakeSpeechLine(game.PlayerCharacter.ID, option.Text, $"{sourceRef}/Option{option.ID}"), GameTextType.DialogOption);
                }

                dialog.Script.Text = processor.ProcessText(GameTextLine.MakeScript(dialog.Script.Text, dialog.ScriptFileName, sourceRef), GameTextType.DialogScript);
            }

            foreach (ScriptAndHeader script in game.RootScriptFolder.AllItemsFlat)
            {
                string newScript = processor.ProcessText(GameTextLine.MakeScript(script.Script.Text, script.Script.FileName), GameTextType.Script);
                if (newScript != script.Script.Text)
                {
                    // Only cause it to flag Modified if we changed it
                    script.Script.Text = newScript;
                }
            }

            ProcessPropertySchema(processor, game.PropertySchema, errors);

            foreach (GUI gui in game.RootGUIFolder.AllItemsFlat)
            {
                foreach (GUIControl control in gui.Controls)
                {
                    string sourceRef = $"GUI:{NameOrID(gui.ScriptName, gui.ID)}/{NameOrID(control.ScriptName, control.ID)}";
                    GUILabel label = control as GUILabel;
                    if (label != null)
                    {
                        label.Text = processor.ProcessText(label.Text, sourceRef, GameTextType.ItemDescription);
                    }
                    else
                    {
                        GUIButton button = control as GUIButton;
                        if (button != null)
                        {
                            button.Text = processor.ProcessText(button.Text, sourceRef, GameTextType.ItemDescription);
                        }
                    }
                }
            }

            foreach (Character character in game.RootCharacterFolder.AllItemsFlat)
            {
                string sourceRef = $"Character:{NameOrID(character.ScriptName, character.ID)}";
                character.DisplayName = processor.ProcessText(character.DisplayName, sourceRef, GameTextType.ItemDescription);
                ProcessProperties(processor, game.PropertySchema, character.Properties, sourceRef, errors);
            }

            foreach (InventoryItem item in game.RootInventoryItemFolder.AllItemsFlat)
            {
                string sourceRef = $"InventoryItem:{NameOrID(item.ScriptName, item.ID)}";
                item.DisplayName = processor.ProcessText(item.DisplayName, sourceRef, GameTextType.ItemDescription);
                ProcessProperties(processor, game.PropertySchema, item.Properties, sourceRef, errors);
            }

            if (game.Settings.TranslateTextParser)
            {
                var parserWordLists = ConstructParserWordsList(game.TextParser);
                // What we do here: because parser dictionary entry may match some
                // generic game text (e.g. if it's a item's name), the cheap and dumb solution
                // that we use is that we prefix the source line with a comma
                // (this works as it's a comma-separated list).
                foreach (var wordKey in parserWordLists.Keys)
                {
                    string word = $",{parserWordLists[wordKey]}";
                    processor.ProcessText(GameTextLine.MakeParserWord(wordKey, word, $"TextParser:{wordKey}"), GameTextType.TextParserWord);
                }
            }

            Factory.AGSEditor.RunProcessAllGameTextsEvent(processor, errors);
        }

        private static void ProcessPropertySchema(IGameTextProcessor processor, CustomPropertySchema schema,
            CompileMessages errors)
        {
            foreach (var def in schema.PropertyDefinitions.Where(
                n => (n.Type == CustomPropertyType.Text) && n.Translated))
            {
                def.DefaultValue = processor.ProcessText(def.DefaultValue, $"PropertySchema:{def.Name}", GameTextType.ItemDescription);
            }
        }

        public static void ProcessProperties(IGameTextProcessor processor, CustomPropertySchema schema,
            CustomProperties props, string sourceRef, CompileMessages errors)
        {
            foreach (var def in schema.PropertyDefinitions.Where(
                n => (n.Type == CustomPropertyType.Text) && n.Translated))
            {
                CustomProperty prop;
                if (props.PropertyValues.TryGetValue(def.Name, out prop))
                {
                    prop.Value = processor.ProcessText(new GameTextLine(prop.Value, $"{sourceRef}/{def.Name}"), GameTextType.ItemDescription);
                }
            }
        }

        /// <summary>
        /// Returns the script name for use in a source reference, or the numeric ID if there's no name.
        /// </summary>
        public static string NameOrID(string scriptName, int id)
        {
            return string.IsNullOrEmpty(scriptName) ? id.ToString() : scriptName;
        }

        private static Dictionary<int, string> ConstructParserWordsList(TextParser parser)
        {
            var wordsPerID = new Dictionary<int, List<string>>();
            foreach (var word in parser.Words)
            {
                if (!wordsPerID.ContainsKey(word.WordGroup))
                    wordsPerID[word.WordGroup] = new List<string>();
                wordsPerID[word.WordGroup].Add(word.Word);
            }

            var wordLists = new Dictionary<int, string>();
            foreach (var wordList in wordsPerID)
            {
                StringBuilder sb = new StringBuilder();
                foreach (var word in wordList.Value)
                {
                    if (sb.Length > 0)
                        sb.Append(',');
                    sb.Append(word);
                }
                wordLists[wordList.Key] = sb.ToString();
            }
            return wordLists;
        }
    }
}
