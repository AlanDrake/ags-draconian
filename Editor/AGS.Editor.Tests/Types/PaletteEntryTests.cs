using NUnit.Framework;
using System.Drawing;

namespace AGS.Types
{
    [TestFixture]
    public class PaletteEntryTests
    {
        [Test]
        public void Clone_CopiesIndexColourAndType()
        {
            PaletteEntry entry = new PaletteEntry(7, Color.FromArgb(10, 20, 30));
            entry.ColourType = PaletteColourType.Gamewide;

            PaletteEntry copy = entry.Clone();

            Assert.AreNotSame(entry, copy);
            Assert.AreEqual(7, copy.Index);
            Assert.AreEqual(Color.FromArgb(10, 20, 30), copy.Colour);
            Assert.AreEqual(PaletteColourType.Gamewide, copy.ColourType);
        }

        [Test]
        public void Clone_ChangingTheCopyLeavesTheOriginal()
        {
            PaletteEntry entry = new PaletteEntry(3, Color.Red);

            PaletteEntry copy = entry.Clone();
            copy.Colour = Color.Blue;
            copy.ColourType = PaletteColourType.Gamewide;

            Assert.AreEqual(Color.Red, entry.Colour);
            Assert.AreEqual(PaletteColourType.Background, entry.ColourType);
        }
    }
}
