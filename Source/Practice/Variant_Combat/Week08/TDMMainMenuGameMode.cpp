// Week 08 — Steam Online Sessions
// ATDMMainMenuGameMode implementation.

#include "TDMMainMenuGameMode.h"
#include "TDMMenuPlayerController.h"

ATDMMainMenuGameMode::ATDMMainMenuGameMode()
{
    PlayerControllerClass = ATDMMenuPlayerController::StaticClass();
}
