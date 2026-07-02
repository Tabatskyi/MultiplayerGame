#include "TDMCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TDMGameMode.h"
#include "TimerManager.h"

ATDMCharacter::ATDMCharacter() {
  PrimaryActorTick.bCanEverTick = false;

  bReplicates = true;
  SetReplicatingMovement(true);
}

void ATDMCharacter::BeginPlay() {
  Super::BeginPlay();

  CurrentHP = MaxHP;
}

float ATDMCharacter::TakeDamage(float Damage,
                                struct FDamageEvent const &DamageEvent,
                                AController *EventInstigator,
                                AActor *DamageCauser) {

  if (!HasAuthority()) {
    return 0.f;
  }

  if (CurrentHP <= 0.f) {
    return 0.f;
  }

  const float ActualDamage =
      Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

  CurrentHP = FMath::Max(0.f, CurrentHP - ActualDamage);

  if (CurrentHP <= 0.f) {

    AController *KillerController = EventInstigator;
    if (!KillerController && DamageCauser) {
      if (const APawn *KillerPawn = Cast<APawn>(DamageCauser)) {
        KillerController = KillerPawn->GetController();
      }
    }

    if (ATDMGameMode *GM = GetWorld()->GetAuthGameMode<ATDMGameMode>()) {
      GM->ScoreKill(GetController(), KillerController);
    }

    HandleDeath();
  }

  return ActualDamage;
}

void ATDMCharacter::HandleDeath_Implementation() {

  GetCharacterMovement()->DisableMovement();

  GetWorld()->GetTimerManager().SetTimer(
      RespawnTimerHandle, this, &ATDMCharacter::DoRespawn, RespawnDelay, false);
}

void ATDMCharacter::DoRespawn() { Destroy(); }
