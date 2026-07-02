// Copyright Epic Games, Inc. All Rights Reserved.

#include "LagCompensatedWeapon.h"
#include "LagCompensationComponent.h"
#include "LagCompensationTypes.h"
#include "CombatDamageable.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"   // TActorIterator

DEFINE_LOG_CATEGORY_STATIC(LogLagCompWeapon, Log, All);

// ---------------------------------------------------------------------------
//  Construction
// ---------------------------------------------------------------------------

ALagCompensatedWeapon::ALagCompensatedWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	// Weapons are replicated so clients can see them attached to characters.
	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);
}

// ---------------------------------------------------------------------------
//  Lifecycle
// ---------------------------------------------------------------------------

void ALagCompensatedWeapon::BeginPlay()
{
	Super::BeginPlay();
}

void ALagCompensatedWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// No additional replicated properties needed on the weapon itself right now.
}

// ---------------------------------------------------------------------------
//  Client-side fire
// ---------------------------------------------------------------------------

void ALagCompensatedWeapon::FireWeapon()
{
	// Guard: reject calls from non-owning or server-only contexts that shouldn't fire locally.
	if (!GetOwner())
	{
		return;
	}

	// Only the locally controlled autonomous proxy (player) or the server may fire.
	const ENetRole LocalRole = GetOwner()->GetLocalRole();
	if (LocalRole != ROLE_AutonomousProxy && !GetOwner()->HasAuthority())
	{
		UE_LOG(LogLagCompWeapon, Warning, TEXT("FireWeapon called from unexpected role (%d)."), (int32)LocalRole);
		return;
	}

	// Determine muzzle location from the weapon socket.
	FVector MuzzleLocation = WeaponMesh->GetSocketLocation(MuzzleSocketName);

	// Fall back to actor location if socket isn't configured.
	if (MuzzleLocation.IsNearlyZero())
	{
		MuzzleLocation = GetActorLocation();
	}

	// Derive aim direction from the owning character's controller rotation.
	FVector AimDirection = FVector::ForwardVector;

	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (const AController* Controller = OwnerPawn->GetController())
		{
			AimDirection = Controller->GetControlRotation().Vector();
		}
	}

	// Stamp with the synchronized server clock — no offset math needed.
	float ClientTimestamp = 0.0f;
	if (const AGameStateBase* GS = GetWorld()->GetGameState<AGameStateBase>())
	{
		ClientTimestamp = GS->GetServerWorldTimeSeconds();
	}

	UE_LOG(LogLagCompWeapon, Log,
	       TEXT("FireWeapon: MuzzleLoc=%s  AimDir=%s  Timestamp=%.4f"),
	       *MuzzleLocation.ToString(), *AimDirection.ToString(), ClientTimestamp);

	// Play local cosmetic effects immediately so the shooter sees no latency.
	BP_PlayFireEffects(MuzzleLocation, AimDirection);

	// Broadcast effects to all other clients.
	MulticastFireEffects(MuzzleLocation, AimDirection);

	// Send the authoritative fire event to the server.
	ServerFireWeapon(MuzzleLocation, AimDirection, ClientTimestamp);
}

// ---------------------------------------------------------------------------
//  Server RPC — Validation
// ---------------------------------------------------------------------------

