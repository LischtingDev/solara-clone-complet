# Couverture API (cible UNC)

Objectif : maximiser la compatibilite avec les scripts publics. Reference :
la spec Unified Naming Convention.

## Niveau 1 - base (J4)

- [ ] `game`, `workspace`, `script`
- [ ] `Instance.new`, `Instance:Destroy`, `Instance:FindFirstChild`
- [ ] acces/ecriture de proprietes par indexation
- [ ] `task.wait`, `task.spawn`, `task.delay`, `task.defer`
- [ ] `wait`, `spawn`, `delay`, `tick`
- [ ] `getgenv`, `getrenv`, `getreg`
- [ ] `print`, `warn`, `error` captures

## Niveau 2 - reflection (J6)

- [ ] `getrawmetatable`, `setrawmetatable`, `setreadonly`, `isreadonly`
- [ ] `hookfunction`, `hookmetamethod`, `restorefunction`
- [ ] `getnamecallmethod`, `newcclosure`
- [ ] `getgc`, `getinstances`, `getnilinstances`
- [ ] `checkcaller`, `islclosure`

## Niveau 3 - signaux et events (J6)

- [ ] `firesignal`, `getconnections`, `fireclickdetector`
- [ ] `getcustomasset`, `getcallingscript`

## Niveau 4 - avance (J6+)

- [ ] `require` sur les modules du jeu
- [ ] `setthreadidentity` / `getthreadidentity` (niveau 8)
- [ ] `cloneref`, `compareinstances`
- [ ] `setclipboard`, `writefile`, `readfile`, `isfile`, `listfiles`
- [ ] `identifyexecutor`

## Mesure

Executer un test de conformite UNC dans l'UI et noter le taux de reussite dans
cette page, section "Resultats", avec la date.

## Resultats

| Date | Version Roblox | Taux UNC | Notes |
|---|---|---|---|
| - | - | - | - |