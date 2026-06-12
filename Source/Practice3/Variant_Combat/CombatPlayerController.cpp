// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Combat/CombatPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "CombatCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "Practice3.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "UHealthComponent.h"

void ACombatPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogPractice3, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ACombatPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		// Setup damage test key bindings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			if (DamageTestAction)
			{
				EnhancedInputComponent->BindAction(DamageTestAction, ETriggerEvent::Started, this, &ACombatPlayerController::OnDamageTestPressed);
			}
			if (InvalidDamageTestAction)
			{
				EnhancedInputComponent->BindAction(InvalidDamageTestAction, ETriggerEvent::Started, this, &ACombatPlayerController::OnInvalidDamageTestPressed);
			}
		}
	}
}

void ACombatPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// subscribe to the pawn's OnDestroyed delegate
	InPawn->OnDestroyed.AddDynamic(this, &ACombatPlayerController::OnPawnDestroyed);
}

void ACombatPlayerController::SetRespawnTransform(const FTransform& NewRespawn)
{
	// save the new respawn transform
	RespawnTransform = NewRespawn;
}

void ACombatPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// spawn a new character at the respawn transform
	if (ACombatCharacter* RespawnedCharacter = GetWorld()->SpawnActor<ACombatCharacter>(CharacterClass, RespawnTransform))
	{
		// possess the character
		Possess(RespawnedCharacter);
	}
}

bool ACombatPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ACombatPlayerController::OnDamageTestPressed()
{
	// Apply valid damage (10.0f) to nearby enemy
	ApplyDamageToNearbyEnemy(10.0f);
}

void ACombatPlayerController::OnInvalidDamageTestPressed()
{
	// Try to apply invalid damage (-99999.0f) to test validation
	ApplyDamageToNearbyEnemy(-99999.0f);
}

void ACombatPlayerController::ApplyDamageToNearbyEnemy(float DamageAmount)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	// Simple raycast to find a target
	FVector StartLoc = ControlledPawn->GetActorLocation();
	FVector ForwardDir = ControlledPawn->GetActorForwardVector();
	FVector EndLoc = StartLoc + ForwardDir * 2000.0f;

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(ControlledPawn);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLoc, EndLoc, ECC_Pawn, QueryParams))
	{
		AActor* HitActor = HitResult.GetActor();
		if (HitActor && HitActor != ControlledPawn)
		{
			if (UUHealthComponent* HealthComponent = HitActor->FindComponentByClass<UUHealthComponent>())
			{
				HealthComponent->ServerApplyDamage(DamageAmount);
				UE_LOG(LogTemp, Warning, TEXT("Damage test: %s applied %.2f damage to %s"), *ControlledPawn->GetName(), DamageAmount, *HitActor->GetName());
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No target found in range for damage test"));
	}
}
