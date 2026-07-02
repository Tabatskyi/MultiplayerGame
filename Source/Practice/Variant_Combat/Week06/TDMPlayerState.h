#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TDMPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTDMPlayerStatsChanged);

UCLASS()
class ATDMPlayerState : public APlayerState {
  GENERATED_BODY()

public:
  UPROPERTY(ReplicatedUsing = OnRep_PlayerStats, VisibleAnywhere,
            BlueprintReadOnly, Category = "TDM")
  int32 TeamId = 0;

  UPROPERTY(ReplicatedUsing = OnRep_PlayerStats, VisibleAnywhere,
            BlueprintReadOnly, Category = "TDM")
  int32 Kills = 0;

  UPROPERTY(ReplicatedUsing = OnRep_PlayerStats, VisibleAnywhere,
            BlueprintReadOnly, Category = "TDM")
  int32 Deaths = 0;

  UPROPERTY(BlueprintAssignable, Category = "TDM")
  FOnTDMPlayerStatsChanged OnPlayerStatsChanged;

public:
  virtual void GetLifetimeReplicatedProps(
      TArray<FLifetimeProperty> &OutLifetimeProps) const override;

private:
  UFUNCTION()
  void OnRep_PlayerStats();
};
