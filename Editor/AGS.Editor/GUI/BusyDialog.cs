using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace AGS.Editor
{
    public interface IWorkProgress
    {
        int? Total { get; set; }
        int? Current { get; set; }
        string Message { get; set; }
        bool AutoFormatProgress { get; set; }

        void SetProgress(int current, string message);
        void SetProgress(int total, int current, string message, bool autoFormatProgress);
    }

    public partial class BusyDialog : Form, IWorkProgress
    {
        public delegate object ProcessingHandler(IWorkProgress progress, object parameter);

        private const int TIMER_INTERVAL_MS = 100;
        private System.Windows.Forms.Timer _timer;
        private int _tickCount = 0;
        private int _timeoutMs = 0;
        private ProcessingHandler _handler;
        private object _parameter;
        private volatile bool _threadFinished;
        private Exception _exceptionThrownOnThread;
        private object _result;
        private bool _allowClose = false;
        private int? _progressTotal;
        private int? _progressCurrent;
        private string _message;
        private bool _autoFormatProgress = true;
        private readonly CancellationTokenSource _cancel;

        private static Bitmap[] _icons = null;

        public BusyDialog(string message, ProcessingHandler handler, object parameter)
            : this(message, handler, parameter, 0)
        {
        }

        public BusyDialog(string message, ProcessingHandler handler, object parameter, int timeoutMs)
            : this(message, handler, parameter, timeoutMs, null)
        {
        }

        /// <param name="cancel">If not null, the dialog has a Cancel button (and Esc) that cancels
        /// this source; the handler decides how soon it stops.</param>
        public BusyDialog(string message, ProcessingHandler handler, object parameter, int timeoutMs,
            CancellationTokenSource cancel)
        {
            InitializeComponent();
            _cancel = cancel;
            if (cancel != null)
            {
                btnCancel.Visible = true;
                ClientSize = new Size(ClientSize.Width, btnCancel.Bottom + 12);
                CancelButton = btnCancel;
                btnCancel.DialogResult = DialogResult.None; // the dialog closes when the handler returns
            }
            _message = message;
            UpdateMessageLabel();
            _handler = handler;
            _parameter = parameter;
            _timeoutMs = timeoutMs;

            if (_icons == null)
            {
                _icons = new Bitmap[8];
                for (int i = 0; i < _icons.Length; i++)
                {
                    _icons[i] = Resources.ResourceManager.GetIcon("busy_blue-circle_" + (i + 1) + ".ico").ToBitmap();
                }
            }
        }

        public Exception ExceptionThrown
        {
            get { return _exceptionThrownOnThread; }
        }

        [System.Diagnostics.DebuggerStepThrough]
        public static object Show(string message, ProcessingHandler handler, object parameter)
        {
            return Show(message, handler, parameter, 0);
        }

        public static object Show(string message, ProcessingHandler handler, object parameter, int timeoutMs)
        {
            return Show(message, handler, parameter, timeoutMs, null);
        }

        /// <summary>
        /// Runs the handler on another thread while the dialog is shown, with a Cancel button if
        /// cancel is not null; returns what the handler returns, and rethrows what it throws.
        /// </summary>
        public static object Show(string message, ProcessingHandler handler, object parameter, int timeoutMs,
            CancellationTokenSource cancel)
        {
            object resultToReturn = null;
            BusyDialog dialog = new BusyDialog(message, handler, parameter, timeoutMs, cancel);
            try
            {
                dialog.ShowDialog();
                resultToReturn = dialog.Result;
                if (dialog.ExceptionThrown != null)
                {
                    // Re-throw the same type of exception
                    ConstructorInfo constructor = dialog.ExceptionThrown.GetType().GetConstructor(new Type[] { dialog.ExceptionThrown.GetType() });
                    if (constructor != null)
                    {
                        // Copy constructor
                        throw (Exception)constructor.Invoke(new object[] { dialog.ExceptionThrown });
                    }
                    else
                    {
                        constructor = dialog.ExceptionThrown.GetType().GetConstructor(new Type[] { typeof(string), typeof(Exception) });
                        if (constructor == null)
                        {
                            // default constructor
                            throw new AGSEditorException(dialog.ExceptionThrown.Message, dialog.ExceptionThrown);
                        }
                        else
                        {
                            throw (Exception)constructor.Invoke(new object[] { dialog.ExceptionThrown.Message, dialog.ExceptionThrown });
                        }
                    }
                }
            }
            finally
            {
                dialog.Dispose();
            }
            return resultToReturn;
        }

        public object Result
        {
            get { return _result; }
        }

        private void UpdateMessageLabel()
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(_message);
            if (_autoFormatProgress)
            {
                int? total = _progressTotal;
                int? current = _progressCurrent;
                string progrText = "";
                if (total.HasValue && current.HasValue)
                    progrText = string.Format("({0} / {1})", current, total);
                else if (total.HasValue)
                    progrText = string.Format("(- / {0})", total);
                else if (current.HasValue)
                    progrText = string.Format("({0} / -)", current);
                sb.Append(" ");
                sb.Append(progrText);
            }
            sb.Append(Environment.NewLine);
            lblMessage.Text = sb.ToString();

            // A bar for the progress that has a total
            int barTotal = _progressTotal ?? 0;
            progressBar.Visible = barTotal > 0;
            if (barTotal > 0)
            {
                progressBar.Maximum = barTotal;
                progressBar.Value = Math.Max(0, Math.Min(barTotal, _progressCurrent ?? 0));
            }
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            btnCancel.Enabled = false;
            btnCancel.Text = "Cancelling...";
            _cancel.Cancel();
        }

        public string Message
        {
            get { return _message; }
            set
            {
                _message = value;
                Invoke((Action)(() => { UpdateMessageLabel(); }));
            }
        }

        public bool AutoFormatProgress
        {
            get { return _autoFormatProgress; }
            set
            {
                _autoFormatProgress = value;
                Invoke((Action)(() => { UpdateMessageLabel(); }));
            }
        }

        public int? Total
        {
            get { return _progressTotal; }
            set
            {
                _progressTotal = value;
                Invoke((Action)(() => { UpdateMessageLabel(); }));
            }
        }

        public int? Current
        {
            get { return _progressCurrent; }
            set
            {
                _progressCurrent = value;
                Invoke((Action)(() => { UpdateMessageLabel(); }));
            }
        }

        public void SetProgress(int current, string message)
        {
            _progressCurrent = current;
            _message = message;
            Invoke((Action)(() => { UpdateMessageLabel(); }));
        }

        public void SetProgress(int total, int current, string message, bool autoFormatProgress)
        {
            _progressTotal = total;
            _progressCurrent = current;
            _message = message;
            _autoFormatProgress = autoFormatProgress;
            Invoke((Action)(() => { UpdateMessageLabel(); }));
        }

        private void BusyDialog_Load(object sender, EventArgs e)
        {
            _timer = new System.Windows.Forms.Timer();
            _timer.Interval = TIMER_INTERVAL_MS;
            _timer.Tick += _timer_Tick;
            _timer.Start();
            _threadFinished = false;
            _exceptionThrownOnThread = null;
            _result = null;
            btnImage.Image = _icons[0];
            new Thread(new ThreadStart(RunHandlerOnThread)).Start();
        }

        private void RunHandlerOnThread()
        {
            try
            {
                _result = _handler(this, _parameter);
            }
            catch (Exception ex)
            {
                _exceptionThrownOnThread = ex;
            }
            _threadFinished = true;
        }

        private void _timer_Tick(object sender, EventArgs e)
        {
            if (_threadFinished)
            {
                _timer.Enabled = false;
                _allowClose = true;
                this.Close();
                return;
            }
            else if ((_timeoutMs > 0) && (_tickCount * (sender as System.Windows.Forms.Timer).Interval >= _timeoutMs))
            {
                _timer.Enabled = false;
                _allowClose = true;
                _exceptionThrownOnThread = new TimeoutException("The work process took unexpectedly long time and was aborted.");
                this.Close();
                return;
            }

            _tickCount++;
            btnImage.Image = _icons[_tickCount % _icons.Length];
        }

        private void BusyDialog_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (!_allowClose)
            {
                e.Cancel = true;
            }
        }
    }
}
