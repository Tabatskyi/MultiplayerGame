#include "TDMCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TDMGameMode.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDMCharacter, Log, All);

ATDMCharacter::ATDMCharacter() {
  // APracticeCharacter already sets up the camera boom, follow camera,
  // SetupPlayerInputComponent with IA_Move/Look/Jump, and movement defaults.
  // We auto-load the project's input assets so BP_TDMCharacter works out of
  // the box without any Blueprint Details-panel assignment.
  PrimaryActorTick.bCanEverTick = false;

  bReplicates = true;
  SetReplicatingMovement(true);

  struct FInputAssets {
    ConstructorHelpers::FObjectFinder<UInputAction> Move  {TEXT("/Game/Input/Actions/IA_Move")};
    ConstructorHelpers::FObjectFinder<UInputAction> Look  {TEXT("/Game/Input/Actions/IA_Look")};
    ConstructorHelpers::FObjectFinder<UInputAction> Jump  {TEXT("/Game/Input/Actions/IA_Jump")};
    ConstructorHelpers::FObjectFinder<UInputAction> Mouse {TEXT("/Game/Input/Actions/IA_MouseLook")};
  } IA;

  if (IA.Move.Succeeded())  MoveAction      = IA.Move.Object;
  if (IA.Look.Succeeded())  LookAction      = IA.Look.Object;
  if (IA.Jump.Succeeded())  JumpAction      = IA.Jump.Object;
  if (IA.Mouse.Succeeded()) MouseLookAction = IA.Mouse.Object;

  UE_LOG(LogTDMCharacter, Log,
         TEXT("[Character] Constructor — MaxHP=%.0f  RespawnDelay=%.1f s  "
              "Move=%s Look=%s Jump=%s MouseLook=%s"),
         MaxHP, RespawnDelay,
         MoveAction      ? TEXT("OK") : TEXT("MISSING"),
         LookAction      ? TEXT("OK") : TEXT("MISSING"),
         JumpAction      ? TEXT("OK") : TEXT("MISSING"),
         MouseLookAction ? TEXT("OK") : TEXT("MISSING"));
}

void ATDMCharacter::BeginPlay() {
  Super::BeginPlay();

  CurrentHP = MaxHP;
  UE_LOG(LogTDMCharacter, Log,
         TEXT("[Character] BeginPlay — '%s'  HP=%.0f/%.0f  Authority=%s"),
         *GetName(), CurrentHP, MaxHP,
         HasAuthority() ? TEXT("server") : TEXT("client"));
}

float ATDMCharacter::TakeDamage(float Damage,
                                struct FDamageEvent const& DamageEvent,
                                AController* EventInstigator,
                                AActor* DamageCauser) {
  if (!HasAuthority()) {
    return 0.f;
  }

  if (CurrentHP <= 0.f) {
    return 0.f;
  }

  const float ActualDamage =
      Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

  const float PrevHP = CurrentHP;
  CurrentHP = FMath::Max(0.f, CurrentHP - ActualDamage);

  const FString CauserName = DamageCauser ? DamageCauser->GetName() : TEXT("world");
  UE_LOG(LogTDMCharacter, Log,
         TEXT("[Character] TakeDamage — '%s'  %.0f -> %.0f HP  (dmg=%.1f  causer=%s)"),
         *GetName(), PrevHP, CurrentHP, ActualDamage, *CauserName);

  if (CurrentHP <= 0.f) {
    AController* KillerController = EventInstigator;
    if (!KillerController && DamageCauser) {
      if (const APawn* KillerPawn = Cast<APawn>(DamageCauser)) {
        KillerController = KillerPawn->GetController();
      }
    }

    const FString KillerName = KillerController
        ? KillerController->GetHumanReadableName() : TEXT("<world>");
    UE_LOG(LogTDMCharacter, Log,
           TEXT("[Character] '%s' KILLED by '%s' — calling ScoreKill"),
           *GetName(), *KillerName);

    if (ATDMGameMode* GM = GetWorld()->GetAuthGameMode<ATDMGameMode>()) {
      GM->ScoreKill(GetController(), KillerController);
    } else {
      UE_LOG(LogTDMCharacter, Warning,
             TEXT("[Character] TakeDamage — GameMode not found (client call?)"));
    }

    HandleDeath();
  }

  return ActualDamage;
}

void ATDMCharacter::HandleDeath_Implementation() {
  UE_LOG(LogTDMCharacter, Log,
         TEXT("[Character] HandleDeath — '%s' disabling movement, "
              "respawn in %.1f s"),
         *GetName(), RespawnDelay);

  GetCharacterMovement()->DisableMovement();

  GetWorld()->GetTimerManager().SetTimer(
      RespawnTimerHandle, this, &ATDMCharacter::DoRespawn, RespawnDelay, false);
}

void ATDMCharacter::DoRespawn() {
  UE_LOG(LogTDMCharacter, Log,
         TEXT("[Character] DoRespawn — destroying '%s'"), *GetName());
  Destroy();
}
