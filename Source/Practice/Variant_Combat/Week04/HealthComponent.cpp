// Copyright Epic Games, Inc. All Rights Reserved.

#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY(LogHealthComponent);

// ---------------------------------------------------------------------------
//  Constructor
// ---------------------------------------------------------------------------

UHealthComponent::UHealthComponent()
{
	// Component does not need a tick — all updates flow through RPCs / RepNotify
	PrimaryComponentTick.bCanEverTick = false;

	// This component must replicate so that property replication works
	SetIsReplicatedByDefault(true);
}

// ---------------------------------------------------------------------------
//  BeginPlay
// ---------------------------------------------------------------------------

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Seed CurrentHealth server-side; clients receive the value via replication
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CurrentHealth = MaxHealth;
	}
}

// ---------------------------------------------------------------------------
//  GetLifetimeReplicatedProps
//  Unreal will issue a compile-time warning if this is missing for any
//  component that has UPROPERTY(Replicated) or UPROPERTY(ReplicatedUsing=...)
// ---------------------------------------------------------------------------

void UHealthComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate MaxHealth to all clients (no RepNotify needed)
	DOREPLIFETIME(UHealthComponent, MaxHealth);

	// Replicate CurrentHealth to all clients, fire OnRep_CurrentHealth on change
	DOREPLIFETIME(UHealthComponent, CurrentHealth);
}

// ---------------------------------------------------------------------------
//  OnRep_CurrentHealth — called on clients when CurrentHealth is updated
// ---------------------------------------------------------------------------

void UHealthComponent::OnRep_CurrentHealth()
{
	// Log the new value to screen so it's visible during PIE testing
	const FString Message = FString::Printf(
	    TEXT("[Client] %s — CurrentHealth changed to %.1f"),
	    *GetOwner()->GetName(), CurrentHealth);

	UE_LOG(LogHealthComponent, Warning, TEXT("%s"), *Message);

	if (GEngine)
	{
		// Display on screen for 3 seconds (key -1 = auto-assign unique key)
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, Message);
	}
}

// ---------------------------------------------------------------------------
//  ServerApplyDamage — Server RPC with Validation
// ---------------------------------------------------------------------------

bool UHealthComponent::ServerApplyDamage_Validate(float Amount)
{
	// Reject obvious cheats: damage must be positive and at most MaxHealth
	// A client sending Amount <= 0 or Amount > MaxHealth is misbehaving
	if (Amount <= 0.0f || Amount > MaxHealth)
	{
		UE_LOG(LogHealthComponent, Warning,
		       TEXT("ServerApplyDamage_Validate REJECTED: Amount=%.1f "
		            "(MaxHealth=%.1f) from %s"),
		       Amount, MaxHealth, *GetOwner()->GetName());
		return false;   // returning false closes the connection on the caller
	}

	return true;
}

void UHealthComponent::ServerApplyDamage_Implementation(float Amount)
{
	// Guard: do not process damage if the actor is already dead
	if (IsDead())
	{
		return;
	}

	// Subtract damage, clamped at zero
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - Amount);

	// Manually call the RepNotify on the server — by default OnRep functions
	// are NOT called on the authority, only on clients
	OnRep_CurrentHealth();

	// Fire the cosmetic multicast flash on every client (including server)
	MulticastDamageFlash();
}

// ---------------------------------------------------------------------------
//  MulticastDamageFlash — NetMulticast, Unreliable cosmetic RPC
// ---------------------------------------------------------------------------

void UHealthComponent::MulticastDamageFlash_Implementation()
{
	// Log and display a brief message so we can confirm it fires on all peers
	const FString Message = FString::Printf(
	    TEXT("[Flash] Damage flash on %s"), *GetOwner()->GetName());

	UE_LOG(LogHealthComponent, Log, TEXT("%s"), *Message);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, Message);
	}
}

// ---------------------------------------------------------------------------
//  ResetHealth
// ---------------------------------------------------------------------------

void UHealthComponent::ResetHealth()
{
	// Only the authority should reset health
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CurrentHealth = MaxHealth;

		// Notify the server's own logic
		OnRep_CurrentHealth();
	}
}
