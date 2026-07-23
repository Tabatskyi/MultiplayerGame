# Week 06 — Team Deathmatch Framework + Week 08 — Steam Online Sessions

## Overview

A server-authoritative two-team deathmatch built on Unreal Engine 5's gameplay framework classes. Four clients can play simultaneously, with scores, timer, and per-player stats replicated to every connected client.

Week 08 adds a full internet-multiplayer session layer backed by **Steam Online Subsystem**, enabling host/browse/join flows that work across separate networks via Steam's relay/NAT traversal.

---

## Week 08 — Online Backend

### Which backend and why?

**Steam (OnlineSubsystemSteam)** was chosen because:
- It is free with no additional account or developer portal setup required.
- Steam's relay network provides built-in NAT traversal — no port-forwarding needed on either the host or client machine.
- App ID **480** (Spacewar) is a Valve-provided development sandbox accessible to every Steam account, so any two machines with Steam installed can test together without a published game.

### Authentication method

Steam handles authentication automatically through its ticket system (`AuthTicket`). When a client connects, the Steam backend validates both identities. No manual credential management is required; `IOnlineSubsystem::Get()->GetIdentityInterface()` surfaces the local user's Steam identity. For production you would set your own App ID and enable VAC, but the flow is identical.

### Architecture

| Class | Role |
|---|---|
| `UTDMSessionSubsystem` | `UGameInstanceSubsystem` — lives for the GameInstance lifetime (survives map travels). Owns all `IOnlineSessionPtr` interaction: create, find, join, destroy. Exposes Blueprint-assignable multicast delegates for each outcome. |
| `UTDMMainMenuWidget` | UMG widget C++ base. Thin wrappers that forward Blueprint button presses to `UTDMSessionSubsystem` and fire `BlueprintImplementableEvent` callbacks so the Blueprint child (`WBP_TDMMainMenu`) can update the UI. |
| `WBP_TDMMainMenu` | Blueprint child of `UTDMMainMenuWidget`. Contains the actual UMG layout: Host panel (session name + max players + HOST button), Browser panel (FIND GAMES button + scrollable results list + JOIN button per row), status text. |

### Session settings

```
NumPublicConnections  = user-selected (2–8, default 4)
bIsLANMatch           = false   (internet session)
bShouldAdvertise      = true
bUsesPresence         = true    (visible in Steam friends list)
bAllowJoinInProgress  = true
bUseLobbiesIfAvailable = true
SteamDevAppId         = 480     (Spacewar — development only)
```

### Flow

1. **Host** — presses HOST → `HostSession()` → `IOnlineSessionInterface::CreateSession()` → on success: `World->ServerTravel("/Game/ThirdPerson/Lvl_ThirdPerson?listen")`
2. **Client** — presses FIND GAMES → `FindSessions()` → results populate the list → presses JOIN → `JoinSessionByIndex()` → on success: `GetResolvedConnectString()` → `PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute)`

---

## Known Limitations

- **Steam client must be running** on both machines and both users must be logged into real (non-anonymous) Steam accounts. Anonymous testing is not supported by the Steam OSS.
- **App ID 480 (Spacewar) is development-only.** For a shipped product, replace `SteamDevAppId` in `DefaultEngine.ini` with your actual App ID and remove `bRelaunchInSteam=false`.
- **Session browser polls once** — there is no automatic refresh timer. The user must press "FIND GAMES" again to see newly created sessions. A future improvement would add a recurring timer.
- **No lobby chat / rich presence string** — the session name is not currently surfaced as a Steam rich presence string. This is a cosmetic limitation only.
- **Linux dedicated server** requires the `steamclient.so` library to be present in the binary directory; this is documented in UE's Steam OSS docs and is not included in the repo.
- **VAC disabled** (`bVACEnabled=0`) during development testing with App ID 480. Enable it when publishing with your own App ID.

---

## Week 06 Notes

- Scoreboard widget (`WBP_TDMScoreboard`) updates via replicated `OnRep_TeamScores` / `OnRep_RemainingMatchTime` delegates — previously the widget arrows were non-functional; the C++ delegate-binding approach in `ATDMPlayerController::BeginPlayingState` resolves this.
- Logs are in `Saved/Logs/Practice.log` — search for `[TDMGameMode]`, `[PlayerController]`, `[Session]` tags.
