using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class TranslationGenerator
    {
        private ICollection<GameTextLine> _linesForTranslation;

        public ICollection<GameTextLine> LinesForTranslation
        {
            get { return _linesForTranslation; }
        }

        /// <summary>
        /// Gathers the game's texts for translation; given the functions that take a format
        /// string, marks the texts that scripts pass to them as the format.
        /// </summary>
        public CompileMessages CreateTranslationList(Game game, ScriptFormatFunctions formatFunctions)
        {
            CompileMessages errors = new CompileMessages();

            TranslationSourceProcessor processor = new TranslationSourceProcessor(game, errors, formatFunctions);

            TextProcessingHelper.ProcessAllGameText(processor, game, errors);

            _linesForTranslation = processor.LinesForTranslation;

            return errors;
        }
    }
}
