// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UHealthComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PRACTICE3_API UUHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UUHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Replicated properties
	UPROPERTY(Replicated)
	float MaxHealth = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth = 100.0f;

	// Called when the game starts
	virtual void BeginPlay() override;

	// RepNotify callback for CurrentHealth changes
	UFUNCTION()
	void OnRep_CurrentHealth();

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Server RPC to apply damage with validation
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerApplyDamage(float Amount);

	// Multicast RPC for damage flash effect
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastDamageFlash();

	// Getters
	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

		
};
