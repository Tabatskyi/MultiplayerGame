// Fill out your copyright notice in the Description page of Project Settings.


#include "UHealthComponent.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UUHealthComponent::UUHealthComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// Enable replication
	SetIsReplicated(true);
}

void UUHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UUHealthComponent, MaxHealth);
	DOREPLIFETIME(UUHealthComponent, CurrentHealth);
}

// Called when the game starts
void UUHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize CurrentHealth to MaxHealth at start
	if (GetOwnerRole() == ROLE_Authority)
	{
		CurrentHealth = MaxHealth;
	}
}

void UUHealthComponent::OnRep_CurrentHealth()
{
	// Log the new health value to screen on clients
	FString HealthMessage = FString::Printf(TEXT("Health changed to: %.2f / %.2f"), CurrentHealth, MaxHealth);
	UE_LOG(LogTemp, Warning, TEXT("%s"), *HealthMessage);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			5.0f,
			FColor::Red,
			HealthMessage
		);
	}
}

// Called every frame
void UUHealthComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UUHealthComponent::ServerApplyDamage_Implementation(float Amount)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		// Apply damage
		CurrentHealth = FMath::Clamp(CurrentHealth - Amount, 0.0f, MaxHealth);
		
		// Call the RepNotify manually on server since it's not called automatically there
		OnRep_CurrentHealth();
		
		// Call multicast damage flash effect
		MulticastDamageFlash();
	}
}

bool UUHealthComponent::ServerApplyDamage_Validate(float Amount)
{
	// Validate: Amount must be positive and not exceed MaxHealth
	if (Amount <= 0.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerApplyDamage validation failed: Amount must be > 0, got %.2f"), Amount);
		return false;
	}
	
	if (Amount > MaxHealth)
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerApplyDamage validation failed: Amount (%.2f) exceeds MaxHealth (%.2f)"), Amount, MaxHealth);
		return false;
	}
	
	return true;
}

void UUHealthComponent::MulticastDamageFlash_Implementation()
{
	// Multicast damage flash effect
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Yellow,
			TEXT("*** DAMAGE FLASH ***")
		);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Damage flash effect triggered on all clients"));
}



