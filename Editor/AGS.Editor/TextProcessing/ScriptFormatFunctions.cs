using AGS.Types;
using AGS.Types.AutoComplete;
using System;
using System.Collections.Generic;
using System.Linq;

namespace AGS.Editor
{
    /// <summary>
    /// The script functions that take a format string: those with a parameter marked
    /// "__format" in the script headers, as in
    ///     import void Display(__format const string message, ...);
    /// The engine translates such a string and then formats it with the call's arguments
    /// (ScriptSprintf in Engine/script/script_api.cpp), so its translation must keep the
    /// placeholders.
    /// </summary>
    public class ScriptFormatFunctions
    {
        private const string FORMAT_KEYWORD = "__format";
        // The characters that may come between '%' and the conversion in a placeholder,
        // and the conversions, as ScriptSprintf parses them
        private const string PLACEHOLDER_FLAGS = "#+*-.0123456789 ";
        private const string PLACEHOLDER_CONVERSIONS = "diouxXceEfFgGaAps";

        // Global functions: the index of the format argument
        private readonly Dictionary<string, int> _functions = new Dictionary<string, int>();
        // Struct member functions by their name alone, since a call like "cEgo.Say(...)" does
        // not tell the object's type: the indexes of the format argument
        private readonly Dictionary<string, HashSet<int>> _members = new Dictionary<string, HashSet<int>>();

        public ScriptFormatFunctions(IEnumerable<ScriptAutoCompleteData> headers)
        {
            foreach (var header in headers)
            {
                foreach (var func in header.Functions)
                {
                    int index = FindFormatParameter(func);
                    if (index >= 0)
                        _functions[func.FunctionName] = index;
                }
                foreach (var struc in header.Structs)
                {
                    foreach (var func in struc.Functions)
                    {
                        int index = FindFormatParameter(func);
                        if (index < 0)
                            continue;
                        HashSet<int> indexes;
                        if (!_members.TryGetValue(func.FunctionName, out indexes))
                            _members.Add(func.FunctionName, indexes = new HashSet<int>());
                        indexes.Add(index);
                    }
                }
            }
        }

        /// <summary>
        /// Collects the format functions of the given script headers, building their
        /// autocomplete data if needed.
        /// </summary>
        public static ScriptFormatFunctions FromHeaders(IEnumerable<Script> headers)
        {
            foreach (Script header in headers.Where(h => !h.AutoCompleteData.Populated))
                AutoComplete.ConstructCache(header, null);
            return new ScriptFormatFunctions(headers.Select(h => h.AutoCompleteData));
        }

        /// <summary>
        /// Tells whether the argument at the given index of a call to the named function is
        /// its format string. The name is as written in the call: "Display", "String.Format",
        /// "player.Say" (the type of an object is not known, so a member function of any
        /// struct with that name is taken).
        /// </summary>
        public bool IsFormatArgument(string functionName, int argIndex)
        {
            int dot = functionName.LastIndexOf(ScriptParsing.SYM_MEMBER_ACCESS);
            if (dot < 0)
            {
                int index;
                return _functions.TryGetValue(functionName, out index) && index == argIndex;
            }
            HashSet<int> indexes;
            return _members.TryGetValue(functionName.Substring(dot + 1), out indexes) && indexes.Contains(argIndex);
        }

        /// <summary>
        /// Tells whether the format string has a placeholder for an argument. "%%" is a
        /// percent sign, and so is a '%' followed by a space ("50% off" parses as "% o"),
        /// since that is far more likely in a game's text than the space flag.
        /// </summary>
        public static bool HasPlaceholders(string format)
        {
            for (int i = 0; i < format.Length; ++i)
            {
                if (format[i] != '%')
                    continue;
                if (i + 1 < format.Length && format[i + 1] == '%')
                {
                    ++i;
                    continue;
                }
                bool spaceFlag = false;
                int at = i + 1;
                for (; at < format.Length && PLACEHOLDER_FLAGS.IndexOf(format[at]) >= 0; ++at)
                    spaceFlag |= format[at] == ' ';
                if (!spaceFlag && at < format.Length && PLACEHOLDER_CONVERSIONS.IndexOf(format[at]) >= 0)
                    return true;
                i = at - 1;
            }
            return false;
        }

        // Returns the index of the parameter marked as the format string, or -1 if there's none
        private static int FindFormatParameter(ScriptFunction func)
        {
            string[] parameters = func.ParamList.Split(',');
            for (int i = 0; i < parameters.Length; ++i)
            {
                if (parameters[i].Split((char[])null, StringSplitOptions.RemoveEmptyEntries).Contains(FORMAT_KEYWORD))
                    return i;
            }
            return -1;
        }
    }
}
