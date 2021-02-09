using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using WeifenLuo.WinFormsUI.Docking;

namespace AGS.Editor
{
    public partial class ProjectPanel : DockContent
    {
        public ProjectPanel()
        {
            InitializeComponent();
            Factory.GUIController.ColorThemes.Apply(LoadColorTheme);
        }

        public void LoadColorTheme(ColorTheme t)
        {
            t.SetColor("project-panel/background", c => BackColor = c);
            projectTree.BorderStyle = BorderStyle.None;
            t.SetColor("project-panel/project-tree/background", c => projectTree.BackColor = c);
            t.SetColor("project-panel/project-tree/foreground", c => projectTree.ForeColor = c);
            t.SetColor("project-panel/project-tree/line", c => projectTree.LineColor = c);
        }
    }
}
