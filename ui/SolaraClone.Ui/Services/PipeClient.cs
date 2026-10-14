using System;
using System.IO.Pipes;
using System.Text;
using System.Text.Json;
using System.Threading;
using System.Threading.Tasks;

namespace SolaraClone.Ui.Services;

public enum MsgType : byte
{
    Ping = 0x01, Pong = 0x02, AttachState = 0x03, Execute = 0x04,
    ExecuteDone = 0x05, Output = 0x06, ClearOutput = 0x07,
    GetInstances = 0x08, Instances = 0x09, OffsetStatus = 0x0A, Shutdown = 0x0B
}

public sealed class PipeClient : IDisposable
{
    private readonly string _pipeName;
    private NamedPipeClientStream? _stream;
    private CancellationTokenSource? _cts;

    public event Action<MsgType, string>? MessageReceived;
    public event Action? Disconnected;

    public PipeClient(string pipeName = "sc_core") => _pipeName = pipeName;

    public bool IsConnected => _stream?.IsConnected == true;

    public async Task<bool> ConnectAsync(int timeoutMs = 3000)
    {
        Dispose();
        _stream = new NamedPipeClientStream(".", _pipeName,
            PipeDirection.InOut, PipeOptions.Asynchronous);
        try { await _stream.ConnectAsync(timeoutMs); }
        catch (TimeoutException) { return false; }
        catch (Exception) { return false; }

        _cts = new CancellationTokenSource();
        _ = Task.Run(() => ReadLoopAsync(_cts.Token));
        return true;
    }

    public async Task SendAsync(MsgType type, object? payload = null)
    {
        if (_stream is null) throw new InvalidOperationException("pipe non connecte");
        var json = payload is null ? "{}" : JsonSerializer.Serialize(payload);
        var body = Encoding.UTF8.GetBytes(json);

        var head = new byte[5];
        BitConverter.GetBytes((uint)(1 + body.Length)).CopyTo(head, 0);
        head[4] = (byte)type;

        await _stream.WriteAsync(head);
        if (body.Length > 0) await _stream.WriteAsync(body);
        await _stream.FlushAsync();
    }

    private async Task ReadLoopAsync(CancellationToken ct)
    {
        var header = new byte[5];
        try
        {
            while (!ct.IsCancellationRequested)
            {
                if (!await ReadExactAsync(header, ct)) break;
                uint size = BitConverter.ToUInt32(header, 0);
                if (size == 0 || size > 64 * 1024 * 1024) break;

                var body = new byte[size - 1];
                if (body.Length > 0 && !await ReadExactAsync(body, ct)) break;

                MessageReceived?.Invoke((MsgType)header[4], Encoding.UTF8.GetString(body));
            }
        }
        catch (OperationCanceledException) { }
        catch (Exception) { }
        finally { Disconnected?.Invoke(); }
    }

    private async Task<bool> ReadExactAsync(byte[] buf, CancellationToken ct)
    {
        int off = 0;
        while (off < buf.Length)
        {
            int n = await _stream!.ReadAsync(buf.AsMemory(off), ct);
            if (n == 0) return false;
            off += n;
        }
        return true;
    }

    public void Dispose()
    {
        _cts?.Cancel();
        _cts?.Dispose();
        _cts = null;
        _stream?.Dispose();
        _stream = null;
    }
}