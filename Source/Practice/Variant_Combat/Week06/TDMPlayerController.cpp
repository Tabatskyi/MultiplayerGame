#include "TDMPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpectatorPawn.h"
#include "InputMappingContext.h"
#include "TDMCharacter.h"
#include "TDMGameMode.h"
#include "TDMGameState.h"
#include "TDMPlayerState.h"
#include "TDMScoreboardWidget.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDMPlayerController, Log, All);

ATDMPlayerController::ATDMPlayerController() {
  // Auto-load the project's mapping contexts so DefaultMappingContexts is
  // populated without any Blueprint Details-panel assignment.
  struct FIMCAssets {
    ConstructorHelpers::FObjectFinder<UInputMappingContext> Default   {TEXT("/Game/Input/IMC_Default")};
    ConstructorHelpers::FObjectFinder<UInputMappingContext> MouseLook {TEXT("/Game/Input/IMC_MouseLook")};
  } IMC;

  if (IMC.Default.Succeeded())   DefaultMappingContexts.Add(IMC.Default.Object);
  if (IMC.MouseLook.Succeeded()) DefaultMappingContexts.Add(IMC.MouseLook.Object);

  UE_LOG(LogTDMPlayerController, Log,
         TEXT("[PlayerController] Constructor — %d mapping context(s) auto-loaded"),
         DefaultMappingContexts.Num());
}


void ATDMPlayerController::BeginPlay() {
  Super::BeginPlay();

  if (ULocalPlayer* LP = GetLocalPlayer()) {
    if (UEnhancedInputLocalPlayerSubsystem* InputSub =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP)) {
      for (UInputMappingContext* IMC : DefaultMappingContexts) {
        if (IMC) {
          InputSub->AddMappingContext(IMC, 0);
        }
      }
    }
  }

  UE_LOG(LogTDMPlayerController, Log,
         TEXT("[PlayerController] BeginPlay — IsLocalController=%s  HasAuthority=%s"),
         IsLocalController() ? TEXT("yes") : TEXT("no"),
         HasAuthority() ? TEXT("yes") : TEXT("no"));
}

void ATDMPlayerController::SetupInputComponent() {
  Super::SetupInputComponent();

  // F1 → debug-simulate a kill for quick scoreboard testing in PIE.
  // Works in both legacy and Enhanced Input mode because we bind directly
  // to the PlayerController (not the character).
  if (InputComponent) {
    InputComponent->BindKey(EKeys::F1, IE_Pressed, this,
                            &ATDMPlayerController::DebugSimulateKill);
    UE_LOG(LogTDMPlayerController, Log,
           TEXT("[PlayerController] F1 bound to DebugSimulateKill"));
  }
}

void ATDMPlayerController::BeginPlayingState() {
  Super::BeginPlayingState();

  UE_LOG(LogTDMPlayerController, Log,
         TEXT("[PlayerController] BeginPlayingState — IsLocalController=%s"),
         IsLocalController() ? TEXT("yes") : TEXT("no"));

  if (!IsLocalController()) {
    return;
  }

  if (!ScoreboardWidgetClass) {
    UE_LOG(LogTDMPlayerController, Warning,
           TEXT("[PlayerController] ScoreboardWidgetClass is not set on '%s'. "
                "Assign WBP_TDMScoreboard in the Blueprint child class."),
           *GetName());
    return;
  }

  ScoreboardWidget =
      CreateWidget<UTDMScoreboardWidget>(this, ScoreboardWidgetClass);
  if (!ScoreboardWidget) {
    UE_LOG(LogTDMPlayerController, Error,
           TEXT("[PlayerController] CreateWidget failed for ScoreboardWidgetClass"));
    return;
  }

  ScoreboardWidget->AddToViewport();
  UE_LOG(LogTDMPlayerController, Log,
         TEXT("[PlayerController] Scoreboard widget created and added to viewport"));

  // Initial refresh
  ScoreboardWidget->RefreshAll();

  // Bind to GameState delegates
  if (ATDMGameState* GS = GetWorld()->GetGameState<ATDMGameState>()) {
    GS->OnTeamScoresChanged.AddUniqueDynamic(ScoreboardWidget,
                                             &UTDMScoreboardWidget::RefreshAll);
    GS->OnTimerChanged.AddUniqueDynamic(ScoreboardWidget,
                                        &UTDMScoreboardWidget::RefreshAll);
    UE_LOG(LogTDMPlayerController, Log,
           TEXT("[PlayerController] Bound to GameState delegates"));
  } else {
    UE_LOG(LogTDMPlayerController, Warning,
           TEXT("[PlayerController] GameState not available yet during BeginPlayingState "
                "— scoreboard won't auto-refresh on GS events"));
  }

  // Bind to local PlayerState delegate
  if (ATDMPlayerState* PS = GetPlayerState<ATDMPlayerState>()) {
    PS->OnPlayerStatsChanged.AddUniqueDynamic(
        ScoreboardWidget, &UTDMScoreboardWidget::RefreshAll);
    UE_LOG(LogTDMPlayerController, Log,
           TEXT("[PlayerController] Bound to PlayerState delegate — "
                "player '%s' Team=%d"),
           *PS->GetPlayerName(), PS->TeamId);
  } else {
    UE_LOG(LogTDMPlayerController, Warning,
           TEXT("[PlayerController] PlayerState not available during BeginPlayingState"));
  }
}

