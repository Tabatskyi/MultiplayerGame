// Week 08 — Steam Online Sessions
// ATDMMainMenuGameMode: minimal GameMode that assigns ATDMMenuPlayerController.
// All widget + delegate logic lives in ATDMMenuPlayerController (runs on client).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TDMMainMenuGameMode.generated.h"

UCLASS()
class ATDMMainMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ATDMMainMenuGameMode();
};