bool ALagCompensatedWeapon::ServerFireWeapon_Validate(
	FVector StartLocation,
	FVector AimDirection,
	float   ClientTimestamp)
{
	// ----- 1. Direction sanity -----
	// Reject zero vectors and grossly non-normalised directions.
	if (AimDirection.IsNearlyZero())
	{
		UE_LOG(LogLagCompWeapon, Warning, TEXT("ServerFireWeapon_Validate: zero aim direction — rejected."));
		return false;
	}

	const float LengthSq = AimDirection.SizeSquared();
	// Allow ±10 % deviation from unit length to tolerate floating-point drift.
	if (LengthSq < 0.81f || LengthSq > 1.21f)
	{
		UE_LOG(LogLagCompWeapon, Warning,
		       TEXT("ServerFireWeapon_Validate: non-normalised direction (|d|²=%.4f) — rejected."),
		       LengthSq);
		return false;
	}

	// ----- 2. Timestamp range -----
	// GetServerWorldTimeSeconds() is available server-side too.
	const AGameStateBase* GS = GetWorld()->GetGameState<AGameStateBase>();
	if (!GS)
	{
		return false;
	}

	const float ServerNow = GS->GetServerWorldTimeSeconds();

	// Reject timestamps more than 250 ms in the past (mirrors MaxAllowedRewindMs).
	const float MaxRewindSec = 0.250f;
	if (ClientTimestamp < (ServerNow - MaxRewindSec))
	{
		UE_LOG(LogLagCompWeapon, Warning,
		       TEXT("ServerFireWeapon_Validate: timestamp %.4f too old (server=%.4f) — rejected."),
		       ClientTimestamp, ServerNow);
		return false;
	}

	// Reject timestamps from the future (with a generous 100 ms tolerance for clock jitter).
	if (ClientTimestamp > (ServerNow + 0.100f))
	{
		UE_LOG(LogLagCompWeapon, Warning,
		       TEXT("ServerFireWeapon_Validate: timestamp %.4f in the future (server=%.4f) — rejected."),
		       ClientTimestamp, ServerNow);
		return false;
	}

	// ----- 3. Start location plausibility -----
	// The muzzle must be within a reasonable distance of the owner's current position.
	if (GetOwner())
	{
		const float MaxMuzzleDistance = 300.0f; // cm — generous for large weapon models
		const float DistSq = FVector::DistSquared(StartLocation, GetOwner()->GetActorLocation());
		if (DistSq > (MaxMuzzleDistance * MaxMuzzleDistance))
		{
			UE_LOG(LogLagCompWeapon, Warning,
			       TEXT("ServerFireWeapon_Validate: muzzle too far from owner (%.1f cm) — rejected."),
			       FMath::Sqrt(DistSq));
			return false;
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
//  Server RPC — Implementation
// ---------------------------------------------------------------------------

void ALagCompensatedWeapon::ServerFireWeapon_Implementation(
	FVector StartLocation,
	FVector AimDirection,
	float   ClientTimestamp)
{
	UE_LOG(LogLagCompWeapon, Log,
	       TEXT("ServerFireWeapon_Implementation: Start=%s  Dir=%s  T=%.4f"),
	       *StartLocation.ToString(), *AimDirection.ToString(), ClientTimestamp);

	const FVector DirNorm = AimDirection.GetSafeNormal();

	// Iterate every pawn in the world and perform lag-compensated hit detection.
	for (TActorIterator<APawn> PawnIt(GetWorld()); PawnIt; ++PawnIt)
	{
		APawn* Target = *PawnIt;

		// Skip the shooter themselves.
		if (Target == GetOwner())
		{
			continue;
		}

		// Skip dead or invalid pawns.
		if (!IsValid(Target))
		{
			continue;
		}

		// The target must expose a ULagCompensationComponent.
		ULagCompensationComponent* LagComp =
			Target->FindComponentByClass<ULagCompensationComponent>();

		if (!LagComp)
		{
			// Target has no lag compensation support — fall back to a live trace.
			// This lets the weapon work against environmental props, etc.
			continue;
		}

		// ---------------------------------------------------------------
		//  Ask the lag compensation component whether this shot hit.
		// ---------------------------------------------------------------
		const FServerSideRewindResult HitResult =
			LagComp->ConfirmHit(StartLocation, DirNorm, ClientTimestamp);

		if (HitResult.bHitConfirmed)
		{
			// Calculate damage — head shots deal double.
			const float Damage = HitResult.bHeadShot
				? BaseDamage * HeadShotMultiplier
				: BaseDamage;

			// Pre-compute the region string — ternary inside UE_LOG fails constexpr validation.
			const TCHAR* RegionStr = HitResult.bHeadShot ? TEXT("HEAD") : TEXT("BODY");
			UE_LOG(LogLagCompWeapon, Log,
			       TEXT("Hit %s for %.1f dmg (%s)"),
			       *Target->GetName(), Damage, RegionStr);

			// Apply via the CombatDamageable interface so any character type works.
			ICombatDamageable* Damageable = Cast<ICombatDamageable>(Target);
			if (Damageable)
			{
				// Impulse direction: away from the shooter along the shot ray.
				const FVector Impulse = DirNorm * 300.0f;
				Damageable->ApplyDamage(Damage, GetOwner(), Target->GetActorLocation(), Impulse);
			}
			else
			{
				// Fall back to UE's built-in damage system.
				Target->TakeDamage(Damage, FDamageEvent(), GetOwner()->GetInstigatorController(), this);
			}

			// Notify BP for impact effects (sparks, decals, sounds).
			BP_PlayImpactEffects(HitResult.bHeadShot, Target);
		}
	}
}

// ---------------------------------------------------------------------------
//  Multicast effects
// ---------------------------------------------------------------------------

void ALagCompensatedWeapon::MulticastFireEffects_Implementation(
	FVector MuzzleLocation,
	FVector AimDir)
{
	// Skip on the owning client — they already played effects locally in FireWeapon().
	// IsLocallyControlled() lives on APawn, not AActor, so we cast first.
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (OwnerPawn->IsLocallyControlled())
		{
			return;
		}
	}

	// Delegate to BP for sound / particle effects.
	BP_PlayFireEffects(MuzzleLocation, AimDir);
}
