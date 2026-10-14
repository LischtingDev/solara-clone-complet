# Protocole IPC

Transport : named pipe Windows, `PIPE_TYPE_BYTE`, duplex, mode overlapped.

Nom par defaut : `\\.\pipe\sc_core`
En multi-instance : `\\.\pipe\sc_core_<pid>` (un pipe par process Roblox).

## Framing

    +----------------+--------+---------------------------+
    | uint32 taille  | uint8  | payload (JSON, UTF-8)     |
    | little-endian  | type   | taille - 1 octets         |
    +----------------+--------+---------------------------+

`taille` inclut l'octet de type. Taille max : 64 Mo.

## Messages

| Code | Nom | Sens | Payload |
|---|---|---|---|
| 0x01 | Ping | UI -> core | `{}` |
| 0x02 | Pong | core -> UI | `{}` |
| 0x03 | AttachState | core -> UI | `{attached,pid,place_id,version}` |
| 0x04 | Execute | UI -> core | `{source,name}` |
| 0x05 | ExecuteDone | core -> UI | `{ok,error}` |
| 0x06 | Output | core -> UI | `{level,text}` |
| 0x07 | ClearOutput | UI -> core | `{}` |
| 0x08 | GetInstances | UI -> core | `{}` |
| 0x09 | Instances | core -> UI | arbre JSON |
| 0x0A | OffsetStatus | core -> UI | `{scanned,missing[]}` |
| 0x0B | Shutdown | UI -> core | `{}` |

`level` vaut `info`, `warn`, `error` ou `lua`.

## Regles

- `kProtocolVersion` dans `ipc/protocol.hpp` est incremente a chaque changement
  incompatible. L'UI refuse de dialoguer avec une version differente.
- Le core ne bloque jamais sur une ecriture : si le pipe est plein, on log et on jette.
- Toute nouvelle fonctionnalite UI/core qui a besoin de communiquer passe par un
  nouveau code de message, jamais par un detournement de payload.