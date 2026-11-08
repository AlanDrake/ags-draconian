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
#include <stack>

#include "preproc/cc_macrotable.h"
#include "util/string.h"
#include "util/version.h"


namespace AGS
{
namespace Preprocessor
{

enum class ErrorCode
{
    None = 0,
    MacroNameMissing = 1,
    MacroDoesNotExist,
    MacroAlreadyExists,
    MacroNameInvalid,
    UnknownPreprocessorDirective,
    UserDefinedError,
    EndIfWithoutIf,
    IfWithoutEndIf,
    ElseIfWithoutIf,
    InvalidVersionNumber,
    UnterminatedString,
    InvalidCharacter
};

struct Error
{
    ErrorCode Type = ErrorCode::None;
    Common::String Message = nullptr;
};

class Preprocessor
{
private:
    bool _inMultiLineComment = false;
    MacroTable _macros = MacroTable();
    int _lineNumber = 0;
    Common::String _scriptName;
    // Optional "module" (a group of types and imports) declared by "#module <name>"; it is added
    // to the new-script marker as "<script>;<module>", which the compiler's scanner reads
    Common::String _moduleName;
    Common::Version _applicationVersion;
    // Conditional statement stack remembers the results of all the nested conditions
    // that we have entered.
    std::stack<bool> _conditionalStatements;
    // Negative counter: it is incremented each time we enter a FALSE condition,
    // and decremented each time we exit a previous FALSE condition.
    uint32_t _negativeCounter = 0u;
    std::vector<Error> _errors;

    void LogError(ErrorCode error, const Common::String &message = nullptr);

    void ProcessConditionalDirective(const Common::String &directive, Common::String &line);

    bool DeletingCurrentLine() const;

    Common::String GetNextWord(Common::String &text, bool trimText = true, bool includeDots = false);

    Common::String RemoveComments(Common::String text);

    Common::String PreProcessDirective(Common::String &line);

    Common::String PreProcessLine(const Common::String &lineToProcess);

public:
    static const Error NoError;
    Error GetLastError() const;
    void SetAppVersion(const Common::String &version);
    void DefineMacro(const Common::String &name, const Common::String &value);

    Common::String Preprocess(const Common::String &script, const Common::String &scriptName);
};

} // namespace Preprocessor
} // namespace AGS
