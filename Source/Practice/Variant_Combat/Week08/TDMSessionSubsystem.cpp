// Week 08 — Steam Online Sessions
// TDMSessionSubsystem implementation.

#include "TDMSessionSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDMSession, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Map to travel to after a session is created (host) or joined (client).
// This is the Level that contains the TDM GameMode.
// ─────────────────────────────────────────────────────────────────────────────
static const TCHAR* TDM_GAME_MAP = TEXT("/Game/ThirdPerson/Lvl_ThirdPerson");

// ─────────────────────────────────────────────────────────────────────────────
UTDMSessionSubsystem::UTDMSessionSubsystem()
    : ActiveSessionName(TEXT("TDMSession"))
{
}

// ─────────────────────────────────────────────────────────────────────────────
void UTDMSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    IOnlineSubsystem* OSS = IOnlineSubsystem::Get();
    if (!OSS)
    {
        UE_LOG(LogTDMSession, Error, TEXT("[Session] No OnlineSubsystem found — Steam running?"));
        return;
    }

    UE_LOG(LogTDMSession, Log, TEXT("[Session] Using OSS: %s"), *OSS->GetSubsystemName().ToString());
    SessionInterface = OSS->GetSessionInterface();

    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTDMSession, Error, TEXT("[Session] SessionInterface is invalid"));
        return;
    }

    // Bind all delegates once; remove on Deinitialize.
    OnCreateSessionCompleteDelegateHandle = SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(
        this, &UTDMSessionSubsystem::HandleCreateSessionComplete);

    OnFindSessionsCompleteDelegateHandle = SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(
        this, &UTDMSessionSubsystem::HandleFindSessionsComplete);

    OnJoinSessionCompleteDelegateHandle = SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(
        this, &UTDMSessionSubsystem::HandleJoinSessionComplete);

    OnDestroySessionCompleteDelegateHandle = SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(
        this, &UTDMSessionSubsystem::HandleDestroySessionComplete);

    UE_LOG(LogTDMSession, Log, TEXT("[Session] Subsystem initialized, all delegates bound"));
}

