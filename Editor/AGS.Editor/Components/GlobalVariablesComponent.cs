using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class GlobalVariablesComponent : BaseComponent, IGlobalVariablesController
    {
        private const string TOP_LEVEL_COMMAND_ID = "GlobalVariables";
        public const string GLOBAL_VARS_HEADER_FILE_NAME = "_GlobalVariables.ash";
        private const string ICON_KEY = "GlobalVarsIcon";
        
        private GlobalVariablesEditor _editor;
        private ContentDocument _document;
        private Script _scriptHeader;

        public GlobalVariablesComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            RecreateDocument();
            _scriptHeader = new Script(GLOBAL_VARS_HEADER_FILE_NAME, string.Empty, true);
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("globalvars.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Global variables", ICON_KEY);
            _editor.GlobalVariableChanged += _editor_GlobalVariableChanged;
            _agsEditor.GetScriptHeaderList += _agsEditor_GetScriptHeaderList;
        }

        public void SelectGlobalVariable(string variableName)
        {
            ShowGlobalVariablesPane(TOP_LEVEL_COMMAND_ID);
            _editor.SelectGlobalVariable(variableName);
        }

        private void _editor_GlobalVariableChanged()
        {
            UpdateScriptHeader();
        }

        private void UpdateScriptHeader()
        {
            _scriptHeader.Text = Tasks.MakeGlobalVariablesHeader(_agsEditor.CurrentGame.GlobalVariables.ToList());
            AutoComplete.ConstructCache(_scriptHeader, null);
        }

        private void _agsEditor_GetScriptHeaderList(GetScriptHeaderListEventArgs evArgs)
        {
            UpdateScriptHeader();

            if (_scriptHeader.Text.Length > 0)
            {
                evArgs.ScriptHeaders.Add(_scriptHeader);
            }
        }

        private void RecreateDocument()
        {
            if (_document != null)
            {
                _document.Dispose();
            }
            _editor = new GlobalVariablesEditor(_agsEditor.CurrentGame);
            _document = new ContentDocument(_editor, "Global Variables", this, ICON_KEY);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.GlobalVariables; }
        }

        public override void CommandClick(string controlID)
        {
            ShowGlobalVariablesPane(controlID);
		}

        private void ShowGlobalVariablesPane(string controlID)
        {
            if (_document.Control.IsDisposed)
            {
                RecreateDocument();
            }
            _document.TreeNodeID = controlID;
            _guiController.AddOrShowPane(_document);
        }

        public override void RefreshDataFromGame()
        {
            _guiController.RemovePaneIfExists(_document);
            RecreateDocument();
        }

    }
}
