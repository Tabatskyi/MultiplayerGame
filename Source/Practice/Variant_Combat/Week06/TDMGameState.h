#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TDMGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTDMTeamScoresChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTDMTimerChanged);

UCLASS()
class ATDMGameState : public AGameState {
  GENERATED_BODY()

public:
  ATDMGameState();

  UPROPERTY(ReplicatedUsing = OnRep_TeamScores, VisibleAnywhere,
            BlueprintReadOnly, Category = "TDM")
  TArray<int32> TeamScores;

  UPROPERTY(ReplicatedUsing = OnRep_RemainingMatchTime, VisibleAnywhere,
            BlueprintReadOnly, Category = "TDM")
  float RemainingMatchTime = 300.f;

  UPROPERTY(BlueprintAssignable, Category = "TDM")
  FOnTDMTeamScoresChanged OnTeamScoresChanged;

  UPROPERTY(BlueprintAssignable, Category = "TDM")
  FOnTDMTimerChanged OnTimerChanged;

  virtual void GetLifetimeReplicatedProps(
      TArray<FLifetimeProperty> &OutLifetimeProps) const override;

  virtual void BeginPlay() override;

private:
  UFUNCTION()
  void OnRep_TeamScores();

  UFUNCTION()
  void OnRep_RemainingMatchTime();
};
