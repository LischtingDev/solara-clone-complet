# Tests

## Automatises (hors Roblox)

    ctest --test-dir build -C Release --output-on-failure

Couvre :
- le framing IPC (encodage/decodage, buffers partiels, tailles invalides)
- le matcher de signatures sur `tests/fixtures/fake_module.bin`
- le parseur d'offsets.json

## Manuel (necessite Roblox installe)

### J1 - injection

1. Lancer Roblox, rejoindre un jeu.
2. `build/out/injector.exe --wait --dll build/out/sc_core.dll`
3. Verifier `sc_core.log` a cote du process et l'absence de crash.
4. Repeter 10 fois.

### J2 - capture du VM

1. Injecter.
2. Verifier dans le log : `lua_State captured`.
3. Depuis l'UI : executer `print("ok")`, verifier l'affichage console.

### J4 - API core

Executer dans l'UI :

    local p = Instance.new("Part")
    p.Name = "ScTest"
    p.Parent = workspace
    print(p:GetFullName())

### J6 - couverture

Executer un test de conformite UNC, reporter le taux dans `docs/API.md`.

## Diagnostic

- Rien dans le log : l'injection a echoue. Verifier le PID et l'antivirus.
- `offsets incomplets` : signature cassee, relancer `sigscan_probe`.
- Crash au hook : Roblox a patche. Verifier `luau_load` en priorite.