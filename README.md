# solara-clone

Clone fonctionnel de Solara : injecteur + DLL core + UI, pour executer des scripts
Luau dans Roblox sous Windows x64.

## Composants

- `injector/` : trouve Roblox, attend la stabilite, injecte `sc_core.dll`
- `core/` : DLL injectee (hook VM Luau, offsets, API, executeur)
- `ipc/` : protocole et pipe nomme partages
- `ui/` : application WPF (editeur, console, script hub)
- `tools/` : dump d'offsets, probe de signatures

## Build

    cmake -S . -B build -G "Visual Studio 17 2022" -A x64
    cmake --build build --config Release
    dotnet build ui/SolaraClone.Ui -c Release

## Tests

    ctest --test-dir build -C Release --output-on-failure

## Etat d'avancement

Voir `CLAUDE.md`, section "Etat".

## Reference

Projet inspire du fonctionnement de Solara. Aucune copie de code binaire.
Voir `CLAUDE.md` pour les references publiques utilisees.