# Week 06 — Team Deathmatch Framework + Week 08 — Steam Online Sessions

## Overview

A server-authoritative two-team deathmatch built on Unreal Engine 5's gameplay framework classes. Four clients can play simultaneously, with scores, timer, and per-player stats replicated to every connected client.

Week 08 adds a full internet-multiplayer session layer backed by **Steam Online Subsystem**, enabling host/browse/join flows that work across separate networks via Steam's relay/NAT traversal.

---

## Week 06 Notes

- Scoreboard widget (`WBP_TDMScoreboard`) updates via replicated `OnRep_TeamScores` / `OnRep_RemainingMatchTime` delegates — previously the widget arrows were non-functional; the C++ delegate-binding approach in `ATDMPlayerController::BeginPlayingState` resolves this.
- Logs are in `Saved/Logs/Practice.log` — search for `[TDMGameMode]`, `[PlayerController]`, `[Session]` tags.

---

## Week 08 — Online Backend

### Which backend and why?

**Steam (OnlineSubsystemSteam)** was chosen because:

- It is free with no additional account or developer portal setup required.
- Steam's relay network provides built-in NAT traversal — no port-forwarding needed on either the host or client machine.
- App ID **480** (Spacewar) is a Valve-provided development sandbox accessible to every Steam account, so any two machines with Steam installed can test together without a published game.

## Known Limitations

- **Steam client doesn't allow join on the same machine**, so you need two computers with Steam installed to test the multiplayer functionality. But it picks up hosts on different machines just fine.
