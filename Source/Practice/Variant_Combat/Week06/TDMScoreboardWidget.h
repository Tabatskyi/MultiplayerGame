#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TDMScoreboardWidget.generated.h"

class UTextBlock;
class UVerticalBox;
class UBorder;

USTRUCT(BlueprintType)
struct FTDMPlayerRow {
  GENERATED_BODY()

  UPROPERTY(BlueprintReadOnly, Category = "TDM")
  FString PlayerName;

  UPROPERTY(BlueprintReadOnly, Category = "TDM")
  int32 TeamId = 0;

  UPROPERTY(BlueprintReadOnly, Category = "TDM")
  int32 Kills = 0;

  UPROPERTY(BlueprintReadOnly, Category = "TDM")
  int32 Deaths = 0;
};

/**
 * Fully C++ driven scoreboard widget.
 * NativeConstruct() builds the entire layout.
 * RefreshAll() updates it from replicated GameState / PlayerArray.
 *
 * NOT abstract — the WBP_TDMScoreboard blueprint inherits this and needs no
 * graph implementation; it just acts as a vehicle for the asset reference.
 */
UCLASS()
class UTDMScoreboardWidget : public UUserWidget {
  GENERATED_BODY()

public:
  /** Called by delegates bound in BeginPlayingState. Never tick-driven. */
  UFUNCTION(BlueprintCallable, Category = "TDM")
  void RefreshAll();

protected:
  virtual void NativeConstruct() override;

  // ── Live-updated text blocks ──────────────────────────────────────────────
  UPROPERTY()
  TObjectPtr<UTextBlock> Text_Team0Score;

  UPROPERTY()
  TObjectPtr<UTextBlock> Text_Team1Score;

  UPROPERTY()
  TObjectPtr<UTextBlock> Text_Timer;

  UPROPERTY()
  TObjectPtr<UVerticalBox> Box_PlayerRows;

private:
  /** Build the widget subtree. Called once from NativeConstruct. */
  void BuildLayout();

  /** Create one row: "PlayerName | Team X | K: n | D: n". */
  void AddPlayerRow(const FTDMPlayerRow& Row);
};
