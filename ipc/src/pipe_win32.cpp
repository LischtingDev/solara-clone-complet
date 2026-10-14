#include "ipc/pipe.hpp"

#include <cstring>
#include <utility>

namespace sc::ipc {

namespace {

constexpr size_t kChunk = 64 * 1024;

bool overlapped_io(HANDLE pipe, HANDLE ev, bool is_write,
                   uint8_t* buf, size_t len, DWORD* done, int timeout_ms) {
    OVERLAPPED ov{};
    ov.hEvent = ev;
    ResetEvent(ev);

    DWORD transferred = 0;
    BOOL ok = is_write
        ? WriteFile(pipe, buf, static_cast<DWORD>(len), &transferred, &ov)
        : ReadFile(pipe, buf, static_cast<DWORD>(len), &transferred, &ov);

    if (!ok) {
        const DWORD err = GetLastError();
        if (err != ERROR_IO_PENDING) {
            if (err == ERROR_BROKEN_PIPE || err == ERROR_PIPE_NOT_CONNECTED) {
                if (done) *done = 0;
                return false;
            }
            if (done) *done = 0;
            return false;
        }
        const DWORD w = WaitForSingleObject(ev, timeout_ms < 0 ? INFINITE : static_cast<DWORD>(timeout_ms));
        if (w != WAIT_OBJECT_0) {
            CancelIoEx(pipe, &ov);
            WaitForSingleObject(ev, 100);
            if (done) *done = 0;
            return false;
        }
        if (!GetOverlappedResult(pipe, &ov, &transferred, FALSE)) {
            if (done) *done = 0;
            return false;
        }
    }
    if (done) *done = transferred;
    return true;
}

void drain(std::vector<uint8_t>& rx, size_t n) {
    if (n >= rx.size()) { rx.clear(); return; }
    rx.erase(rx.begin(), rx.begin() + static_cast<ptrdiff_t>(n));
}

} // namespace

std::vector<uint8_t> encode(const Message& msg) {
    const uint32_t size = static_cast<uint32_t>(1 + msg.payload.size());
    std::vector<uint8_t> out(4 + size);
    std::memcpy(out.data(), &size, 4);
    out[4] = static_cast<uint8_t>(msg.type);
    if (!msg.payload.empty())
        std::memcpy(out.data() + 5, msg.payload.data(), msg.payload.size());
    return out;
}

bool try_decode(const uint8_t* data, size_t size, Message& out, size_t& consumed) {
    consumed = 0;
    if (size < 5) return false;

    uint32_t plen = 0;
    std::memcpy(&plen, data, 4);
    if (plen == 0 || plen > kMaxPayload) { consumed = size; return false; }

    const size_t total = 4 + static_cast<size_t>(plen);
    if (size < total) return false;

    out.type = static_cast<MsgType>(data[4]);
    out.payload.assign(reinterpret_cast<const char*>(data + 5), plen - 1);
    consumed = total;
    return true;
}

// ---------------- PipeServer ----------------

PipeServer::~PipeServer() {
    if (pipe_ != INVALID_HANDLE_VALUE) CloseHandle(pipe_);
    if (ev_ != nullptr) CloseHandle(ev_);
}

bool PipeServer::create(std::wstring_view name, size_t buffer) {
    ev_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (ev_ == nullptr) return false;

    pipe_ = CreateNamedPipeW(
        std::wstring(name).c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES,
        static_cast<DWORD>(buffer), static_cast<DWORD>(buffer),
        0, nullptr);

    return pipe_ != INVALID_HANDLE_VALUE;
}

bool PipeServer::accept(int timeout_ms) {
    if (pipe_ == INVALID_HANDLE_VALUE) return false;

    OVERLAPPED ov{};
    ov.hEvent = ev_;
    ResetEvent(ev_);

    const BOOL ok = ConnectNamedPipe(pipe_, &ov);
    if (!ok) {
        const DWORD err = GetLastError();
        if (err == ERROR_PIPE_CONNECTED) { connected_ = true; return true; }
        if (err != ERROR_IO_PENDING) return false;
        const DWORD w = WaitForSingleObject(ev_, timeout_ms < 0 ? INFINITE : static_cast<DWORD>(timeout_ms));
        if (w != WAIT_OBJECT_0) { CancelIoEx(pipe_, &ov); return false; }
    }
    connected_ = true;
    return true;
}

void PipeServer::disconnect() {
    if (pipe_ == INVALID_HANDLE_VALUE) return;
    FlushFileBuffers(pipe_);
    DisconnectNamedPipe(pipe_);
    connected_ = false;
    rx_.clear();
}

bool PipeServer::read(Message& out, int timeout_ms) {
    for (;;) {
        size_t consumed = 0;
        if (try_decode(rx_.data(), rx_.size(), out, consumed)) {
            drain(rx_, consumed);
            return true;
        }
        if (consumed) drain(rx_, consumed);

        uint8_t buf[kChunk];
        DWORD got = 0;
        if (!overlapped_io(pipe_, ev_, false, buf, sizeof(buf), &got, timeout_ms))
            return false;
        if (got == 0) { connected_ = false; return false; }
        rx_.insert(rx_.end(), buf, buf + got);
    }
}

bool PipeServer::write(const Message& msg, int timeout_ms) {
    auto bytes = encode(msg);
    size_t off = 0;
    while (off < bytes.size()) {
        DWORD sent = 0;
        if (!overlapped_io(pipe_, ev_, true, bytes.data() + off, bytes.size() - off, &sent, timeout_ms))
            return false;
        if (sent == 0) return false;
        off += sent;
    }
    return true;
}

// ---------------- PipeClient ----------------

PipeClient::~PipeClient() { close(); }

bool PipeClient::connect(std::wstring_view name, int timeout_ms) {
    ev_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (ev_ == nullptr) return false;

    const DWORD deadline = GetTickCount() + static_cast<DWORD>(timeout_ms < 0 ? 0 : timeout_ms);
    for (;;) {
        pipe_ = CreateFileW(
            std::wstring(name).c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);

        if (pipe_ != INVALID_HANDLE_VALUE) {
            DWORD mode = PIPE_READMODE_BYTE;
            SetNamedPipeHandleState(pipe_, &mode, nullptr, nullptr);
            return true;
        }
        if (timeout_ms >= 0 && GetTickCount() >= deadline) return false;
        Sleep(50);
    }
}

void PipeClient::close() {
    if (pipe_ != INVALID_HANDLE_VALUE) { CloseHandle(pipe_); pipe_ = INVALID_HANDLE_VALUE; }
    if (ev_ != nullptr) { CloseHandle(ev_); ev_ = nullptr; }
    rx_.clear();
}

bool PipeClient::read(Message& out, int timeout_ms) {
    for (;;) {
        size_t consumed = 0;
        if (try_decode(rx_.data(), rx_.size(), out, consumed)) {
            drain(rx_, consumed);
            return true;
        }
        if (consumed) drain(rx_, consumed);

        uint8_t buf[kChunk];
        DWORD got = 0;
        if (!overlapped_io(pipe_, ev_, false, buf, sizeof(buf), &got, timeout_ms)) return false;
        if (got == 0) return false;
        rx_.insert(rx_.end(), buf, buf + got);
    }
}

bool PipeClient::write(const Message& msg, int timeout_ms) {
    auto bytes = encode(msg);
    size_t off = 0;
    while (off < bytes.size()) {
        DWORD sent = 0;
        if (!overlapped_io(pipe_, ev_, true, bytes.data() + off, bytes.size() - off, &sent, timeout_ms))
            return false;
        if (sent == 0) return false;
        off += sent;
    }
    return true;
}

} // namespace sc::ipc