#include "TDMGameState.h"
#include "Net/UnrealNetwork.h"

ATDMGameState::ATDMGameState() {}

void ATDMGameState::BeginPlay() {
  Super::BeginPlay();

  if (HasAuthority()) {
    TeamScores.SetNum(2);
    TeamScores[0] = 0;
    TeamScores[1] = 0;
  }
}

void ATDMGameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(ATDMGameState, TeamScores);
  DOREPLIFETIME(ATDMGameState, RemainingMatchTime);
}

void ATDMGameState::OnRep_TeamScores() { OnTeamScoresChanged.Broadcast(); }

void ATDMGameState::OnRep_RemainingMatchTime() { OnTimerChanged.Broadcast(); }