// ─────────────────────────────────────────────────────────────────────────────
void UTDMSessionSubsystem::Deinitialize()
{
    if (SessionInterface.IsValid())
    {
        SessionInterface->OnCreateSessionCompleteDelegates.Remove(OnCreateSessionCompleteDelegateHandle);
        SessionInterface->OnFindSessionsCompleteDelegates.Remove(OnFindSessionsCompleteDelegateHandle);
        SessionInterface->OnJoinSessionCompleteDelegates.Remove(OnJoinSessionCompleteDelegateHandle);
        SessionInterface->OnDestroySessionCompleteDelegates.Remove(OnDestroySessionCompleteDelegateHandle);
    }
    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────────────────────
void UTDMSessionSubsystem::HostSession(int32 MaxPlayers, FName SessionName)
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTDMSession, Error, TEXT("[Session] HostSession — SessionInterface invalid"));
        OnHostSessionComplete.Broadcast(false);
        return;
    }

    ActiveSessionName = SessionName;

    // Destroy any existing session first (graceful re-host).
    const FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(ActiveSessionName);
    if (ExistingSession)
    {
        UE_LOG(LogTDMSession, Log, TEXT("[Session] HostSession — destroying existing session before re-host"));
        SessionInterface->DestroySession(ActiveSessionName);
        // HandleDestroySessionComplete will NOT re-trigger HostSession here;
        // the user must press Host again. This is intentional to keep state simple.
        return;
    }

    FOnlineSessionSettings Settings;
    Settings.NumPublicConnections  = FMath::Clamp(MaxPlayers, 2, 8);
    Settings.NumPrivateConnections = 0;
    Settings.bAllowJoinInProgress  = true;
    Settings.bIsLANMatch           = false;      // internet session via Steam relay
    Settings.bShouldAdvertise      = true;
    Settings.bAllowJoinViaPresence = true;
    Settings.bUsesPresence         = true;       // enables Steam friend-list visibility
    Settings.bUseLobbiesIfAvailable = true;
    Settings.Set(FName(TEXT("keywords")), FString(TEXT("TDM")),
                 EOnlineDataAdvertisementType::ViaOnlineService);

    UE_LOG(LogTDMSession, Log,
           TEXT("[Session] HostSession — Name='%s' MaxPlayers=%d"),
           *ActiveSessionName.ToString(), Settings.NumPublicConnections);

    if (!SessionInterface->CreateSession(0, ActiveSessionName, Settings))
    {
        UE_LOG(LogTDMSession, Error, TEXT("[Session] HostSession — CreateSession call failed immediately"));
        OnHostSessionComplete.Broadcast(false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void UTDMSessionSubsystem::FindSessions(int32 MaxResults)
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTDMSession, Error, TEXT("[Session] FindSessions — SessionInterface invalid"));
        OnFindSessionsComplete.Broadcast(false);
        return;
    }

    SessionSearch = MakeShared<FOnlineSessionSearch>();
    SessionSearch->MaxSearchResults = FMath::Clamp(MaxResults, 1, 100);
    SessionSearch->bIsLanQuery      = false;
    SessionSearch->QuerySettings.Set(FName(TEXT("presence")), true, EOnlineComparisonOp::Equals);

    UE_LOG(LogTDMSession, Log,
           TEXT("[Session] FindSessions — MaxResults=%d"), SessionSearch->MaxSearchResults);

    if (!SessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
    {
        UE_LOG(LogTDMSession, Error, TEXT("[Session] FindSessions — call failed immediately"));
        OnFindSessionsComplete.Broadcast(false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void UTDMSessionSubsystem::JoinSessionByIndex(int32 ResultIndex)
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTDMSession, Error, TEXT("[Session] JoinSession — SessionInterface invalid"));
        OnJoinSessionComplete.Broadcast(false);
        return;
    }

    if (!SessionSearch.IsValid()
        || !SessionSearch->SearchResults.IsValidIndex(ResultIndex))
    {
        UE_LOG(LogTDMSession, Error,
               TEXT("[Session] JoinSession — invalid ResultIndex %d (have %d results)"),
               ResultIndex,
               SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0);
        OnJoinSessionComplete.Broadcast(false);
        return;
    }

    const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[ResultIndex];
    UE_LOG(LogTDMSession, Log,
           TEXT("[Session] JoinSession — joining result %d, owner='%s', ping=%dms"),
           ResultIndex,
           *Result.Session.OwningUserName,
           Result.PingInMs);

    if (!SessionInterface->JoinSession(0, ActiveSessionName, Result))
    {
        UE_LOG(LogTDMSession, Error, TEXT("[Session] JoinSession — call failed immediately"));
        OnJoinSessionComplete.Broadcast(false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void UTDMSessionSubsystem::DestroySession()
{
    if (!SessionInterface.IsValid())
    {
        UE_LOG(LogTDMSession, Warning, TEXT("[Session] DestroySession — SessionInterface invalid"));
        OnDestroySessionComplete.Broadcast(false);
        return;
    }

    UE_LOG(LogTDMSession, Log,
           TEXT("[Session] DestroySession — destroying '%s'"), *ActiveSessionName.ToString());

    if (!SessionInterface->DestroySession(ActiveSessionName))
    {
        UE_LOG(LogTDMSession, Error, TEXT("[Session] DestroySession — call failed immediately"));
        OnDestroySessionComplete.Broadcast(false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
int32 UTDMSessionSubsystem::GetNumSearchResults() const
{
    if (!SessionSearch.IsValid())
    {
        return 0;
    }
    return SessionSearch->SearchResults.Num();
}

// ─────────────────────────────────────────────────────────────────────────────
FString UTDMSessionSubsystem::GetSearchResultDisplayString(int32 ResultIndex) const
{
    if (!SessionSearch.IsValid()
        || !SessionSearch->SearchResults.IsValidIndex(ResultIndex))
    {
        return TEXT("(invalid)");
    }

    const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[ResultIndex];
    const int32 MaxConn     = Result.Session.SessionSettings.NumPublicConnections;
    const int32 OpenConn    = Result.Session.NumOpenPublicConnections;
    const int32 CurrentConn = MaxConn - OpenConn;

    return FString::Printf(TEXT("%s  |  Ping: %dms  |  Players: %d/%d"),
        *Result.Session.OwningUserName,
        Result.PingInMs,
        CurrentConn,
        MaxConn);
}

// ─────────────────────────────────────────────────────────────────────────────
// ── Private callbacks ─────────────────────────────────────────────────────────

void UTDMSessionSubsystem::HandleCreateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
    UE_LOG(LogTDMSession, Log,
           TEXT("[Session] HandleCreateSessionComplete — '%s' success=%s"),
           *InSessionName.ToString(), bWasSuccessful ? TEXT("yes") : TEXT("no"));

    OnHostSessionComplete.Broadcast(bWasSuccessful);

    if (bWasSuccessful)
    {
        // Server-travel to the TDM map as a listen server.
        if (UWorld* World = GetWorld())
        {
            const FString TravelURL = FString::Printf(TEXT("%s?listen"), TDM_GAME_MAP);
            UE_LOG(LogTDMSession, Log, TEXT("[Session] ServerTravel → %s"), *TravelURL);
            World->ServerTravel(TravelURL);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void UTDMSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
    const int32 NumResults = (SessionSearch.IsValid() ? SessionSearch->SearchResults.Num() : 0);
    UE_LOG(LogTDMSession, Log,
           TEXT("[Session] HandleFindSessionsComplete — success=%s, results=%d"),
           bWasSuccessful ? TEXT("yes") : TEXT("no"), NumResults);

    OnFindSessionsComplete.Broadcast(bWasSuccessful);
}

// ─────────────────────────────────────────────────────────────────────────────
void UTDMSessionSubsystem::HandleJoinSessionComplete(
    FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
    const bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
    UE_LOG(LogTDMSession, Log,
           TEXT("[Session] HandleJoinSessionComplete — '%s' result=%d success=%s"),
           *InSessionName.ToString(), (int32)Result, bSuccess ? TEXT("yes") : TEXT("no"));

    OnJoinSessionComplete.Broadcast(bSuccess);

    if (bSuccess && SessionInterface.IsValid())
    {
        // Resolve the Steam connection string and client-travel to the host.
        FString ConnectString;
        if (SessionInterface->GetResolvedConnectString(InSessionName, ConnectString))
        {
            UE_LOG(LogTDMSession, Log,
                   TEXT("[Session] ClientTravel → %s"), *ConnectString);

            if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
            {
                PC->ClientTravel(ConnectString, TRAVEL_Absolute);
            }
        }
        else
        {
            UE_LOG(LogTDMSession, Error,
                   TEXT("[Session] GetResolvedConnectString failed — cannot travel to host"));
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void UTDMSessionSubsystem::HandleDestroySessionComplete(FName InSessionName, bool bWasSuccessful)
{
    UE_LOG(LogTDMSession, Log,
           TEXT("[Session] HandleDestroySessionComplete — '%s' success=%s"),
           *InSessionName.ToString(), bWasSuccessful ? TEXT("yes") : TEXT("no"));

    OnDestroySessionComplete.Broadcast(bWasSuccessful);
}
