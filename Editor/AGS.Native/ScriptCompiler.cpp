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
//
// AGS Script Compiler interface to .NET
//
//=============================================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "NativeMethods.h"
#include "NativeUtils.h"
#include "CompiledScript.h"
#include "IScriptCompiler.h"
#include "script/cs_compiler.h"
#include "script/cc_common.h"
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/string_utils.h"

using namespace System::IO;

namespace AGS
{
namespace Native
{

//
// IScriptCompiler implementation for AGS4 compiler
//
public ref class AGS4ScriptCompiler : public IScriptCompiler
{
public:
    AGS4ScriptCompiler()
    {
    }

    // Gets this compiler's identifying name
    virtual String^ GetName()
    {
        return "AGS SCOM 4";
    }
    // Gets this compiler's human-readable name
    virtual String^ GetDescription()
    {
        return "AGS v4 Script Compiler";
    }
    // Gets this compiler's list of supported extensions
    virtual List<String^>^ GetExtensions()
    {
        std::vector<std::string> cc_exts;
        ccGetExtensions(cc_exts);
        List<String^>^ exts = gcnew List<String^>();
        for (const auto &s : cc_exts)
            exts->Add(gcnew String(s.c_str()));
        return exts;
    }
    // Gets if this compiler supports building several scripts in parallel
    virtual bool DoesSupportParallelBuilds()
    {
        return true;
    }

    // Compiles the given script texts.
    // Returns a CompiledScript object, fills collection of compilation messages.
    virtual CompiledScript^ CompileScript(String^ scriptName, cli::array<String^>^ scriptTexts, ScriptCompilerOptions options, CompileMessages^ messages)
    {
        // Concatenate the whole thing together
        String^ all_the_script = "";
        for each (String^ header in scriptTexts)
            all_the_script += header;

        return CompileScript(scriptName, all_the_script, options, messages);
    }

    // Compiles the given script.
    // Returns a CompiledScript object, fills collection of compilation messages.
    virtual CompiledScript^ CompileScript(String^ scriptName, String^ scriptText, ScriptCompilerOptions options, CompileMessages^ messages)
    {
        // Prepare to convert script texts to native strings, in UTF-8 format
        TextConverter^ tcv = NativeMethods::GetGameTextConverter();
        // Set up compiler options
        uint64_t const cc_options = (UInt64)options;

        AGS::MessageHandler mh;
        std::string mainScript = tcv->ConvertToStd(scriptText);
        std::string mainScriptName = TextHelper::ConvertASCIIToStd(scriptName);
        std::unique_ptr<ccScript> cc_script(
            ccCompileText(mainScript, mainScriptName, cc_options, mh));

        auto compiler_messages = mh.GetMessages();
        int message_index = 0;
        for (auto msg = compiler_messages.begin(); msg != compiler_messages.end(); msg++)
            if (msg->Severity >= mh.kSV_UserError)
                messages->Add(gcnew CompileError(
                    message_index++,
                    gcnew String (msg->Message.c_str()),
                    gcnew String (msg->Section.c_str()),
                    static_cast<int>(msg->Lineno)));
            else
                messages->Add(gcnew CompileWarning(
                    message_index++,
                    gcnew String(msg->Message.c_str()),
                    gcnew String(msg->Section.c_str()),
                    static_cast<int>(msg->Lineno)));

        if (mh.HasError())
        {
            return nullptr;
        }

        // Success, create new CompiledData
        return gcnew CompiledScript(std::move(cc_script));
    }
};


List<IScriptCompiler^>^ NativeMethods::GetEmbeddedScriptCompilers()
{
    // Gather compilers in order of priority
    List<IScriptCompiler^>^ compilers = gcnew List<IScriptCompiler^>();
    compilers->Add(gcnew AGS4ScriptCompiler());
    return compilers;
}

} // namespace Native
} // namespace AGS
