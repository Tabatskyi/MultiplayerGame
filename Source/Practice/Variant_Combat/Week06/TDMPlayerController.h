#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TDMPlayerController.generated.h"

class UInputMappingContext;
class UTDMScoreboardWidget;

UCLASS(abstract, Config = "Game")
class ATDMPlayerController : public APlayerController {
  GENERATED_BODY()

protected:
  UPROPERTY(EditDefaultsOnly, Category = "Input")
  TArray<UInputMappingContext *> DefaultMappingContexts;

  UPROPERTY(EditDefaultsOnly, Category = "TDM|UI")
  TSubclassOf<UTDMScoreboardWidget> ScoreboardWidgetClass;

  UPROPERTY()
  TObjectPtr<UTDMScoreboardWidget> ScoreboardWidget;

  UPROPERTY(EditDefaultsOnly, Category = "TDM|Respawn")
  TSubclassOf<APawn> RespawnCharacterClass;

  FTransform RespawnTransform;

protected:
  virtual void BeginPlay() override;

  virtual void SetupInputComponent() override;

  virtual void BeginPlayingState() override;

  virtual void OnPossess(APawn *InPawn) override;

private:
  UFUNCTION()
  void OnPawnDestroyed(AActor *DestroyedActor);

  void RespawnPawn();
};
