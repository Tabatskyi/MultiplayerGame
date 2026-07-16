// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "HealthComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogHealthComponent, Log, All);

/**
 *  Week 04 — Replicated Health & Damage System
 *
 *  UHealthComponent is a replication-aware actor component that owns the
 *  authoritative health state for any actor that attaches it.
 *
 *  Key networking primitives demonstrated:
 *    - Replicated UPROPERTY with RepNotify (CurrentHealth)
 *    - Replicated UPROPERTY without RepNotify (MaxHealth)
 *    - Server RPC with WithValidation (ServerApplyDamage)
 *    - NetMulticast unreliable RPC for cosmetic effects (MulticastDamageFlash)
 */
UCLASS(ClassGroup=(Combat), meta=(BlueprintSpawnableComponent))
class UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UHealthComponent();

	// -----------------------------------------------------------------------
	//  Replicated Properties
	// -----------------------------------------------------------------------

	/** Maximum health the owning actor can have */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category="Health",
	          meta=(ClampMin=1, ClampMax=1000))
	float MaxHealth = 100.0f;

	/**
	 *  Current health value.
	 *  ReplicatedUsing means OnRep_CurrentHealth() fires on every client
	 *  that receives a new value.
	 */
	UPROPERTY(ReplicatedUsing=OnRep_CurrentHealth, VisibleAnywhere,
	          BlueprintReadOnly, Category="Health")
	float CurrentHealth = 0.0f;

	// -----------------------------------------------------------------------
	//  Server RPC — apply damage (validated)
	// -----------------------------------------------------------------------

	/**
	 *  Called by owning client to ask the server to subtract Amount from
	 *  CurrentHealth.  WithValidation means the engine calls
	 *  ServerApplyDamage_Validate() before _Implementation().
	 */
	UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category="Health")
	void ServerApplyDamage(float Amount);

	// -----------------------------------------------------------------------
	//  NetMulticast RPC — cosmetic flash
	// -----------------------------------------------------------------------

	/**
	 *  Fires on server + all clients when damage is successfully applied.
	 *  Unreliable because occasional drops are acceptable for a visual effect.
	 */
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category="Health")
	void MulticastDamageFlash();

	// -----------------------------------------------------------------------
	//  RepNotify
	// -----------------------------------------------------------------------

	/** Called on clients (and optionally server) whenever CurrentHealth changes */
	UFUNCTION()
	void OnRep_CurrentHealth();

	// -----------------------------------------------------------------------
	//  Helpers
	// -----------------------------------------------------------------------

	/** Resets CurrentHealth to MaxHealth on the server */
	UFUNCTION(BlueprintCallable, Category="Health")
	void ResetHealth();

	/** True if the owning actor is considered dead */
	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const { return CurrentHealth <= 0.0f; }

protected:

	/** Registers replicated properties with the engine */
	virtual void GetLifetimeReplicatedProps(
	    TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Sets replication on and seeds initial health */
	virtual void BeginPlay() override;
};
