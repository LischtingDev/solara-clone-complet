# Offsets et signatures

Regle dure : AUCUNE adresse codee en dur. Toute adresse vient d'une signature
scannee dans `core/offsets.cpp`, ou d'un override dans `core/offsets.json`.

## Pourquoi

Roblox n'exporte quasiment rien. Tout passe par pattern scanning sur le binaire.
Roblox patche chaque semaine : sans mecanisme de mise a jour, le projet meurt
en quelques jours. C'est le critere de viabilite numero 1.

## Format d'une signature

    { "Nom::Fonction", "48 8B 05 ?? ?? ?? ?? 48 85 C0", offset, indirection, required }

- pattern IDA-style, `??` ou `?` = wildcard d'un octet
- `offset` : delta applique a l'adresse trouvee
- `indirection` : nombre de dereferencements rip-relatifs a suivre
- `required` : si false, l'absence degrade sans bloquer

## Signatures a identifier (a remplir au J2)

| Nom | Role | Pattern | Statut |
|---|---|---|---|
| `Luau::luau_load` | charge un chunk, sert de point de hook pour capturer le lua_State | ? | A TROUVER |
| `Luau::luaD_throw` | fallback de capture du lua_State | ? | A TROUVER |
| `ScriptContext::GetGlobalState` | acces direct au thread principal Luau | ? | A TROUVER |
| `Lua::lua_pcall` | appel protege | ? | A TROUVER |
| `Lua::lua_setglobal` | ecriture de globals | ? | A TROUVER |
| `Lua::lua_getglobal` | lecture de globals | ? | A TROUVER |
| `Lua::lua_pushcclosurek` | exposition de fonctions C | ? | A TROUVER |
| `Lua::lua_createtable` | tables | ? | A TROUVER |
| `DataModel::GetScriptContext` | acces au ScriptContext | ? | A TROUVER |
| `Identity::setthreadidentity` | execution niveau 8 | ? | A TROUVER (J6) |

## Methode de travail

1. `sigscan_probe.exe RobloxPlayerBeta.exe "pattern"` sur le binaire sur disque
   pour valider qu'une signature est unique.
2. `dump_offsets.exe --pid <pid> --out core/offsets.json` sur un process live.
3. Verifier qu'une signature retourne bien UNE seule adresse. Sinon, allonger.
4. Documenter ici, avec la version de Roblox au moment de la trouvaille.

## Auto-update (J7)

- `core/offsets.json` peut etre ecrase par un JSON distant (meme schema).
- Si une signature `required` echoue au scan : log Error + message `OffsetStatus`
  vers l'UI, qui affiche un bandeau "offsets obsoletes".

## Pieges

- Les patterns doivent eviter les adresses relatives (elles changent).
- Un pattern trop court matche plusieurs fois : preferer 8+ octets, et verifier.
- Ne jamais hooker avant que le VM Luau soit initialise.