using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;

namespace AGS.Editor
{
    class DefaultRuntimeSetupPane : BigPropertySheet
    {
        public DefaultRuntimeSetupPane()
            : base(Factory.AGSEditor.CurrentGame.DefaultSetup)
        {
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
        }

        protected override string OnGetHelpKeyword()
        {
            return "Default setup";
        }

        private void LoadColorTheme(ColorTheme t)
        {
            t.ControlHelper(this, "general-settings");
            t.PropertyGridHelper(propertyGrid, "general-settings/property-grid");
        }
    }
}
