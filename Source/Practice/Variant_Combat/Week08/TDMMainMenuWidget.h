// Week 08 — Steam Online Sessions
// STDMMainMenuWidget: pure SCompoundWidget. No UMG, no Blueprint required.
// Added directly to the viewport via GEngine->GameViewport->AddViewportWidgetContent.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SBoxPanel.h"     // SVerticalBox (merged here in UE5.8)
#include "Widgets/Text/STextBlock.h"

class UTDMSessionSubsystem;

class STDMMainMenuWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STDMMainMenuWidget) {}
        SLATE_ARGUMENT(TWeakObjectPtr<UTDMSessionSubsystem>, SessionSubsystem)
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

    /** Called by the GameMode when a host/find/join delegate fires. */
    void SetStatus(const FString& Msg, FLinearColor Color = FLinearColor::White);
    void RebuildResults();

private:
    TWeakObjectPtr<UTDMSessionSubsystem> SessionSubsystem;
    TSharedPtr<SVerticalBox>             ResultsBox;
    TSharedPtr<STextBlock>               StatusLabel;

    FReply HandleHost();
    FReply HandleFind();
};
