// Week 08 — Steam Online Sessions
// ATDMMenuPlayerController: creates the main menu Slate widget on the owning client.
// BeginPlayingState() is called on the CLIENT that owns this controller — correct for UI.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TDMMenuPlayerController.generated.h"

class STDMMainMenuWidget;
class UTDMSessionSubsystem;

UCLASS()
class ATDMMenuPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    // Called on the owning client when entering playing state.
    virtual void BeginPlayingState() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    TSharedPtr<STDMMainMenuWidget> MenuWidget;

    UPROPERTY()
    TObjectPtr<UTDMSessionSubsystem> SessionSubsystem;

    UFUNCTION() void OnHostComplete(bool bSuccess);
    UFUNCTION() void OnFindComplete(bool bSuccess);
    UFUNCTION() void OnJoinComplete(bool bSuccess);
};
