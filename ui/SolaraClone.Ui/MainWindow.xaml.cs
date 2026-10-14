using System.IO;
using System.Windows;
using System.Windows.Input;
using ICSharpCode.AvalonEdit.Highlighting;
using ICSharpCode.AvalonEdit.Highlighting.Xshd;
using System.Xml;
using SolaraClone.Ui.ViewModels;

namespace SolaraClone.Ui;

public partial class MainWindow : Window
{
    private readonly MainViewModel _vm = new();

    public MainWindow()
    {
        InitializeComponent();
        DataContext = _vm;
        Loaded += OnLoaded;
        Closed += (_, _) => _vm.Dispose();
    }

    private static void RegisterLuauHighlighting()
    {
        var asm = typeof(MainWindow).Assembly;
        using var stream = asm.GetManifestResourceStream("SolaraClone.Ui.Editor.luau.xshd");
        if (stream is null) return;
        using var reader = new XmlTextReader(stream);
        var def = HighlightingLoader.Load(reader, HighlightingManager.Instance);
        HighlightingManager.Instance.RegisterHighlighting("Luau", new[] { ".luau", ".lua" }, def);
    }

    private async void OnLoaded(object sender, RoutedEventArgs e)
    {
        RegisterLuauHighlighting();
        await _vm.AttachAsync();
    }

    protected override void OnKeyDown(KeyEventArgs e)
    {
        if (e.Key == Key.F5) { _vm.ExecuteCommand.Execute(null); e.Handled = true; }
        base.OnKeyDown(e);
    }
}