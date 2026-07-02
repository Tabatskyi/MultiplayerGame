#include "TDMScoreboardWidget.h"
#include "TDMGameState.h"
#include "TDMPlayerState.h"

void UTDMScoreboardWidget::NativeConstruct() {
  Super::NativeConstruct();

  if (ATDMGameState *GS = GetWorld()->GetGameState<ATDMGameState>()) {
    GS->OnTeamScoresChanged.AddUniqueDynamic(this,
                                             &UTDMScoreboardWidget::RefreshAll);
    GS->OnTimerChanged.AddUniqueDynamic(this,
                                        &UTDMScoreboardWidget::RefreshAll);
  }
}

void UTDMScoreboardWidget::RefreshAll() {

  ATDMGameState *GS =
      GetWorld() ? GetWorld()->GetGameState<ATDMGameState>() : nullptr;
  if (!GS) {

    return;
  }

  BP_RefreshTeamScores(GS->TeamScores);

  BP_RefreshTimer(GS->RemainingMatchTime);

  TArray<FTDMPlayerRow> Rows;
  Rows.Reserve(GS->PlayerArray.Num());

  for (APlayerState *PS : GS->PlayerArray) {
    if (const ATDMPlayerState *TDMPS = Cast<ATDMPlayerState>(PS)) {
      FTDMPlayerRow Row;
      Row.PlayerName = TDMPS->GetPlayerName();
      Row.TeamId = TDMPS->TeamId;
      Row.Kills = TDMPS->Kills;
      Row.Deaths = TDMPS->Deaths;
      Rows.Add(MoveTemp(Row));
    }
  }

  BP_RefreshPlayerRows(Rows);
}
