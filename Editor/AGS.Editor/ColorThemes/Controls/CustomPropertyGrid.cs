using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

// The only purpose of this class is to let us theme the toolstrip
namespace AGS.Editor
{
    public class CustomPropertyGrid : PropertyGrid
    {
        public CustomPropertyGrid() : base()
        {
        }

        public void SetRenderer(ToolStripRenderer renderer)
        {
            ToolStripRenderer = renderer;
        }

        protected override void CreateHandle()
        {
            base.CreateHandle();
            if (!this.DesignMode)
            {
                ScrollBar sb = Hacks.GetPropertyGridScrollBar(this);
                Hacks.DarkThemeControl(sb.Handle);
            }
            //Hacks.DarkThemeControl(this.Handle); // only toolstrip is getting colored...

        }
    }
}
