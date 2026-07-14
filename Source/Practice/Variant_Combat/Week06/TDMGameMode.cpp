#include "TDMGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "TDMGameState.h"
#include "TDMPlayerController.h"
#include "TDMPlayerState.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDMGameMode, Log, All);

ATDMGameMode::ATDMGameMode() {
  GameStateClass       = ATDMGameState::StaticClass();
  PlayerStateClass     = ATDMPlayerState::StaticClass();
  PlayerControllerClass = ATDMPlayerController::StaticClass();

  UE_LOG(LogTDMGameMode, Log,
         TEXT("[GameMode] Constructor — GameStateClass=%s, PlayerStateClass=%s, "
              "PlayerControllerClass=%s"),
         *GameStateClass->GetName(),
         *PlayerStateClass->GetName(),
         *PlayerControllerClass->GetName());
}

void ATDMGameMode::BeginPlay() {
  Super::BeginPlay();
  UE_LOG(LogTDMGameMode, Log,
         TEXT("[GameMode] BeginPlay — MatchDuration=%.0f s"), MatchDuration);
}

void ATDMGameMode::HandleMatchHasStarted() {
  Super::HandleMatchHasStarted();

  UE_LOG(LogTDMGameMode, Log,
         TEXT("[GameMode] HandleMatchHasStarted — match is now InProgress"));

  if (ATDMGameState* GS = GetGameState<ATDMGameState>()) {
    GS->RemainingMatchTime = MatchDuration;
    UE_LOG(LogTDMGameMode, Log,
           TEXT("[GameMode] Set RemainingMatchTime = %.0f"), GS->RemainingMatchTime);
  }

  GetWorldTimerManager().SetTimer(MatchTimerHandle, this,
                                  &ATDMGameMode::MatchTimerTick, 1.f, true);
  UE_LOG(LogTDMGameMode, Log, TEXT("[GameMode] 1-second match timer started"));
}

void ATDMGameMode::PostLogin(APlayerController* NewPlayer) {
  Super::PostLogin(NewPlayer);

  if (ATDMPlayerState* PS = NewPlayer->GetPlayerState<ATDMPlayerState>()) {
    PS->TeamId = GetSmallestTeamIndex();
    UE_LOG(LogTDMGameMode, Log,
           TEXT("[GameMode] PostLogin — player '%s' assigned to Team %d"),
           *PS->GetPlayerName(), PS->TeamId);
  } else {
    UE_LOG(LogTDMGameMode, Warning,
           TEXT("[GameMode] PostLogin — could not get TDMPlayerState for new player"));
  }
}

void ATDMGameMode::ScoreKill(AController* VictimController,
                             AController* KillerController) {
  if (!HasAuthority()) {
    return;
  }

  const FString VictimName = VictimController
      ? VictimController->GetHumanReadableName() : TEXT("<none>");
  const FString KillerName = KillerController
      ? KillerController->GetHumanReadableName() : TEXT("<none>");

  UE_LOG(LogTDMGameMode, Log,
         TEXT("[GameMode] ScoreKill — Killer=%s  Victim=%s"),
         *KillerName, *VictimName);

  if (ATDMPlayerState* VictimPS =
          VictimController ? VictimController->GetPlayerState<ATDMPlayerState>()
                           : nullptr) {
    VictimPS->Deaths++;
    UE_LOG(LogTDMGameMode, Log,
           TEXT("[GameMode]   %s Deaths -> %d"), *VictimName, VictimPS->Deaths);
  }

  if (KillerController && KillerController != VictimController) {
    if (ATDMPlayerState* KillerPS =
            KillerController->GetPlayerState<ATDMPlayerState>()) {
      KillerPS->Kills++;
      UE_LOG(LogTDMGameMode, Log,
             TEXT("[GameMode]   %s Kills -> %d"), *KillerName, KillerPS->Kills);

      if (ATDMGameState* GS = GetGameState<ATDMGameState>()) {
        const int32 TeamIdx = FMath::Clamp(KillerPS->TeamId, 0, 1);
        if (GS->TeamScores.IsValidIndex(TeamIdx)) {
          GS->TeamScores[TeamIdx]++;
          UE_LOG(LogTDMGameMode, Log,
                 TEXT("[GameMode]   Team %d score -> %d"),
                 TeamIdx, GS->TeamScores[TeamIdx]);
        }
      }
    }
  }
}

void ATDMGameMode::MatchTimerTick() {
  ATDMGameState* GS = GetGameState<ATDMGameState>();
  if (!GS) {
    return;
  }

  GS->RemainingMatchTime = FMath::Max(0.f, GS->RemainingMatchTime - 1.f);

  // Log every 30 seconds to avoid flooding
  const int32 T = FMath::FloorToInt(GS->RemainingMatchTime);
  if (T % 30 == 0 || T <= 5) {
    UE_LOG(LogTDMGameMode, Log,
           TEXT("[GameMode] MatchTimerTick — %.0f s remaining"),
           GS->RemainingMatchTime);
  }

  if (GS->RemainingMatchTime <= 0.f) {
    GetWorldTimerManager().ClearTimer(MatchTimerHandle);

    // Determine winner
    const int32 S0 = GS->TeamScores.IsValidIndex(0) ? GS->TeamScores[0] : 0;
    const int32 S1 = GS->TeamScores.IsValidIndex(1) ? GS->TeamScores[1] : 0;
    const FString Winner = (S0 > S1) ? TEXT("Team 1")
                         : (S1 > S0) ? TEXT("Team 2")
                         :             TEXT("Draw");
    UE_LOG(LogTDMGameMode, Log,
           TEXT("[GameMode] Match OVER — %s wins! (T1=%d T2=%d)"),
           *Winner, S0, S1);

    EndMatch();
  }
}

int32 ATDMGameMode::GetSmallestTeamIndex() const {
  int32 TeamCounts[2] = {0, 0};

  if (const AGameStateBase* GS = GetWorld()->GetGameState()) {
    for (const APlayerState* PS : GS->PlayerArray) {
      if (const ATDMPlayerState* TDMPS = Cast<ATDMPlayerState>(PS)) {
        const int32 TeamIdx = FMath::Clamp(TDMPS->TeamId, 0, 1);
        TeamCounts[TeamIdx]++;
      }
    }
  }

  const int32 Chosen = (TeamCounts[1] < TeamCounts[0]) ? 1 : 0;
  UE_LOG(LogTDMGameMode, Verbose,
         TEXT("[GameMode] GetSmallestTeamIndex — T0=%d T1=%d -> Team %d"),
         TeamCounts[0], TeamCounts[1], Chosen);
  return Chosen;
}
