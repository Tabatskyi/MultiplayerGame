// Copyright Epic Games, Inc. All Rights Reserved.


#include "Practice3PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "Practice3.h"
#include "UHealthComponent.h"
#include "Widgets/Input/SVirtualJoystick.h"

void APractice3PlayerController::BeginPlay()
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

void APractice3PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			

        // Setup damage test key bindings
        if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
        {
                if (DamageTestAction)
                {
                        EnhancedInputComponent->BindAction(DamageTestAction, ETriggerEvent::Started, this, &APractice3PlayerController::OnDamageTestPressed);
                }
                if (InvalidDamageTestAction)
                {
                        EnhancedInputComponent->BindAction(InvalidDamageTestAction, ETriggerEvent::Started, this, &APractice3PlayerController::OnInvalidDamageTestPressed);
                }
        }
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
	}
}

bool APractice3PlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}


void APractice3PlayerController::OnDamageTestPressed()
{
        // Apply valid damage (10.0f) to nearby enemy
        ApplyDamageToNearbyEnemy(10.0f);
}

void APractice3PlayerController::OnInvalidDamageTestPressed()
{
        // Try to apply invalid damage (-99999.0f) to test validation
        ApplyDamageToNearbyEnemy(-99999.0f);
}

void APractice3PlayerController::ApplyDamageToNearbyEnemy(float DamageAmount)
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
