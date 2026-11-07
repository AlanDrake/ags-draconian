using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using AGS.Types;

namespace AGS.Editor.Build
{
    /// <summary>
    /// Script headers that exist only in the editor (those of plugins), written to files for
    /// agsbuild --header: each in a folder of its own under a new folder in the system's
    /// temporary folder, under the header's own file name, which agscc takes as the name of the
    /// header's section (so two headers may have the same name). Outside the project, so that
    /// agsbuild, which clears the project's _obj folder, never deletes them. Disposing deletes
    /// the files; a failure to delete them does not throw (<see cref="DeleteError"/>).
    /// </summary>
    public sealed class ScriptHeaderFiles : IDisposable
    {
        private readonly string _directory;
        private readonly List<string> _paths = new List<string>();

        /// <summary>Writes the headers in UTF-8 without a byte order mark, as the tools read scripts.</summary>
        /// <exception cref="ArgumentException">A header has no file name.</exception>
        /// <exception cref="IOException">A file could not be written; nothing is left behind.</exception>
        public ScriptHeaderFiles(IEnumerable<Script> headers)
        {
            if (headers == null)
            {
                throw new ArgumentNullException(nameof(headers));
            }
            _directory = Path.Combine(Path.GetTempPath(), "AGSEditor-headers-" + Guid.NewGuid().ToString("N"));
            try
            {
                Encoding utf8 = new UTF8Encoding(false);
                foreach (Script header in headers)
                {
                    string name = Path.GetFileName(header.FileName ?? string.Empty);
                    if (string.IsNullOrEmpty(name))
                    {
                        throw new ArgumentException("A script header has no file name.", nameof(headers));
                    }
                    string folder = Path.Combine(_directory, _paths.Count.ToString(CultureInfo.InvariantCulture));
                    Directory.CreateDirectory(folder);
                    string path = Path.Combine(folder, name);
                    File.WriteAllText(path, header.Text ?? string.Empty, utf8);
                    _paths.Add(path);
                }
            }
            catch
            {
                Dispose();
                throw;
            }
        }

        /// <summary>The files, in the order of the headers.</summary>
        public IList<string> Paths
        {
            get { return _paths.AsReadOnly(); }
        }

        /// <summary>
        /// Why <see cref="Dispose"/> could not delete the files (one still open, by a virus scanner
        /// for instance), or null: a reason for a warning, not for failing a build.
        /// </summary>
        public string DeleteError { get; private set; }

        public void Dispose()
        {
            if (!Directory.Exists(_directory))
            {
                return;
            }
            try
            {
                Directory.Delete(_directory, true);
            }
            catch (IOException ex)
            {
                DeleteError = "Could not delete " + _directory + ": " + ex.Message;
            }
            catch (UnauthorizedAccessException ex)
            {
                DeleteError = "Could not delete " + _directory + ": " + ex.Message;
            }
        }
    }
}
