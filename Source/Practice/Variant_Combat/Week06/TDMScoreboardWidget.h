#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TDMScoreboardWidget.generated.h"

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

UCLASS(abstract)
class UTDMScoreboardWidget : public UUserWidget {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, Category = "TDM")
  void RefreshAll();

protected:
  virtual void NativeConstruct() override;

  UFUNCTION(BlueprintImplementableEvent, Category = "TDM")
  void BP_RefreshTeamScores(const TArray<int32> &Scores);

  UFUNCTION(BlueprintImplementableEvent, Category = "TDM")
  void BP_RefreshTimer(float RemainingSeconds);

  UFUNCTION(BlueprintImplementableEvent, Category = "TDM")
  void BP_RefreshPlayerRows(const TArray<FTDMPlayerRow> &Rows);
};
