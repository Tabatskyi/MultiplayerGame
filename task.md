## UNREAL ENGINE TRACK · WEEK 08

## Session-Based Host/Join Over the Internet

Lecture: Online Subsystems, Sessions, and Matchmaking · Points: 9 · Format: Unreal Engine

## Overview

Get out of PIE / local Editor and onto the actual internet. You will integrate an online service backend (Steam or EOS for Unreal, Unity Gaming Services for Unity), build host/browse/join UI, and verify the flow works across two physically separate machines on different networks.

## Learning goals

- Configure an online services backend in your engine

- Implement session/lobby creation, search, and join flows

- Test multiplayer over the public internet with NAT traversal

## Setup

- Continue from Week 6 or start fresh; you need a packaged client build to test from a second machine

- Pick your backend: Steam (free, requires Steam app open during testing) OR EOS (cross-platform, requires an Epic Developer account at dev.epicgames.com)

- Install the corresponding plugin from the Plugins menu; configure DefaultEngine.ini per the docs

## Tasks

- Add and configure the Online Subsystem Steam or Online Subsystem EOS plugin

- Create a session-management class (a UGameInstanceSubsystem works well, or a dedicated UUserWidget). Obtain the session interface via IOnlineSubsystem::Get()->GetSessionInterface() (returns IOnlineSessionPtr)

- To create a session: populate an FOnlineSessionSettings struct (NumPublicConnections, bAllowJoinInProgress, bUsesPresence, bIsLANMatch=false, etc.), then call Sessions->CreateSession(0, SessionName, SessionSettings). Bind OnCreateSessionCompleteDelegate to know when it finishes

- Build a UMG create-session widget that exposes max player count and session name to the user, and calls into your session-management class

- To find sessions: create a TSharedRef<FOnlineSessionSearch>, configure filters (e.g., MaxSearchResults, PresenceSearch), and call Sessions->FindSessions(0, SearchSettings). Bind OnFindSessionsCompleteDelegate; iterate SearchSettings->SearchResults to populate a UMG browser widget with a Join button per row

- On Join, call Sessions->JoinSession(0, SessionName, SearchResult) and bind OnJoinSessionCompleteDelegate. On completion, call Sessions->GetResolvedConnectString(SessionName, ConnectString) and use PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute) to travel to the host's map

- Package a Shipping or Development build of your client (Platforms Windows Package Project)

- Test the full flow: machine A hosts; machine B (different network) finds and joins; both play together


- Record a short demo of the flow working end-to-end

## Hints

- For Steam testing, set bRelaunchInSteam=false and use Steam App ID 480 (Spacewar) during development — the official docs explain this

- EOS requires the Epic Online Services SDK and a configured product on the dev portal — budget time for this setup

- Use the console command "online dump" to see Online Subsystem state when debugging

- For a cleaner starting point, consider the community-maintained Advanced Sessions plugin — it wraps the session interface behind Blueprint-friendly nodes and covers the common host/find/join flow without direct C++ calls

## Common pitfalls

- The default Online Subsystem is Null — a no-op backend with no real networking capability (not LAN-only, it's local-machine-only). CreateSession appears to succeed but no one outside your process can see the session. Switch to Steam or EOS in DefaultEngine.ini

- Mismatched build versions between host and client cause silent join failures — version numbers are checked

- Steam testing requires the Steam client running and logged into a real account — anonymous testing isn't supported

Deliverable: A working host-and-join flow demonstrated in a recorded session with at least one peer on a different network. README must include: which backend you used and why, what authentication method you used, and any known limitations of your implementation.

Submission: Push to /Week08/ in your course GitHub repo. Include the demo recording. DO NOT commit credential files (Steam app keys, EOS product secrets, UGS service account keys) — use .gitignore.


## Grading rubric

| Criterion | Points |
| --- | --- |
| Online services backend correctly configured | 1 |
| Session / lobby host flow works end-to-end | 2 |
| Session / lobby browser correctly lists open games | 2 |
| Join flow connects client to host's match | 2 |
| Successful test with peer on a different network | 1 |
| README quality and security hygiene (no leaked credentials) | 1 |
| Total | 9 |
