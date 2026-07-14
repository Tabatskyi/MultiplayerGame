#include "TDMGameState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDMGameState, Log, All);

ATDMGameState::ATDMGameState() {
  UE_LOG(LogTDMGameState, Log, TEXT("[GameState] Constructor"));
}

void ATDMGameState::BeginPlay() {
  Super::BeginPlay();

  if (HasAuthority()) {
    TeamScores.SetNum(2);
    TeamScores[0] = 0;
    TeamScores[1] = 0;
    UE_LOG(LogTDMGameState, Log,
           TEXT("[GameState] BeginPlay (server) — TeamScores initialised [0, 0]"));
  } else {
    UE_LOG(LogTDMGameState, Log,
           TEXT("[GameState] BeginPlay (client) — waiting for replicated state"));
  }
}

void ATDMGameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(ATDMGameState, TeamScores);
  DOREPLIFETIME(ATDMGameState, RemainingMatchTime);
}

void ATDMGameState::OnRep_TeamScores() {
  const int32 S0 = TeamScores.IsValidIndex(0) ? TeamScores[0] : 0;
  const int32 S1 = TeamScores.IsValidIndex(1) ? TeamScores[1] : 0;
  UE_LOG(LogTDMGameState, Log,
         TEXT("[GameState] OnRep_TeamScores — T1=%d  T2=%d"), S0, S1);
  OnTeamScoresChanged.Broadcast();
}

void ATDMGameState::OnRep_RemainingMatchTime() {
  // Only log at integer seconds to avoid spam
  const int32 T = FMath::FloorToInt(RemainingMatchTime);
  if (T % 30 == 0 || T <= 5) {
    UE_LOG(LogTDMGameState, Log,
           TEXT("[GameState] OnRep_RemainingMatchTime — %.0f s"), RemainingMatchTime);
  }
  OnTimerChanged.Broadcast();
}
