// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LagCompensatedWeapon.generated.h"

class USkeletalMeshComponent;
class ULagCompensationComponent;

/**
 *  ALagCompensatedWeapon
 *
 *  A hitscan ranged weapon that uses server-side lag compensation to confirm hits.
 *
 *  Flow:
 *    1. Client calls FireWeapon() locally (plays effects, sends RPC).
 *    2. RPC ServerFireWeapon_Implementation() runs on the server:
 *         a. _Validate() rejects impossible payloads early.
 *         b. Iterates all pawns, calls ULagCompensationComponent::ConfirmHit().
 *         c. Applies damage to confirmed targets.
 *
 *  Attach to a character's weapon socket and assign it via CombatCharacter::EquippedWeapon.
 */
UCLASS(Blueprintable)
class ALagCompensatedWeapon : public AActor
{
	GENERATED_BODY()

public:

	ALagCompensatedWeapon();

	// ------------------------------------------------------------------
	// Configuration
	// ------------------------------------------------------------------

	/** Amount of damage a single shot deals on a torso hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|Damage",
	          meta=(ClampMin=0, ClampMax=200))
	float BaseDamage = 25.0f;

	/** Additional damage multiplier applied when the head hitbox is struck */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|Damage",
	          meta=(ClampMin=1, ClampMax=10))
	float HeadShotMultiplier = 2.0f;

	/** Name of the muzzle socket on the weapon mesh — used as trace origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|Mesh")
	FName MuzzleSocketName = FName("MuzzleFlash");

	// ------------------------------------------------------------------
	// Client-side fire entry point
	// ------------------------------------------------------------------

	/**
	 *  Called on the owning client when the player presses the fire button.
	 *  Captures the current muzzle location and camera aim direction, stamps with
	 *  the server-synchronized clock, plays cosmetic effects, and sends the
	 *  ServerFireWeapon RPC.
	 */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void FireWeapon();

protected:

	// ------------------------------------------------------------------
	// Server RPC
	// ------------------------------------------------------------------

	/**
	 *  Server-side hit confirmation.
	 *
	 *  @param StartLocation   World-space muzzle position at the moment of fire
	 *  @param AimDirection    Normalised camera forward vector at the moment of fire
	 *  @param ClientTimestamp AGameStateBase::GetServerWorldTimeSeconds() captured on client
	 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFireWeapon(FVector StartLocation, FVector AimDirection, float ClientTimestamp);

	/** Validation — reject impossible payloads before the implementation runs */
	bool ServerFireWeapon_Validate(FVector StartLocation, FVector AimDirection, float ClientTimestamp);

	/** Implementation — iterates targets, rewinds hitboxes, applies damage */
	void ServerFireWeapon_Implementation(FVector StartLocation, FVector AimDirection, float ClientTimestamp);

	// ------------------------------------------------------------------
	// Blueprint hooks — implement cosmetic effects in BP subclass
	// ------------------------------------------------------------------

	/** Called on all clients to play muzzle flash, sound, etc. */
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFireEffects(FVector MuzzleLocation, FVector AimDir);
	void MulticastFireEffects_Implementation(FVector MuzzleLocation, FVector AimDir);

	/** BP hook for muzzle flash and audio */
	UFUNCTION(BlueprintImplementableEvent, Category="Weapon|Effects")
	void BP_PlayFireEffects(FVector MuzzleLocation, FVector AimDir);

	/** BP hook called on a confirmed hit — good place for impact sparks/decals */
	UFUNCTION(BlueprintImplementableEvent, Category="Weapon|Effects")
	void BP_PlayImpactEffects(bool bHeadShot, AActor* HitActor);

protected:

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

	/** Weapon skeletal mesh (optional — purely cosmetic) */
	UPROPERTY(VisibleAnywhere, Category="Components")
	USkeletalMeshComponent* WeaponMesh;
};