void ATDMPlayerController::OnPossess(APawn* InPawn) {
  Super::OnPossess(InPawn);

  if (InPawn) {
    RespawnTransform = InPawn->GetActorTransform();
    InPawn->OnDestroyed.AddDynamic(this,
                                   &ATDMPlayerController::OnPawnDestroyed);
    UE_LOG(LogTDMPlayerController, Log,
           TEXT("[PlayerController] OnPossess — possessed '%s' at %s"),
           *InPawn->GetName(),
           *RespawnTransform.GetLocation().ToString());
  }
}

void ATDMPlayerController::OnPawnDestroyed(AActor* DestroyedActor) {
  UE_LOG(LogTDMPlayerController, Log,
         TEXT("[PlayerController] OnPawnDestroyed — pawn '%s' destroyed, "
              "scheduling respawn"),
         *DestroyedActor->GetName());

  if (HasAuthority()) {
    RespawnPawn();
  }
}

void ATDMPlayerController::RespawnPawn() {
  if (!RespawnCharacterClass) {
    UE_LOG(LogTDMPlayerController, Warning,
           TEXT("[PlayerController] RespawnCharacterClass not set — cannot respawn"));
    return;
  }

  FActorSpawnParameters Params;
  Params.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

  APawn* NewPawn = GetWorld()->SpawnActor<APawn>(RespawnCharacterClass,
                                                 RespawnTransform, Params);
  if (NewPawn) {
    Possess(NewPawn);
    UE_LOG(LogTDMPlayerController, Log,
           TEXT("[PlayerController] Respawned '%s' at %s"),
           *NewPawn->GetName(),
           *RespawnTransform.GetLocation().ToString());
  } else {
    UE_LOG(LogTDMPlayerController, Error,
           TEXT("[PlayerController] SpawnActor failed — respawn pawn is null"));
  }
}

// ── Debug helpers ──────────────────────────────────────────────────────────────────────

void ATDMPlayerController::DebugSimulateKill() {
  UE_LOG(LogTDMPlayerController, Log,
         TEXT("[PlayerController] F1 pressed — sending debug kill to server"));
  Server_DebugKill();
}

void ATDMPlayerController::Server_DebugKill_Implementation() {
  // Simulate this player scoring a kill against themselves (self-kill),
  // which still increments their team score and Kills counter —
  // enough to verify the scoreboard refreshes on all clients.
  if (ATDMGameMode* GM = GetWorld()->GetAuthGameMode<ATDMGameMode>()) {
    UE_LOG(LogTDMPlayerController, Log,
           TEXT("[PlayerController] Server_DebugKill — simulating kill for '%s'"),
           *GetHumanReadableName());
    GM->ScoreKill(nullptr, this);  // victim=nullptr avoids Deaths increment
  } else {
    UE_LOG(LogTDMPlayerController, Warning,
           TEXT("[PlayerController] Server_DebugKill — GameMode not found"));
  }
}
