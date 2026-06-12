// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Practice3PlayerController.generated.h"

class UInputMappingContext;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class APractice3PlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Damage test input action (F key) */
	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* DamageTestAction;

	/** Invalid damage test input action (G key) */
	UPROPERTY(EditAnywhere, Category="Input")
	class UInputAction* InvalidDamageTestAction;

private:

	/** Called when the damage test key is pressed */
	void OnDamageTestPressed();

	/** Called when the invalid damage test key is pressed */
	void OnInvalidDamageTestPressed();

	/** Helper function to find and damage nearby enemies */
	void ApplyDamageToNearbyEnemy(float DamageAmount);

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

};
