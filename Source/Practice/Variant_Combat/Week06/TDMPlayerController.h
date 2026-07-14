#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TDMPlayerController.generated.h"

class UInputMappingContext;
class UTDMScoreboardWidget;

UCLASS(abstract, Config = "Game")
class ATDMPlayerController : public APlayerController {
  GENERATED_BODY()

public:
  ATDMPlayerController();

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

  // ── Debug helpers (testing only) ────────────────────────────────────────

  /** F1 → simulate a kill scored by this player (for scoreboard testing). */
  void DebugSimulateKill();

  /** Server RPC that actually calls GameMode->ScoreKill. */
  UFUNCTION(Server, Reliable)
  void Server_DebugKill();
};
