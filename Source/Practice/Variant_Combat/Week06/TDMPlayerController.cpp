#include "TDMPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpectatorPawn.h"
#include "InputMappingContext.h"
#include "TDMCharacter.h"
#include "TDMGameState.h"
#include "TDMPlayerState.h"
#include "TDMScoreboardWidget.h"

void ATDMPlayerController::BeginPlay() {
  Super::BeginPlay();

  if (ULocalPlayer *LP = GetLocalPlayer()) {
    if (UEnhancedInputLocalPlayerSubsystem *InputSub =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
                LP)) {
      for (UInputMappingContext *IMC : DefaultMappingContexts) {
        if (IMC) {
          InputSub->AddMappingContext(IMC, 0);
        }
      }
    }
  }
}

void ATDMPlayerController::SetupInputComponent() {
  Super::SetupInputComponent();
}

void ATDMPlayerController::BeginPlayingState() {
  Super::BeginPlayingState();

  if (!IsLocalController()) {
    return;
  }

  if (!ScoreboardWidgetClass) {
    UE_LOG(LogTemp, Warning,
           TEXT("ATDMPlayerController: ScoreboardWidgetClass is not set on %s. "
                "Assign it in the Blueprint child class."),
           *GetName());
    return;
  }

  ScoreboardWidget =
      CreateWidget<UTDMScoreboardWidget>(this, ScoreboardWidgetClass);
  if (!ScoreboardWidget) {
    return;
  }

  ScoreboardWidget->AddToViewport();

  ScoreboardWidget->RefreshAll();

  if (ATDMGameState *GS = GetWorld()->GetGameState<ATDMGameState>()) {
    GS->OnTeamScoresChanged.AddUniqueDynamic(ScoreboardWidget,
                                             &UTDMScoreboardWidget::RefreshAll);
    GS->OnTimerChanged.AddUniqueDynamic(ScoreboardWidget,
                                        &UTDMScoreboardWidget::RefreshAll);
  }

  if (ATDMPlayerState *PS = GetPlayerState<ATDMPlayerState>()) {
    PS->OnPlayerStatsChanged.AddUniqueDynamic(
        ScoreboardWidget, &UTDMScoreboardWidget::RefreshAll);
  }
}

void ATDMPlayerController::OnPossess(APawn *InPawn) {
  Super::OnPossess(InPawn);

  if (InPawn) {
    RespawnTransform = InPawn->GetActorTransform();

    InPawn->OnDestroyed.AddDynamic(this,
                                   &ATDMPlayerController::OnPawnDestroyed);
  }
}

void ATDMPlayerController::OnPawnDestroyed(AActor *DestroyedActor) {

  if (HasAuthority()) {
    RespawnPawn();
  }
}

void ATDMPlayerController::RespawnPawn() {
  if (!RespawnCharacterClass) {
    UE_LOG(LogTemp, Warning,
           TEXT("ATDMPlayerController: RespawnCharacterClass not set — cannot "
                "respawn %s."),
           *GetName());
    return;
  }

  FActorSpawnParameters Params;
  Params.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

  APawn *NewPawn = GetWorld()->SpawnActor<APawn>(RespawnCharacterClass,
                                                 RespawnTransform, Params);

  if (NewPawn) {
    Possess(NewPawn);
  }
}
