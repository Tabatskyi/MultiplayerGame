#include "TDMPlayerState.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDMPlayerState, Log, All);

void ATDMPlayerState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const {
  Super::GetLifetimeReplicatedProps(OutLifetimeProps);

  DOREPLIFETIME(ATDMPlayerState, TeamId);
  DOREPLIFETIME(ATDMPlayerState, Kills);
  DOREPLIFETIME(ATDMPlayerState, Deaths);
}

void ATDMPlayerState::OnRep_PlayerStats() {
  UE_LOG(LogTDMPlayerState, Log,
         TEXT("[PlayerState] OnRep_PlayerStats — '%s'  Team=%d  K=%d  D=%d"),
         *GetPlayerName(), TeamId, Kills, Deaths);
  OnPlayerStatsChanged.Broadcast();
}
