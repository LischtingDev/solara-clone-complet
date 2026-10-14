#include "injector/inject.hpp"
#include "injector/process.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct Args {
    std::wstring dll      = L"sc_core.dll";
    std::wstring pipe     = L"sc_core";
    std::wstring target   = L"RobloxPlayerBeta.exe";
    uint32_t     pid      = 0;
    int          delay_ms = 1500;
    int          timeout  = 30000;
    bool         list     = false;
    bool         wait     = false;
    bool         no_init  = false;
};

void usage() {
    std::puts(
        "injector -- usage\n"
        "  --dll <path>       chemin de sc_core.dll (defaut: sc_core.dll)\n"
        "  --pipe <name>      nom du pipe (defaut: sc_core)\n"
        "  --target <exe>     process cible (defaut: RobloxPlayerBeta.exe)\n"
        "  --pid <n>          cibler un PID precis\n"
        "  --delay <ms>       delai apres stabilite avant injection\n"
        "  --timeout <ms>     attente max du process\n"
        "  --wait             attendre que le process apparaisse\n"
        "  --list             lister les process cibles et sortir\n"
        "  --no-init          injecter sans appeler Sc_Init\n");
}

bool parse(int argc, wchar_t** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        const std::wstring k = argv[i];
        auto next = [&](std::wstring& dst) { if (i + 1 < argc) dst = argv[++i]; };

        if (k == L"--help" || k == L"-h") { usage(); return false; }
        else if (k == L"--dll")    next(a.dll);
        else if (k == L"--pipe")   next(a.pipe);
        else if (k == L"--target") next(a.target);
        else if (k == L"--pid")    a.pid = static_cast<uint32_t>(_wtoi(argv[++i]));
        else if (k == L"--delay")  a.delay_ms = _wtoi(argv[++i]);
        else if (k == L"--timeout")a.timeout = _wtoi(argv[++i]);
        else if (k == L"--list")   a.list = true;
        else if (k == L"--wait")   a.wait = true;
        else if (k == L"--no-init")a.no_init = true;
        else { std::wprintf(L"option inconnue: %s\n", k.c_str()); usage(); return false; }
    }
    return true;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    using namespace sc::injector;

    Args a;
    if (!parse(argc, argv, a)) return 2;

    enable_debug_privilege();

    if (a.list) {
        for (const auto& p : find_processes(a.target)) {
            std::wprintf(L"%u\t%s\n", p.pid, p.path.c_str());
        }
        return 0;
    }

    if (a.pid != 0) {
        std::wprintf(L"cible: pid %u\n", a.pid);
    } else {
        std::wprintf(L"recherche de %s...\n", a.target.c_str());
        auto p = a.wait
            ? wait_for_process(a.target, a.timeout, 3000)
            : std::optional<ProcessInfo>{};
        if (!p) {
            auto all = find_processes(a.target);
            if (all.empty()) { std::puts("process introuvable"); return 1; }
            p = all.front();
        }
        a.pid = p->pid;
        std::wprintf(L"cible: pid %u (%s)\n", a.pid, p->path.c_str());
    }

    for (int i = 0; i < 60 && !is_window_ready(a.pid); ++i)
        Sleep(250);

    if (a.delay_ms > 0) {
        std::printf("attente de %d ms (stabilite Hyperion)...\n", a.delay_ms);
        Sleep(static_cast<DWORD>(a.delay_ms));
    }

    const auto r = inject_dll(a.pid, a.dll);
    if (!r.ok) {
        std::printf("injection echouee: %s (err %lu)\n", r.detail.c_str(), r.last_error);
        return 1;
    }
    std::printf("DLL injectee @ 0x%llx\n", static_cast<unsigned long long>(r.module_base));

    if (!a.no_init) {
        const bool ok = call_remote_export(a.pid, a.dll, "Sc_Init", a.pipe);
        std::printf("Sc_Init: %s\n", ok ? "ok" : "echec");
        if (!ok) return 1;
    }

    std::puts("termine.");
    return 0;
}