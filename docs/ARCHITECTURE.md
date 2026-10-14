# Architecture

    +---------------------+          +----------------------------------+
    |  UI (WPF, externe)  |          |   RobloxPlayerBeta.exe           |
    |                     |          |                                  |
    |  editeur / onglets  |          |  +----------------------------+  |
    |  console            |  pipe    |  |  sc_core.dll (injectee)    |  |
    |  script hub         |<-------->|  |                            |  |
    |  auto-exec          |          |  |  scanner / offsets         |  |
    +---------------------+          |  |  hook Luau                 |  |
             ^                       |  |  env builder               |  |
             | CreateRemoteThread    |  |  executeur                 |  |
             | + LoadLibraryW        |  |  dispatcher pipe           |  |
    +---------------------+          |  +----------------------------+  |
    |  injector.exe       |--------->|                                  |
    +---------------------+          |   Luau VM / DataModel            |
                                     +----------------------------------+

## Flux

1. L'UI demarre, cree le pipe serveur `\\.\pipe\sc_core`.
2. L'UI lance `injector.exe --pid <roblox> --dll sc_core.dll --pipe sc_core`.
3. L'injecteur attend que Roblox soit stable (fenetre principale + uptime mini).
4. L'injecteur injecte la DLL puis appelle son export `Sc_Init(pipe_name)` a distance.
5. La DLL, sur son thread de bootstrap :
   - initialise le logger
   - resout les offsets (offsets.json puis scan de signatures)
   - installe le hook Luau, capture un `lua_State`
   - construit l'environnement de script (env::build)
   - lance le dispatcher (boucle de lecture du pipe)
6. L'UI envoie `Execute{source}` ; la DLL execute dans le thread Luau et renvoie
   `Output{level,text}` puis `ExecuteDone{ok,error}`.

## Frontiere

Le seul etat partage entre UI et core est le protocole IPC (`ipc/protocol.hpp`).
Aucun autre couplage. Toute evolution du protocole se fait dans le meme commit
cote C++ et cote C#.

## Composants et responsabilites

| Composant | Responsabilite | Ne fait pas |
|---|---|---|
| `injector` | trouver, attendre, injecter, ejecter | connaitre Luau |
| `core/scanner` | lire la memoire, matcher des signatures | interpreter les offsets |
| `core/offsets` | table de signatures -> adresses nommees | hooker |
| `core/luau_hook` | hook VM, capture lua_State | executer du code utilisateur |
| `core/env` | exposer l'API Roblox | gerer le pipe |
| `core/executor` | executer une source, capturer erreurs | parser le protocole |
| `core/dispatcher` | lire/ecrire le pipe, router | connaitre Luau en detail |
| `ui` | afficher, envoyer | toucher a la memoire |

## Cycle de vie et robustesse

- Tout travail lourd se fait hors de `DllMain`.
- L'execution utilisateur est wrappee : `__try/__except` (SEH) + `lua_pcall`.
- Perte du pipe : le serveur se re-armes automatiquement (boucle accept).
- Fermeture de Roblox : le dispatcher sort proprement, l'UI repasse en "detached".