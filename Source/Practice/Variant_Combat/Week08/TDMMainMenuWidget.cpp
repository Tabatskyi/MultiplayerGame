// Week 08 — Steam Online Sessions
// STDMMainMenuWidget: pure Slate implementation.

#include "TDMMainMenuWidget.h"
#include "TDMSessionSubsystem.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"

// ─────────────────────────────────────────────────────────────────────────────
void STDMMainMenuWidget::Construct(const FArguments& InArgs)
{
    SessionSubsystem = InArgs._SessionSubsystem;

    const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold",    22);
    const FSlateFontInfo HeadFont  = FCoreStyle::GetDefaultFontStyle("Bold",    14);
    const FSlateFontInfo BodyFont  = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    const FLinearColor   Gold      = FLinearColor(1.f, 0.75f, 0.1f);

    ChildSlot
    [
        SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SBox).WidthOverride(520.f)
            [
                SNew(SVerticalBox)

                // ── Title ─────────────────────────────────────────────────
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 28.f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(TEXT("TDM  ·  ONLINE SESSIONS")))
                    .Font(TitleFont)
                    .ColorAndOpacity(Gold)
                    .Justification(ETextJustify::Center)
                ]

                // ── HOST ──────────────────────────────────────────────────
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
                [
                    SNew(SBox).HeightOverride(46.f)
                    [
                        SNew(SButton)
                        .HAlign(HAlign_Center).VAlign(VAlign_Center)
                        .OnClicked_Lambda([this]() -> FReply { return HandleHost(); })
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("HOST GAME  (4 players)")))
                            .Font(HeadFont)
                        ]
                    ]
                ]

                // ── FIND ──────────────────────────────────────────────────
                + SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)
                [
                    SNew(SBox).HeightOverride(46.f)
                    [
                        SNew(SButton)
                        .HAlign(HAlign_Center).VAlign(VAlign_Center)
                        .OnClicked_Lambda([this]() -> FReply { return HandleFind(); })
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(TEXT("FIND GAMES")))
                            .Font(HeadFont)
                        ]
                    ]
                ]

                // ── Results scroll box ─────────────────────────────────────
                + SVerticalBox::Slot().MaxHeight(300.f).Padding(0.f, 0.f, 0.f, 12.f)
                [
                    SNew(SScrollBox)
                    + SScrollBox::Slot()
                    [
                        SAssignNew(ResultsBox, SVerticalBox)
                    ]
                ]

                // ── Status text ────────────────────────────────────────────
                + SVerticalBox::Slot().AutoHeight()
                [
                    SAssignNew(StatusLabel, STextBlock)
                    .Text(FText::GetEmpty())
                    .Font(BodyFont)
                    .ColorAndOpacity(FLinearColor::Yellow)
                    .Justification(ETextJustify::Center)
                ]
            ]
        ]
    ];
}

// ─────────────────────────────────────────────────────────────────────────────
FReply STDMMainMenuWidget::HandleHost()
{
    if (SessionSubsystem.IsValid())
    {
        SetStatus(TEXT("Creating session…"), FLinearColor::Yellow);
        SessionSubsystem->HostSession(4, TEXT("TDMSession"));
    }
    return FReply::Handled();
}

FReply STDMMainMenuWidget::HandleFind()
{
    if (SessionSubsystem.IsValid())
    {
        SetStatus(TEXT("Searching…"), FLinearColor::Yellow);
        SessionSubsystem->FindSessions(20);
    }
    return FReply::Handled();
}

// ─────────────────────────────────────────────────────────────────────────────
void STDMMainMenuWidget::SetStatus(const FString& Msg, FLinearColor Color)
{
    if (StatusLabel.IsValid())
    {
        StatusLabel->SetText(FText::FromString(Msg));
        StatusLabel->SetColorAndOpacity(Color);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void STDMMainMenuWidget::RebuildResults()
{
    if (!ResultsBox.IsValid() || !SessionSubsystem.IsValid()) return;

    ResultsBox->ClearChildren();

    const FSlateFontInfo RowFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
    const int32 Num = SessionSubsystem->GetNumSearchResults();

    for (int32 i = 0; i < Num; ++i)
    {
        const FString Label = SessionSubsystem->GetSearchResultDisplayString(i);
        TWeakObjectPtr<UTDMSessionSubsystem> WeakSub = SessionSubsystem;
        const int32 Idx = i;

        ResultsBox->AddSlot().AutoHeight().Padding(0.f, 2.f)
        [
            SNew(SBox).HeightOverride(40.f)
            [
                SNew(SButton)
                .HAlign(HAlign_Left).VAlign(VAlign_Center)
                .OnClicked_Lambda([WeakSub, Idx]() -> FReply
                {
                    if (WeakSub.IsValid()) WeakSub->JoinSessionByIndex(Idx);
                    return FReply::Handled();
                })
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(FString::Printf(TEXT("[JOIN]  %s"), *Label)))
                    .Font(RowFont)
                ]
            ]
        ];
    }
}
