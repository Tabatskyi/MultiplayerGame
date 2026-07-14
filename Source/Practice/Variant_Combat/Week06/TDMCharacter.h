#pragma once

#include "CoreMinimal.h"
#include "PracticeCharacter.h"
#include "TDMCharacter.generated.h"

UCLASS(abstract)
class ATDMCharacter : public APracticeCharacter {
  GENERATED_BODY()

public:
  ATDMCharacter();

  virtual float TakeDamage(float Damage, struct FDamageEvent const &DamageEvent,
                           AController *EventInstigator,
                           AActor *DamageCauser) override;

  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TDM|Health")
  float CurrentHP = 100.f;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TDM|Health")
  float MaxHP = 100.f;

protected:
  virtual void BeginPlay() override;

  UFUNCTION(BlueprintNativeEvent, Category = "TDM|Health")
  void HandleDeath();
  virtual void HandleDeath_Implementation();

  UPROPERTY(EditDefaultsOnly, Category = "TDM|Health")
  float RespawnDelay = 3.f;

private:
  FTimerHandle RespawnTimerHandle;

  void DoRespawn();
};
