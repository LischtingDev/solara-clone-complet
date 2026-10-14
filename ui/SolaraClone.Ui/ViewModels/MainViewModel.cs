using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows.Input;
using SolaraClone.Ui.Services;

namespace SolaraClone.Ui.ViewModels;

public sealed class ScriptTab : INotifyPropertyChanged
{
    private string _text = "";
    public string Name { get; set; } = "script1.luau";
    public string Text
    {
        get => _text;
        set { _text = value; OnPropertyChanged(); }
    }
    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? n = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(n));
}

public sealed class MainViewModel : INotifyPropertyChanged, IDisposable
{
    private readonly PipeClient _pipe = new();
    private readonly StringBuilder _console = new();
    private string _status = "detache";
    private int _tabCounter = 1;

    public ObservableCollection<ScriptTab> Tabs { get; } = new();
    public ScriptTab? SelectedTab { get; set; }

    public string Status
    {
        get => _status;
        private set { _status = value; OnPropertyChanged(); }
    }

    public string ConsoleText => _console.ToString();

    public ICommand AttachCommand { get; }
    public ICommand ExecuteCommand { get; }
    public ICommand ClearCommand { get; }

    public MainViewModel()
    {
        Tabs.Add(new ScriptTab { Name = "script1.luau", Text = "print(\"hello from Vesta\")\n" });

        AttachCommand  = new Relay(async () => await AttachAsync());
        ExecuteCommand = new Relay(async () => await ExecuteAsync());
        ClearCommand   = new Relay(() =>
        {
            _console.Clear();
            OnPropertyChanged(nameof(ConsoleText));
            _ = _pipe.SendAsync(MsgType.ClearOutput);
        });

        _pipe.MessageReceived += OnMessage;
        _pipe.Disconnected += () => Status = "deconnecte";
    }

    public async Task AttachAsync()
    {
        Status = "connexion au pipe...";
        if (await _pipe.ConnectAsync(1500))
        {
            Status = "connecte";
            Append("info", "pipe connecte");
        }
        else
        {
            Status = "aucun core detecte (lancer injector.exe)";
            Append("warn", "pipe introuvable : lancer injector.exe --wait");
        }
    }

    private async Task ExecuteAsync()
    {
        var tab = SelectedTab ?? (Tabs.Count > 0 ? Tabs[0] : null);
        if (tab is null) return;

        if (!_pipe.IsConnected) { await AttachAsync(); if (!_pipe.IsConnected) return; }

        await _pipe.SendAsync(MsgType.Execute, new { source = tab.Text, name = tab.Name });
        Append("info", $"-- execute {tab.Name} --");
    }

    private void OnMessage(MsgType type, string payload)
    {
        switch (type)
        {
            case MsgType.Output:
            case MsgType.AttachState:
            case MsgType.OffsetStatus:
            case MsgType.ExecuteDone:
            {
                string level = "info", text = payload;
                try
                {
                    using var doc = JsonDocument.Parse(payload);
                    if (doc.RootElement.TryGetProperty("level", out var l)) level = l.GetString() ?? level;
                    if (doc.RootElement.TryGetProperty("text", out var t)) text = t.GetString() ?? text;
                    if (type == MsgType.AttachState) text = "core attache";
                    if (type == MsgType.ExecuteDone)
                        text = doc.RootElement.TryGetProperty("ok", out var ok) && ok.GetBoolean()
                            ? "-- termine --" : "-- erreur: " + text + " --";
                }
                catch (JsonException) { }
                Append(level, text);
                break;
            }
        }
    }

    private void Append(string level, string text)
    {
        var prefix = level switch
        {
            "error" => "[ERR] ",
            "warn"  => "[WRN] ",
            "lua"   => "",
            _       => "[INF] "
        };
        _console.AppendLine(prefix + text);
        OnPropertyChanged(nameof(ConsoleText));
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    private void OnPropertyChanged([CallerMemberName] string? n = null)
        => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(n));

    public void Dispose() => _pipe.Dispose();

    private sealed class Relay : ICommand
    {
        private readonly Func<Task> _fn;
        public Relay(Func<Task> fn) => _fn = fn;
        public Relay(Action fn) => _fn = () => { fn(); return Task.CompletedTask; };
        public bool CanExecute(object? _) => true;
        public async void Execute(object? _) => await _fn();
        public event EventHandler? CanExecuteChanged;
    }
}