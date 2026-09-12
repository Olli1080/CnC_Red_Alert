---
name: code-networking-multiplayer
description: CODE/ lockstep event system, connection abstraction, and the three coexisting network transport generations
metadata:
  type: project_state
---

# CODE/ — Networking & Multiplayer

Parent index: [summary.md](summary.md). Background on why three transport generations coexist here: [../../../knowledge_base/codebase_history_and_cruft.md](../../../knowledge_base/codebase_history_and_cruft.md).

## Event system / determinism core
- `EVENT.CPP`/`.H` — `EventClass`: encapsulates every player-issued command (move, attack, place building, sell, options, game-speed change, etc.) as a serializable event, so lockstep simulation stays identical across networked machines.
- `QUEUE.CPP`/`.H` — template `QueueClass<T,size>`: generic ring-buffer queue used to buffer these events per-frame.
- `IOOBJ.CPP` / `IOMAP.CPP` — object/map I/O helpers tied into the event/save pipeline.
- `RANDOM.CPP`/`.H`, `RAND.CPP`, `RNG.H` (see [platform_audio_misc.md](platform_audio_misc.md)) — deterministic PRNG, critical for keeping lockstep simulation identical across machines.

## Connection abstraction
- `CONNECT.CPP`/`.H` (+ stale `.CPP.BAK`) — `ConnectionClass`: pure-virtual base representing one network "connection", wrapping a `CommBufferClass` of queued packets.
- `CONNMGR.H` — `ConnectionManagerClass`: abstract base pooling/distributing incoming packets across multiple `ConnectionClass` instances — the only intended access point to network/modem I/O.
- `COMBUF.CPP`/`.H`, `COMQUEUE.CPP`/`.H`, `COMINIT.CPP`/`.H` — communication buffer/init plumbing shared by connection managers.

## Transport generation 1: IPX LAN (1996, original)
- `IPX.CPP`/`.H`, `IPX95.CPP`/`.H`, `IPXADDR.CPP`/`.H`, `IPXCONN.CPP`/`.H`, `IPXGCONN.CPP`/`.H`, `IPXMGR.CPP`/`.H`, `IPXPROT.ASM`, `IPXREAL.ASM` — legacy Novell IPX LAN transport (direct + "global channel" broadcast connection variants), with real-mode ASM helpers for DOS IPX access.
- **This is the game-side protocol code.** It calls into the separate top-level [IPX/](../ipx_launch_launcher_tools.md) directory's 16-bit thunk DLLs (`THIPX16.DLL`/`THIPX32.DLL`) — don't confuse the two; `CODE/IPX*.CPP` is the protocol/connection logic, `IPX/` builds the DLLs that bridge 32-bit calls down to the real-mode DOS IPX interrupt.
- `SEQCONN.CPP`/`.H`, `NOSEQCON.CPP`/`.H` — sequenced vs. non-sequenced packet-delivery connection variants layered over IPX/UDP.
- `NULLCONN.CPP`/`.H`, `NULLMGR.CPP`/`.H`, `NULLDLG.CPP` — "null modem"/serial cable direct-connect play mode.

## Transport generation 2: TEN (superseded, dead)
- `TENMGR.CPP`/`.H` — "Total Entertainment Network" online-service connection manager, an older online-play backend predating WOLAPI. Effectively dead code, not removed.

## Transport generation 3: TCP/IP + WOLAPI (newest, 1998)
- `TCPIP.CPP`/`.H`, `WSPROTO.CPP`/`.H` (+ `_WSPROTO.CPP`/`.H`), `WSPIPX.CPP`/`.H`, `WSPUDP.CPP`/`.H`, `UDPADDR.CPP` — Winsock-based TCP/IP and UDP transports (`WSP*` = "WinSock Protocol" wrapper layer), the newer path superseding raw IPX for Win32/Internet play.
- `PACKET.CPP`/`.H` — `PacketClass`: generic linked-list-of-fields packet builder/parser used for the online registration/matchmaking protocol.
- `DDE.CPP`/`.H`, `CCDDE.CPP`/`.H` — Windows DDE (Dynamic Data Exchange) IPC, used to launch/coordinate with the Windows 95 chat/launcher front-end.
- `INTERNET.CPP`/`.H`, `INTERPAL.CPP` — Internet-play connection setup and interpolation/ping handling.
- `TRACKER.CPP`, `UTRACKER.CPP`/`.H` — game-session tracking/reporting (stats/heartbeat to a tracker service).
- **WOLAPI glue** (root-level `CODE/`, distinct from `CODE/WOLAPI/`'s generated COM headers — see [build_entrypoint_eng_wolapi.md](build_entrypoint_eng_wolapi.md)): `WOLAPIOB.CPP`/`.H` (`WolapiObject`, the main game-side container, active only when `WOLAPI_INTEGRATION` is defined — added 1998, layered on top of the original netcode), `RAWOLAPI.CPP`/`.H` (Red-Alert-specific bridge/adapter), `WOLSTRNG.CPP`/`.H`, `WOLDEBUG.H`, `WOLEDIT.CPP`/`.H` (string lookup, debug logging, text-edit glue), `WOL_CGAM.CPP`, `WOL_CHAT.CPP`, `WOL_DNLD.CPP`, `WOL_GSUP.CPP`/`.H`, `WOL_LOGN.CPP`, `WOL_MAIN.CPP`, `WOL_OPT.CPP` (WOL UI screen flow: create-game, chat lobby, patch/download, game-setup support, login, main menu, options).

## Multiplayer session / setup UI
- `SESSION.CPP`/`.H` — `SessionClass`: top-level multiplayer game-session state (player list, game options) — the central hub tying connections, houses, and UI together for a multiplayer match.
- `MPLAYER.CPP`, `MP.CPP`/`.H`, `MPU.CPP`/`.H`, `MPGSET.CPP`/`.H`, `MPMGRD.CPP`/`.H` (DOS modem manager), `MPMGRW.CPP`/`.H` (Win32 modem manager), `MPLIB.CPP`, `MPLPC.CPP` — multiplayer game setup screens/support (modem, LAN, internet lobby variants).
