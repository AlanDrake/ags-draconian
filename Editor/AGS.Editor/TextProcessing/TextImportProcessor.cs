using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class TextImportProcessor : GameSpeechProcessor
    {
        private Dictionary<string, TranslationEntry> _translationToUse;

        public TextImportProcessor(Game game, CompileMessages errors, Dictionary<string, TranslationEntry> translationToUse)
            : base(game, errors, makesChanges: true, processHotspotAndObjectDescriptions: true,
                  lookupForFunctionCalls: false, lookupForOuterFunctionCalls: false)
        {
            _translationToUse = translationToUse;
        }

        protected override bool ParseFunctionCall(string scriptCodeExtract, out int characterID)
        {
            // dummy character ID 0, because we don't really care
            characterID = 0;
            return true;
        }

        protected override string CreateSpeechLine(GameTextLine textLine, GameTextType textType)
        {
            var text = textLine.Text;
            TranslationEntry entry;
            if (_translationToUse.TryGetValue(MakeTranslationKey(text, textType), out entry) &&
                !string.IsNullOrEmpty(entry.Value) && !entry.IsObsolete && !entry.IsFuzzy)
            {
                return MakeTextFromTranslation(entry.Value, textType);
            }
            return text;
        }

    }
}
