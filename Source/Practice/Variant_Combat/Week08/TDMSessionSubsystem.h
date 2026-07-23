// Week 08 — Steam Online Sessions
// TDMSessionSubsystem: UGameInstanceSubsystem that owns all session logic.
// Lives for the lifetime of the GameInstance (survives map travels).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "TDMSessionSubsystem.generated.h"

// ─── Delegates ────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTDMSessionHostComplete,    bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTDMSessionFindComplete,    bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTDMSessionJoinComplete,    bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTDMSessionDestroyComplete, bool, bWasSuccessful);

// ─── Subsystem ────────────────────────────────────────────────────────────────

UCLASS()
class UTDMSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UTDMSessionSubsystem();

    // ── Public API ─────────────────────────────────────────────────────────

    /**
     * Create and advertise a new Steam session.
     * @param MaxPlayers   Maximum number of human connections (including host).
     * @param SessionName  FName key used internally (default: "TDMSession").
     */
    UFUNCTION(BlueprintCallable, Category = "TDM|Session")
    void HostSession(int32 MaxPlayers = 4, FName SessionName = TEXT("TDMSession"));

    /**
     * Search for available Steam sessions.
     * Results are delivered via OnFindSessionsComplete.
     */
    UFUNCTION(BlueprintCallable, Category = "TDM|Session")
    void FindSessions(int32 MaxResults = 20);

    /**
     * Join a session from the last search result set.
     * @param ResultIndex  Index into the last SearchResults array.
     */
    UFUNCTION(BlueprintCallable, Category = "TDM|Session")
    void JoinSessionByIndex(int32 ResultIndex);

    /**
     * Destroy the currently hosted/joined session (call before hosting again).
     */
    UFUNCTION(BlueprintCallable, Category = "TDM|Session")
    void DestroySession();

    /**
     * Returns the number of results from the last FindSessions call.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TDM|Session")
    int32 GetNumSearchResults() const;

    /**
     * Returns a display string for the given search result index.
     * Format: "OwnerName  |  Ping: Xms  |  Players: Y/Z"
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "TDM|Session")
    FString GetSearchResultDisplayString(int32 ResultIndex) const;

    // ── Blueprint-assignable outcome delegates ─────────────────────────────

    UPROPERTY(BlueprintAssignable, Category = "TDM|Session")
    FOnTDMSessionHostComplete OnHostSessionComplete;

    UPROPERTY(BlueprintAssignable, Category = "TDM|Session")
    FOnTDMSessionFindComplete OnFindSessionsComplete;

    UPROPERTY(BlueprintAssignable, Category = "TDM|Session")
    FOnTDMSessionJoinComplete OnJoinSessionComplete;

    UPROPERTY(BlueprintAssignable, Category = "TDM|Session")
    FOnTDMSessionDestroyComplete OnDestroySessionComplete;

protected:
    // USubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    // ── Cached interface ────────────────────────────────────────────────────
    IOnlineSessionPtr SessionInterface;

    // ── Search state ────────────────────────────────────────────────────────
    TSharedPtr<FOnlineSessionSearch> SessionSearch;

    // ── Delegate handles (for binding cleanup) ──────────────────────────────
    FDelegateHandle OnCreateSessionCompleteDelegateHandle;
    FDelegateHandle OnFindSessionsCompleteDelegateHandle;
    FDelegateHandle OnJoinSessionCompleteDelegateHandle;
    FDelegateHandle OnDestroySessionCompleteDelegateHandle;

    // ── Cached session name (set by HostSession / JoinSession) ─────────────
    FName ActiveSessionName;

    // ── Delegate callbacks ──────────────────────────────────────────────────
    void HandleCreateSessionComplete(FName InSessionName, bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);
    void HandleDestroySessionComplete(FName InSessionName, bool bWasSuccessful);
};
