using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace AGS.Types
{
    public class TranslationEntry
    {
        // FIXME: we're abusing PO Context to mark obsolete entries,
        // but that's a quick-port from 3.6.3, and must be reimplemented
        // as a standard PO method of commenting entries out.
        private const string CONTEXT_OBSOLETE = "OBSOLETE";
        private const string CONTEXT_PARSERWORD = "PARSERWORD";

        private string _context = null;
        private List<string> _sourceRefs = new List<string>();
        private bool _obsolete = false;
        private int _parserWordID = -1;

        public TranslationEntry()
        {
            Key = string.Empty;
            Value = string.Empty;
            Metadata = new List<string>();
        }

        public TranslationEntry(string key)
        {
            Key = key;
            Value = string.Empty;
            Metadata = new List<string>();
        }

        public TranslationEntry(string key, string value)
        {
            Key = key;
            Value = value;
            Metadata = new List<string>();
        }

        public string Context
        {
            get { return _context; }
            set
            {
                _context = value;
                _obsolete = false;
                _parserWordID = -1;
                if (!string.IsNullOrEmpty(_context))
                {
                    if (_context == CONTEXT_OBSOLETE)
                    {
                        _obsolete = true;
                    }
                    else if (_context.StartsWith(CONTEXT_PARSERWORD))
                    {
                        var keyValue = Utilities.ParseKeyValue(_context, ':');
                        if (keyValue.Key == CONTEXT_PARSERWORD)
                        {
                            _parserWordID = Utilities.ParseIntOrDefault(keyValue.Value, -1);
                        }
                    }
                }
            }
        }

        public string Key { get; set; }

        public string Value { get; set; }

        /// <summary>
        /// References to the locations in game where this text is used.
        /// Following the PO format, references may not contain whitespace.
        /// </summary>
        public List<string> SourceReferences
        {
            get { return _sourceRefs; }
            set { _sourceRefs = value ?? new List<string>(); }
        }

        /// <summary>
        /// Source references as a single whitespace-separated string,
        /// or null if there are none.
        /// </summary>
        public string SourceReference
        {
            get { return _sourceRefs.Count > 0 ? string.Join(" ", _sourceRefs) : null; }
            set { _sourceRefs = SplitReferences(value); }
        }

        public static List<string> SplitReferences(string references)
        {
            if (string.IsNullOrWhiteSpace(references))
                return new List<string>();
            return references.Split((char[])null, StringSplitOptions.RemoveEmptyEntries).ToList();
        }

        public bool IsObsolete
        {
            get { return _obsolete; }
            set
            {
                if (value)
                {
                    _context = CONTEXT_OBSOLETE;
                    _parserWordID = -1;
                }
                else if (_obsolete)
                {
                    _context = null;
                }
                _obsolete = value;
            }
        }

        /// <summary>
        /// Whether this translation item belongs to the Text Parser's words dictionary.
        /// </summary>
        public bool IsParserDictionary { get { return ParserWordID >= 0; } }

        /// <summary>
        /// Text Parser's word ID, which this translation item corresponds to.
        /// -1 if does not correspond to any.
        /// </summary>
        public int ParserWordID
        {
            get
            {
                return _parserWordID;
            }
            set
            {
                if (value >= 0)
                {
                    _context = $"{CONTEXT_PARSERWORD}:{value}";
                    _obsolete = false;
                }
                else if (_parserWordID >= 0)
                {
                    _context = null;
                }
                _parserWordID = value;
            }
        }

        /// <summary>
        /// Any kind of additional comments and annotations for this translation item.
        /// </summary>
        public List<string> Metadata { get; set; } 
    }
}
