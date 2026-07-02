#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TDMGameMode.generated.h"

class ATDMGameState;
class ATDMPlayerState;

UCLASS()
class ATDMGameMode : public AGameMode {
  GENERATED_BODY()

public:
  ATDMGameMode();

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TDM")
  float MatchDuration = 300.f;

  void ScoreKill(AController *VictimController, AController *KillerController);

protected:
  virtual void BeginPlay() override;

  virtual void PostLogin(APlayerController *NewPlayer) override;

  virtual void HandleMatchHasStarted() override;

private:
  FTimerHandle MatchTimerHandle;

  void MatchTimerTick();

  int32 GetSmallestTeamIndex() const;
};
