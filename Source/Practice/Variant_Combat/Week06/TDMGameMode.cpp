#include "TDMGameMode.h"
#include "GameFramework/GameStateBase.h"
#include "TDMGameState.h"
#include "TDMPlayerController.h"
#include "TDMPlayerState.h"
#include "TimerManager.h"

ATDMGameMode::ATDMGameMode() {

  GameStateClass = ATDMGameState::StaticClass();
  PlayerStateClass = ATDMPlayerState::StaticClass();
  PlayerControllerClass = ATDMPlayerController::StaticClass();
}

void ATDMGameMode::BeginPlay() { Super::BeginPlay(); }

void ATDMGameMode::HandleMatchHasStarted() {
  Super::HandleMatchHasStarted();

  if (ATDMGameState *GS = GetGameState<ATDMGameState>()) {
    GS->RemainingMatchTime = MatchDuration;
  }

  GetWorldTimerManager().SetTimer(MatchTimerHandle, this,
                                  &ATDMGameMode::MatchTimerTick, 1.f, true);
}

void ATDMGameMode::PostLogin(APlayerController *NewPlayer) {

  Super::PostLogin(NewPlayer);

  if (ATDMPlayerState *PS = NewPlayer->GetPlayerState<ATDMPlayerState>()) {
    PS->TeamId = GetSmallestTeamIndex();
  }
}

void ATDMGameMode::ScoreKill(AController *VictimController,
                             AController *KillerController) {

  if (!HasAuthority()) {
    return;
  }

  if (ATDMPlayerState *VictimPS =
          VictimController ? VictimController->GetPlayerState<ATDMPlayerState>()
                           : nullptr) {
    VictimPS->Deaths++;
  }

  if (KillerController && KillerController != VictimController) {
    if (ATDMPlayerState *KillerPS =
            KillerController->GetPlayerState<ATDMPlayerState>()) {
      KillerPS->Kills++;

      if (ATDMGameState *GS = GetGameState<ATDMGameState>()) {
        const int32 TeamIdx = FMath::Clamp(KillerPS->TeamId, 0, 1);
        if (GS->TeamScores.IsValidIndex(TeamIdx)) {
          GS->TeamScores[TeamIdx]++;
        }
      }
    }
  }
}

void ATDMGameMode::MatchTimerTick() {
  ATDMGameState *GS = GetGameState<ATDMGameState>();
  if (!GS) {
    return;
  }

  GS->RemainingMatchTime = FMath::Max(0.f, GS->RemainingMatchTime - 1.f);

  if (GS->RemainingMatchTime <= 0.f) {

    GetWorldTimerManager().ClearTimer(MatchTimerHandle);

    EndMatch();
  }
}

int32 ATDMGameMode::GetSmallestTeamIndex() const {
  int32 TeamCounts[2] = {0, 0};

  if (const AGameStateBase *GS = GetWorld()->GetGameState()) {
    for (const APlayerState *PS : GS->PlayerArray) {
      if (const ATDMPlayerState *TDMPS = Cast<ATDMPlayerState>(PS)) {
        const int32 TeamIdx = FMath::Clamp(TDMPS->TeamId, 0, 1);
        TeamCounts[TeamIdx]++;
      }
    }
  }

  return (TeamCounts[1] < TeamCounts[0]) ? 1 : 0;
}
