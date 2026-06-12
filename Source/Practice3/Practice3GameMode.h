// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Practice3GameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class APractice3GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	APractice3GameMode();
};



