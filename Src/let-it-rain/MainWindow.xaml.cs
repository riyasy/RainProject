#nullable enable

using Microsoft.Graphics.Canvas.UI;
using Microsoft.Graphics.Canvas.UI.Xaml;
using Microsoft.UI;
using Microsoft.UI.Composition.SystemBackdrops;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using Vanara.PInvoke;
using Windows.System;
using Windows.UI;
using WinRT.Interop;
using WinUIEx;
using static Vanara.PInvoke.User32;
using Window = Microsoft.UI.Xaml.Window;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace let_it_rain
{
    /// <summary>
    /// An empty window that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainWindow : Window
    {
        private readonly AppWindow _appWindow;
        private readonly TransparentTintBackdrop _transparentTintBackdrop = new() { TintColor = Color.FromArgb(0xAA, 0, 0, 0) };
        private readonly Win2dCanvasController _canvasController;
        public MainWindow()
        {
            this.InitializeComponent();

            _appWindow = GetAppWindowForCurrentWindow();
            SetupTransparentTitleBar();
            SystemBackdrop = _transparentTintBackdrop;

            _canvasController = new Win2dCanvasController(MainLayout, D2dCanvas);
             

            var overLappedPresenter = _appWindow.Presenter as OverlappedPresenter;
            if (overLappedPresenter == null) return;
            overLappedPresenter.Maximize();
            overLappedPresenter.SetBorderAndTitleBar(false, false);
            overLappedPresenter.IsAlwaysOnTop = true;
        }

        private AppWindow GetAppWindowForCurrentWindow()
        {
            var hWnd = WindowNative.GetWindowHandle(this);
            var myWndId = Win32Interop.GetWindowIdFromWindow(hWnd);
            return AppWindow.GetFromWindowId(myWndId);
        }

        private void SetupTransparentTitleBar()
        {
            SetTitleBar(AppTitlebar);
            var titleBar = _appWindow.TitleBar;
            titleBar.ExtendsContentIntoTitleBar = true;
            titleBar.ButtonBackgroundColor = Colors.Transparent;
            titleBar.ButtonInactiveBackgroundColor = Colors.Transparent;
            titleBar.ButtonForegroundColor = Colors.Gray;
        }
    }
}


