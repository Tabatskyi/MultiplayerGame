// Week 08 — Steam Online Sessions
// ATDMMenuPlayerController implementation.

#include "TDMMenuPlayerController.h"
#include "TDMMainMenuWidget.h"
#include "TDMSessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogTDMMenuPC, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
void ATDMMenuPlayerController::BeginPlayingState()
{
    Super::BeginPlayingState();

    // Only the local (owning) client should create a UI widget.
    if (!IsLocalController())
    {
        return;
    }

    UE_LOG(LogTDMMenuPC, Log, TEXT("[MenuPC] BeginPlayingState — local controller, building UI"));

    // Get the session subsystem and bind delegates.
    if (UGameInstance* GI = GetGameInstance())
    {
        SessionSubsystem = GI->GetSubsystem<UTDMSessionSubsystem>();
    }

    if (SessionSubsystem)
    {
        SessionSubsystem->OnHostSessionComplete .AddUniqueDynamic(this, &ATDMMenuPlayerController::OnHostComplete);
        SessionSubsystem->OnFindSessionsComplete.AddUniqueDynamic(this, &ATDMMenuPlayerController::OnFindComplete);
        SessionSubsystem->OnJoinSessionComplete .AddUniqueDynamic(this, &ATDMMenuPlayerController::OnJoinComplete);
        UE_LOG(LogTDMMenuPC, Log, TEXT("[MenuPC] Session subsystem bound"));
    }
    else
    {
        UE_LOG(LogTDMMenuPC, Warning, TEXT("[MenuPC] TDMSessionSubsystem not found — is Steam running?"));
    }

    // Build and show the Slate widget.
    if (!GEngine || !GEngine->GameViewport)
    {
        UE_LOG(LogTDMMenuPC, Error, TEXT("[MenuPC] No GameViewport"));
        return;
    }

    MenuWidget = SNew(STDMMainMenuWidget)
        .SessionSubsystem(TWeakObjectPtr<UTDMSessionSubsystem>(SessionSubsystem));

    GEngine->GameViewport->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 10);

    bShowMouseCursor = true;
    SetInputMode(FInputModeUIOnly());

    UE_LOG(LogTDMMenuPC, Log, TEXT("[MenuPC] Menu widget added to viewport"));
}

// ─────────────────────────────────────────────────────────────────────────────
void ATDMMenuPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (SessionSubsystem)
    {
        SessionSubsystem->OnHostSessionComplete .RemoveDynamic(this, &ATDMMenuPlayerController::OnHostComplete);
        SessionSubsystem->OnFindSessionsComplete.RemoveDynamic(this, &ATDMMenuPlayerController::OnFindComplete);
        SessionSubsystem->OnJoinSessionComplete .RemoveDynamic(this, &ATDMMenuPlayerController::OnJoinComplete);
    }

    if (MenuWidget.IsValid() && GEngine && GEngine->GameViewport)
    {
        GEngine->GameViewport->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
    }

    Super::EndPlay(EndPlayReason);
}

// ─────────────────────────────────────────────────────────────────────────────
void ATDMMenuPlayerController::OnHostComplete(bool bSuccess)
{
    if (!MenuWidget.IsValid()) return;
    if (bSuccess)
        MenuWidget->SetStatus(TEXT("Session created — loading map…"), FLinearColor::Green);
    else
        MenuWidget->SetStatus(TEXT("Host failed. Is Steam running?"), FLinearColor::Red);
}

void ATDMMenuPlayerController::OnFindComplete(bool bSuccess)
{
    if (!MenuWidget.IsValid()) return;
    const int32 Num = SessionSubsystem ? SessionSubsystem->GetNumSearchResults() : 0;
    if (!bSuccess || Num == 0)
        MenuWidget->SetStatus(TEXT("No sessions found."), FLinearColor(0.8f, 0.8f, 0.8f));
    else
        MenuWidget->SetStatus(FString::Printf(TEXT("%d session(s) found:"), Num), FLinearColor::Green);
    MenuWidget->RebuildResults();
}

void ATDMMenuPlayerController::OnJoinComplete(bool bSuccess)
{
    if (!MenuWidget.IsValid()) return;
    if (bSuccess)
        MenuWidget->SetStatus(TEXT("Joining — travelling to host…"), FLinearColor::Green);
    else
        MenuWidget->SetStatus(TEXT("Join failed. Session may be full."), FLinearColor::Red);
}
